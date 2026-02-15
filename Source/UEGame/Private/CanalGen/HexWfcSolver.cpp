#include "CanalGen/HexWfcSolver.h"

#include "Containers/Queue.h"

bool FHexWfcGridConfig::EnsureValid(FString& OutError) const
{
	if (Width <= 0 || Height <= 0)
	{
		OutError = FString::Printf(TEXT("Grid dimensions must be > 0. Width=%d Height=%d"), Width, Height);
		return false;
	}
	return true;
}

bool FHexWfcGridConfig::Contains(const FHexAxialCoord& Coord) const
{
	return Coord.Q >= 0 && Coord.Q < Width && Coord.R >= 0 && Coord.R < Height;
}

FHexWfcSolver::FHexWfcSolver(const FCanalTileCompatibilityTable& InCompatibility)
	: Compatibility(InCompatibility)
{
}

FHexWfcSolveResult FHexWfcSolver::Solve(const FHexWfcGridConfig& Grid, const FHexWfcSolveConfig& Config) const
{
	FHexWfcSolveResult FinalResult;
	FinalResult.TotalCells = Grid.Width * Grid.Height;

	FString GridError;
	if (!Grid.EnsureValid(GridError))
	{
		FinalResult.Message = GridError;
		return FinalResult;
	}

	if (Config.MaxAttempts <= 0)
	{
		FinalResult.Message = TEXT("MaxAttempts must be > 0.");
		return FinalResult;
	}

	if (!Compatibility.IsBuilt() || Compatibility.GetAllVariants().Num() == 0)
	{
		FinalResult.Message = TEXT("Compatibility table is not built or has zero variants.");
		return FinalResult;
	}

	FString LastFailure = TEXT("Unknown failure.");

	for (int32 Attempt = 1; Attempt <= Config.MaxAttempts; ++Attempt)
	{
		TMap<FHexAxialCoord, FCellState> States;
		States.Reserve(FinalResult.TotalCells);
		for (int32 R = 0; R < Grid.Height; ++R)
		{
			for (int32 Q = 0; Q < Grid.Width; ++Q)
			{
				FCellState Cell;
				Cell.Candidates = Compatibility.GetAllVariants();
				States.Add(FHexAxialCoord(Q, R), MoveTemp(Cell));
			}
		}

		FRandomStream Random(Config.Seed + (Attempt - 1) * 7919);
		FHexWfcSolveResult AttemptResult;
		AttemptResult.TotalCells = FinalResult.TotalCells;
		AttemptResult.AttemptsUsed = Attempt;

		bool bAttemptContradiction = false;

		while (true)
		{
			FHexAxialCoord TargetCell;
			int32 Entropy = 0;
			if (!SelectLowestEntropyCell(States, TargetCell, Entropy))
			{
				break;
			}

			FCellState& TargetState = States[TargetCell];
			const FCanalTileVariantKey Picked = ChooseVariant(TargetState.Candidates, Random);
			TargetState.Candidates = {Picked};

			TQueue<FHexAxialCoord> Queue;
			Queue.Enqueue(TargetCell);

			while (!Queue.IsEmpty())
			{
				if (AttemptResult.PropagationSteps >= Config.MaxPropagationSteps)
				{
					bAttemptContradiction = true;
					AttemptResult.bContradiction = true;
					AttemptResult.Message = FString::Printf(TEXT("Exceeded propagation budget (%d)."), Config.MaxPropagationSteps);
					break;
				}

				FHexAxialCoord Current;
				Queue.Dequeue(Current);
				const TArray<FCanalTileVariantKey>& CurrentCandidates = States[Current].Candidates;

				for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
				{
					const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
					const FHexAxialCoord Neighbor = Current.Neighbor(Direction);

					FCellState* NeighborState = States.Find(Neighbor);
					if (!NeighborState)
					{
						continue;
					}

					TArray<FCanalTileVariantKey> Filtered;
					Filtered.Reserve(NeighborState->Candidates.Num());
					for (const FCanalTileVariantKey& Candidate : NeighborState->Candidates)
					{
						if (IsVariantAllowedByAnySource(Candidate, CurrentCandidates, Direction))
						{
							Filtered.Add(Candidate);
						}
					}

					if (Filtered.Num() == 0)
					{
						bAttemptContradiction = true;
						AttemptResult.bContradiction = true;
						AttemptResult.Message = FString::Printf(
							TEXT("Contradiction at %s when propagating from %s."),
							*Neighbor.ToString(),
							*Current.ToString());
						break;
					}

					if (Filtered.Num() != NeighborState->Candidates.Num())
					{
						NeighborState->Candidates = MoveTemp(Filtered);
						Queue.Enqueue(Neighbor);
					}

					++AttemptResult.PropagationSteps;
				}

				if (bAttemptContradiction)
				{
					break;
				}
			}

			if (bAttemptContradiction)
			{
				break;
			}
		}

		if (bAttemptContradiction)
		{
			LastFailure = FString::Printf(TEXT("Attempt %d failed: %s"), Attempt, *AttemptResult.Message);
			continue;
		}

		FHexBoundaryPort ResolvedEntryPort;
		FHexBoundaryPort ResolvedExitPort;
		FString ValidationError;
		if (!ValidateSolvedState(States, Grid, Config, ResolvedEntryPort, ResolvedExitPort, ValidationError))
		{
			LastFailure = FString::Printf(TEXT("Attempt %d rejected by validation: %s"), Attempt, *ValidationError);
			continue;
		}

		AttemptResult.bSolved = true;
		AttemptResult.Message = FString::Printf(TEXT("Solved in attempt %d."), Attempt);
		AttemptResult.Cells.Reserve(FinalResult.TotalCells);
		for (auto& Pair : States)
		{
			FHexWfcCellResult Cell;
			Cell.Coord = Pair.Key;
			if (Pair.Value.Candidates.Num() > 0)
			{
				Cell.Variant = Compatibility.ToVariantRef(Pair.Value.Candidates[0]);
			}
			AttemptResult.Cells.Add(Cell);
		}

		AttemptResult.Cells.Sort([](const FHexWfcCellResult& A, const FHexWfcCellResult& B)
		{
			if (A.Coord.R != B.Coord.R)
			{
				return A.Coord.R < B.Coord.R;
			}
			return A.Coord.Q < B.Coord.Q;
		});

		AttemptResult.CollapsedCells = AttemptResult.Cells.Num();
		AttemptResult.ResolvedEntryPort = ResolvedEntryPort;
		AttemptResult.ResolvedExitPort = ResolvedExitPort;
		AttemptResult.bHasResolvedPorts = ResolvedEntryPort.bEnabled && ResolvedExitPort.bEnabled;
		return AttemptResult;
		}

	FinalResult.bSolved = false;
	FinalResult.bContradiction = true;
	FinalResult.AttemptsUsed = Config.MaxAttempts;
	FinalResult.Message = LastFailure;
	return FinalResult;
}

bool FHexWfcSolver::SelectLowestEntropyCell(
	const TMap<FHexAxialCoord, FCellState>& States,
	FHexAxialCoord& OutCoord,
	int32& OutEntropy) const
{
	bool bFound = false;
	int32 BestEntropy = MAX_int32;
	FHexAxialCoord BestCoord;

	for (const TPair<FHexAxialCoord, FCellState>& Pair : States)
	{
		const int32 Entropy = Pair.Value.Candidates.Num();
		if (Entropy <= 1)
		{
			continue;
		}

		if (!bFound || Entropy < BestEntropy)
		{
			bFound = true;
			BestEntropy = Entropy;
			BestCoord = Pair.Key;
			continue;
		}

		if (Entropy == BestEntropy)
		{
			if (Pair.Key.R < BestCoord.R || (Pair.Key.R == BestCoord.R && Pair.Key.Q < BestCoord.Q))
			{
				BestCoord = Pair.Key;
			}
		}
	}

	if (!bFound)
	{
		return false;
	}

	OutCoord = BestCoord;
	OutEntropy = BestEntropy;
	return true;
}

FCanalTileVariantKey FHexWfcSolver::ChooseVariant(const TArray<FCanalTileVariantKey>& Candidates, FRandomStream& Random) const
{
	check(Candidates.Num() > 0);

	TArray<FCanalTileVariantKey> Sorted = Candidates;
	Sorted.Sort([](const FCanalTileVariantKey& A, const FCanalTileVariantKey& B)
	{
		if (A.TileIndex != B.TileIndex)
		{
			return A.TileIndex < B.TileIndex;
		}
		return A.RotationSteps < B.RotationSteps;
	});

	float TotalWeight = 0.0f;
	for (const FCanalTileVariantKey& Candidate : Sorted)
	{
		if (const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Candidate.TileIndex))
		{
			TotalWeight += FMath::Max(0.0f, Tile->Weight);
		}
	}

	if (TotalWeight <= KINDA_SMALL_NUMBER)
	{
		return Sorted[0];
	}

	const float Pick = Random.FRandRange(0.0f, TotalWeight);
	float Cumulative = 0.0f;
	for (const FCanalTileVariantKey& Candidate : Sorted)
	{
		const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Candidate.TileIndex);
		const float Weight = Tile ? FMath::Max(0.0f, Tile->Weight) : 0.0f;
		Cumulative += Weight;
		if (Pick <= Cumulative)
		{
			return Candidate;
		}
	}

	return Sorted.Last();
}

bool FHexWfcSolver::IsVariantAllowedByAnySource(
	const FCanalTileVariantKey& Candidate,
	const TArray<FCanalTileVariantKey>& SourceCandidates,
	const EHexDirection SourceToTargetDirection) const
{
	for (const FCanalTileVariantKey& Source : SourceCandidates)
	{
		const TArray<FCanalTileVariantKey>& Allowed = Compatibility.GetCompatibleVariants(Source, SourceToTargetDirection);
		for (const FCanalTileVariantKey& AllowedVariant : Allowed)
		{
			if (AllowedVariant == Candidate)
			{
				return true;
			}
		}
	}
	return false;
}

bool FHexWfcSolver::ValidateSolvedState(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid,
	const FHexWfcSolveConfig& Config,
	FHexBoundaryPort& OutResolvedEntry,
	FHexBoundaryPort& OutResolvedExit,
	FString& OutError) const
{
	for (const TPair<FHexAxialCoord, FCellState>& Pair : States)
	{
		if (!Pair.Value.IsCollapsed())
		{
			OutError = FString::Printf(TEXT("Cell %s was not collapsed."), *Pair.Key.ToString());
			return false;
		}
	}

	OutResolvedEntry = Config.EntryPort;
	OutResolvedExit = Config.ExitPort;

	if (Config.bRequireEntryExitPath)
	{
		if (!ResolveBoundaryPorts(States, Grid, Config, OutResolvedEntry, OutResolvedExit, OutError))
		{
			return false;
		}

		if (!HasEntryExitPath(States, Grid, OutResolvedEntry, OutResolvedExit))
		{
			OutError = TEXT("No water path exists between Entry and Exit ports.");
			return false;
		}
	}
	else
	{
		if (OutResolvedEntry.bEnabled && !ValidateBoundaryPort(States, Grid, OutResolvedEntry, OutError))
		{
			return false;
		}

		if (OutResolvedExit.bEnabled && !ValidateBoundaryPort(States, Grid, OutResolvedExit, OutError))
		{
			return false;
		}
	}

	if (Config.bDisallowUnassignedBoundaryWater)
	{
		if (!ValidateBoundarySockets(States, Grid, OutResolvedEntry, OutResolvedExit, OutError))
		{
			return false;
		}
	}

	if (Config.bRequireSingleWaterComponent)
	{
		if (!HasSingleWaterComponent(States, Grid))
		{
			OutError = TEXT("Water graph has more than one connected component.");
			return false;
		}
	}

	return true;
}

bool FHexWfcSolver::ResolveBoundaryPorts(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid,
	const FHexWfcSolveConfig& Config,
	FHexBoundaryPort& OutEntry,
	FHexBoundaryPort& OutExit,
	FString& OutError) const
{
	const bool bEntryProvided = Config.EntryPort.bEnabled;
	const bool bExitProvided = Config.ExitPort.bEnabled;

	if (bEntryProvided && bExitProvided)
	{
		if (!ValidateBoundaryPort(States, Grid, Config.EntryPort, OutError))
		{
			return false;
		}
		if (!ValidateBoundaryPort(States, Grid, Config.ExitPort, OutError))
		{
			return false;
		}
		if (Config.EntryPort.Coord == Config.ExitPort.Coord)
		{
			OutError = TEXT("Entry and Exit ports must be on different cells.");
			return false;
		}
		OutEntry = Config.EntryPort;
		OutExit = Config.ExitPort;
		return true;
	}

	if (!Config.bAutoSelectBoundaryPorts)
	{
		OutError = TEXT("Entry/Exit path required but one or both ports are missing and auto-selection is disabled.");
		return false;
	}

	TArray<FHexBoundaryPort> Candidates;
	CollectBoundaryWaterSockets(States, Grid, true, Candidates);
	if (Candidates.Num() < 2)
	{
		OutError = TEXT("Could not auto-select Entry/Exit ports: fewer than two explicit boundary water sockets found.");
		return false;
	}

	auto IsBetter = [](const FHexBoundaryPort& A, const FHexBoundaryPort& B)
	{
		if (A.Coord.R != B.Coord.R)
		{
			return A.Coord.R < B.Coord.R;
		}
		if (A.Coord.Q != B.Coord.Q)
		{
			return A.Coord.Q < B.Coord.Q;
		}
		return HexDirectionToIndex(A.Direction) < HexDirectionToIndex(B.Direction);
	};

	if (bEntryProvided || bExitProvided)
	{
		const FHexBoundaryPort Fixed = bEntryProvided ? Config.EntryPort : Config.ExitPort;
		if (!ValidateBoundaryPort(States, Grid, Fixed, OutError))
		{
			return false;
		}

		bool bFound = false;
		int32 BestDistance = -1;
		FHexBoundaryPort BestCandidate;

		for (const FHexBoundaryPort& Candidate : Candidates)
		{
			if (Candidate.Coord == Fixed.Coord)
			{
				continue;
			}
			if (!HasEntryExitPath(States, Grid, Fixed, Candidate))
			{
				continue;
			}

			const int32 Dist = Fixed.Coord.DistanceTo(Candidate.Coord);
			if (!bFound || Dist > BestDistance || (Dist == BestDistance && IsBetter(Candidate, BestCandidate)))
			{
				bFound = true;
				BestDistance = Dist;
				BestCandidate = Candidate;
			}
		}

		if (!bFound)
		{
			OutError = TEXT("Could not auto-select a matching boundary port for the provided port.");
			return false;
		}

		if (bEntryProvided)
		{
			OutEntry = Fixed;
			OutExit = BestCandidate;
		}
		else
		{
			OutEntry = BestCandidate;
			OutExit = Fixed;
		}
		return true;
	}

	bool bPairFound = false;
	int32 BestDistance = -1;
	FHexBoundaryPort BestEntry;
	FHexBoundaryPort BestExit;

	for (int32 I = 0; I < Candidates.Num(); ++I)
	{
		for (int32 J = I + 1; J < Candidates.Num(); ++J)
		{
			const FHexBoundaryPort& A = Candidates[I];
			const FHexBoundaryPort& B = Candidates[J];
			if (A.Coord == B.Coord)
			{
				continue;
			}
			if (!HasEntryExitPath(States, Grid, A, B))
			{
				continue;
			}

			const int32 Dist = A.Coord.DistanceTo(B.Coord);
			if (!bPairFound || Dist > BestDistance ||
				(Dist == BestDistance && (IsBetter(A, BestEntry) || (IsSamePort(A, BestEntry) && IsBetter(B, BestExit)))))
			{
				bPairFound = true;
				BestDistance = Dist;
				BestEntry = A;
				BestExit = B;
			}
		}
	}

	if (!bPairFound)
	{
		OutError = TEXT("Could not auto-select Entry/Exit ports with a valid water path.");
		return false;
	}

	OutEntry = BestEntry;
	OutExit = BestExit;
	return true;
}

bool FHexWfcSolver::ValidateBoundarySockets(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid,
	const FHexBoundaryPort& Entry,
	const FHexBoundaryPort& Exit,
	FString& OutError) const
{
	TArray<FHexBoundaryPort> BoundaryWaterSockets;
	CollectBoundaryWaterSockets(States, Grid, false, BoundaryWaterSockets);

	for (const FHexBoundaryPort& SocketPort : BoundaryWaterSockets)
	{
		if (IsSamePort(SocketPort, Entry) || IsSamePort(SocketPort, Exit))
		{
			continue;
		}

		const FCellState* Cell = States.Find(SocketPort.Coord);
		if (!Cell || Cell->Candidates.Num() != 1)
		{
			continue;
		}

		const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Cell->Candidates[0].TileIndex);
		const bool bExplicitlyAllowed = Tile && Tile->bAllowAsBoundaryPort;
		if (!bExplicitlyAllowed)
		{
			OutError = FString::Printf(
				TEXT("Unassigned boundary water socket at %s dir=%d is not explicitly allowed."),
				*SocketPort.Coord.ToString(),
				HexDirectionToIndex(SocketPort.Direction));
			return false;
		}
	}

	return true;
}

void FHexWfcSolver::CollectBoundaryWaterSockets(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid,
	const bool bRequireExplicitBoundaryFlag,
	TArray<FHexBoundaryPort>& OutPorts) const
{
	OutPorts.Reset();

	for (int32 R = 0; R < Grid.Height; ++R)
	{
		for (int32 Q = 0; Q < Grid.Width; ++Q)
		{
			const FHexAxialCoord Coord(Q, R);
			const FCellState* Cell = States.Find(Coord);
			if (!Cell || Cell->Candidates.Num() != 1)
			{
				continue;
			}

			const FCanalTileVariantKey Variant = Cell->Candidates[0];
			const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Variant.TileIndex);
			if (!Tile)
			{
				continue;
			}
			if (bRequireExplicitBoundaryFlag && !Tile->bAllowAsBoundaryPort)
			{
				continue;
			}

			for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
			{
				const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
				const FHexAxialCoord OutsideNeighbor = Coord.Neighbor(Direction);
				if (Grid.Contains(OutsideNeighbor))
				{
					continue;
				}

				const ECanalSocketType Socket = Tile->GetSocket(Direction, Variant.RotationSteps);
				if (!IsWaterLikeSocket(Socket))
				{
					continue;
				}

				FHexBoundaryPort Port;
				Port.bEnabled = true;
				Port.Coord = Coord;
				Port.Direction = Direction;
				OutPorts.Add(Port);
			}
		}
	}
}

bool FHexWfcSolver::IsSamePort(const FHexBoundaryPort& A, const FHexBoundaryPort& B)
{
	return A.bEnabled && B.bEnabled && A.Coord == B.Coord && A.Direction == B.Direction;
}

bool FHexWfcSolver::ValidateBoundaryPort(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid,
	const FHexBoundaryPort& Port,
	FString& OutError) const
{
	if (!Grid.Contains(Port.Coord))
	{
		OutError = FString::Printf(TEXT("Port coord %s is outside grid."), *Port.Coord.ToString());
		return false;
	}

	const FHexAxialCoord OutsideNeighbor = Port.Coord.Neighbor(Port.Direction);
	if (Grid.Contains(OutsideNeighbor))
	{
		OutError = FString::Printf(TEXT("Port at %s is not on boundary for direction %d."), *Port.Coord.ToString(), HexDirectionToIndex(Port.Direction));
		return false;
	}

	const FCellState* Cell = States.Find(Port.Coord);
	if (!Cell || Cell->Candidates.Num() != 1)
	{
		OutError = FString::Printf(TEXT("Port cell %s is not collapsed."), *Port.Coord.ToString());
		return false;
	}

	const FCanalTileVariantKey Variant = Cell->Candidates[0];
	const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Variant.TileIndex);
	if (!Tile)
	{
		OutError = TEXT("Port tile lookup failed.");
		return false;
	}

	const ECanalSocketType Socket = Tile->GetSocket(Port.Direction, Variant.RotationSteps);
	if (!IsWaterLikeSocket(Socket))
	{
		OutError = FString::Printf(TEXT("Port at %s does not expose a water/lock socket."), *Port.Coord.ToString());
		return false;
	}

	return true;
}

bool FHexWfcSolver::HasEntryExitPath(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid,
	const FHexBoundaryPort& Entry,
	const FHexBoundaryPort& Exit) const
{
	if (Entry.Coord == Exit.Coord)
	{
		return true;
	}

	TSet<FHexAxialCoord> Visited;
	TQueue<FHexAxialCoord> Queue;

	Queue.Enqueue(Entry.Coord);
	Visited.Add(Entry.Coord);

	while (!Queue.IsEmpty())
	{
		FHexAxialCoord Current;
		Queue.Dequeue(Current);

		for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
		{
			const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
			const FHexAxialCoord Neighbor = Current.Neighbor(Direction);
			if (!Grid.Contains(Neighbor) || Visited.Contains(Neighbor))
			{
				continue;
			}

			if (!AreCellsWaterConnected(States, Current, Neighbor, Direction))
			{
				continue;
			}

			if (Neighbor == Exit.Coord)
			{
				return true;
			}

			Visited.Add(Neighbor);
			Queue.Enqueue(Neighbor);
		}
	}

	return false;
}

bool FHexWfcSolver::HasSingleWaterComponent(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexWfcGridConfig& Grid) const
{
	TSet<FHexAxialCoord> WaterCells;
	for (const TPair<FHexAxialCoord, FCellState>& Pair : States)
	{
		if (IsWaterCell(States, Pair.Key))
		{
			WaterCells.Add(Pair.Key);
		}
	}

	if (WaterCells.Num() <= 1)
	{
		return true;
	}

	TSet<FHexAxialCoord> Visited;
	int32 Components = 0;

	for (const FHexAxialCoord& Start : WaterCells)
	{
		if (Visited.Contains(Start))
		{
			continue;
		}

		++Components;
		if (Components > 1)
		{
			return false;
		}

		TQueue<FHexAxialCoord> Queue;
		Queue.Enqueue(Start);
		Visited.Add(Start);

		while (!Queue.IsEmpty())
		{
			FHexAxialCoord Current;
			Queue.Dequeue(Current);

			for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
			{
				const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
				const FHexAxialCoord Neighbor = Current.Neighbor(Direction);
				if (!Grid.Contains(Neighbor) || !WaterCells.Contains(Neighbor) || Visited.Contains(Neighbor))
				{
					continue;
				}

				if (!AreCellsWaterConnected(States, Current, Neighbor, Direction))
				{
					continue;
				}

				Visited.Add(Neighbor);
				Queue.Enqueue(Neighbor);
			}
		}
	}

	return true;
}

bool FHexWfcSolver::AreCellsWaterConnected(
	const TMap<FHexAxialCoord, FCellState>& States,
	const FHexAxialCoord& Source,
	const FHexAxialCoord& Target,
	const EHexDirection Direction) const
{
	const FCellState* SourceCell = States.Find(Source);
	const FCellState* TargetCell = States.Find(Target);
	if (!SourceCell || !TargetCell || SourceCell->Candidates.Num() != 1 || TargetCell->Candidates.Num() != 1)
	{
		return false;
	}

	const FCanalTileVariantKey SourceVariant = SourceCell->Candidates[0];
	const FCanalTileVariantKey TargetVariant = TargetCell->Candidates[0];

	const FCanalTopologyTileDefinition* SourceTile = Compatibility.GetTileDefinition(SourceVariant.TileIndex);
	const FCanalTopologyTileDefinition* TargetTile = Compatibility.GetTileDefinition(TargetVariant.TileIndex);
	if (!SourceTile || !TargetTile)
	{
		return false;
	}

	const ECanalSocketType SourceSocket = SourceTile->GetSocket(Direction, SourceVariant.RotationSteps);
	const ECanalSocketType TargetSocket = TargetTile->GetSocket(OppositeHexDirection(Direction), TargetVariant.RotationSteps);
	return SourceSocket == TargetSocket && IsWaterLikeSocket(SourceSocket);
}

bool FHexWfcSolver::IsWaterCell(const TMap<FHexAxialCoord, FCellState>& States, const FHexAxialCoord& Coord) const
{
	const FCellState* Cell = States.Find(Coord);
	if (!Cell || Cell->Candidates.Num() != 1)
	{
		return false;
	}

	const FCanalTileVariantKey Variant = Cell->Candidates[0];
	const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Variant.TileIndex);
	if (!Tile)
	{
		return false;
	}

	for (int32 DirectionIndex = 0; DirectionIndex < 6; ++DirectionIndex)
	{
		if (IsWaterLikeSocket(Tile->GetSocket(HexDirectionFromIndex(DirectionIndex), Variant.RotationSteps)))
		{
			return true;
		}
	}
	return false;
}

bool FHexWfcSolver::IsWaterLikeSocket(const ECanalSocketType Socket)
{
	return Socket == ECanalSocketType::Water || Socket == ECanalSocketType::Lock;
}

FHexWfcSolveResult UCanalWfcBlueprintLibrary::SolveHexWfc(
	const UCanalTopologyTileSetAsset* TileSet,
	const FHexWfcGridConfig& Grid,
	const FHexWfcSolveConfig& Config)
{
	FHexWfcSolveResult Result;
	if (!TileSet)
	{
		Result.Message = TEXT("TileSet is null.");
		return Result;
	}

	const FCanalTileCompatibilityTable& Compatibility = TileSet->GetCompatibilityTable();
	if (!Compatibility.IsBuilt())
	{
		Result.Message = TEXT("TileSet compatibility cache is not built. Validate tile definitions first.");
		return Result;
	}

	const FHexWfcSolver Solver(Compatibility);
	return Solver.Solve(Grid, Config);
}
