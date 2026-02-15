#include "CanalGen/CanalTopologyGeneratorActor.h"

#include "CanalGen/CanalTopologyTileSetAsset.h"
#include "Algo/Reverse.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACanalTopologyGeneratorActor::ACanalTopologyGeneratorActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WaterInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterInstances"));
	BankInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BankInstances"));
	TowpathInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TowpathInstances"));
	LockInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LockInstances"));
	RoadInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RoadInstances"));
	WaterPathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WaterPathSpline"));

	WaterInstances->SetupAttachment(SceneRoot);
	BankInstances->SetupAttachment(SceneRoot);
	TowpathInstances->SetupAttachment(SceneRoot);
	LockInstances->SetupAttachment(SceneRoot);
	RoadInstances->SetupAttachment(SceneRoot);
	WaterPathSpline->SetupAttachment(SceneRoot);

	WaterPathSpline->SetClosedLoop(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DefaultMesh = CubeMesh.Object;
	}

	GridConfig.Width = 12;
	GridConfig.Height = 8;
	SolveConfig.Seed = 1337;
	SolveConfig.MaxAttempts = 8;
	SolveConfig.MaxPropagationSteps = 100000;
	SolveConfig.bRequireEntryExitPath = true;
	SolveConfig.bRequireSingleWaterComponent = true;
	SolveConfig.bAutoSelectBoundaryPorts = true;
	SolveConfig.bDisallowUnassignedBoundaryWater = true;
	GridLayout.HexSize = 200.0f;
}

void ACanalTopologyGeneratorActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGenerateOnBeginPlay)
	{
		GenerateTopology();
	}
}

void ACanalTopologyGeneratorActor::GenerateTopology()
{
	ClearGenerated();

	FString ValidationError;
	if (!ValidateTileSet(ValidationError))
	{
		UE_LOG(LogTemp, Warning, TEXT("Canal generation aborted: %s"), *ValidationError);
		return;
	}

	RefreshInstanceMeshes();

	LastSolveResult = UCanalWfcBlueprintLibrary::SolveHexWfc(TileSet, GridConfig, SolveConfig);
	if (!LastSolveResult.bSolved)
	{
		UE_LOG(LogTemp, Warning, TEXT("Canal solve failed: %s"), *LastSolveResult.Message);
		return;
	}

	LastGenerationMetadata = FCanalGenerationMetadata();
	if (LastSolveResult.bHasResolvedPorts)
	{
		LastGenerationMetadata.bHasEntryPort = true;
		LastGenerationMetadata.EntryPort = LastSolveResult.ResolvedEntryPort;
		LastGenerationMetadata.bHasExitPort = true;
		LastGenerationMetadata.ExitPort = LastSolveResult.ResolvedExitPort;
	}
	else
	{
		LastGenerationMetadata.bHasEntryPort = SolveConfig.EntryPort.bEnabled;
		LastGenerationMetadata.EntryPort = SolveConfig.EntryPort;
		LastGenerationMetadata.bHasExitPort = SolveConfig.ExitPort.bEnabled;
		LastGenerationMetadata.ExitPort = SolveConfig.ExitPort;
	}

	const FCanalTileCompatibilityTable& Compatibility = TileSet->GetCompatibilityTable();

	for (const FHexWfcCellResult& Cell : LastSolveResult.Cells)
	{
		const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Cell.Variant.TileIndex);
		if (!Tile)
		{
			continue;
		}

		const FVector Center = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord));
		for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
		{
			const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
			const FVector NeighborPos = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord.Neighbor(Direction)));
			const FVector DirectionVec = (NeighborPos - Center).GetSafeNormal();
			const FVector SocketPos = Center + DirectionVec * (GridLayout.HexSize * SocketOffsetScale);
			const FRotator Rotation = DirectionVec.Rotation();

			const ECanalSocketType Socket = Tile->GetSocket(Direction, Cell.Variant.RotationSteps);
			const FTransform InstanceXf(Rotation, SocketPos, InstanceScale);
			AddSocketInstance(Socket, InstanceXf);
		}
	}

	if (bGenerateSpline)
	{
		BuildSplineFromSolvedCells(Compatibility, LastSolveResult.Cells);
	}
	LastGenerationMetadata.SplinePointCount = WaterPathSpline->GetNumberOfSplinePoints();

	if (bDrawPortDebug)
	{
		DrawPortDebug();
	}
	if (bDrawGridDebug)
	{
		DrawGridDebug(Compatibility);
	}

	UE_LOG(LogTemp, Log, TEXT("Canal topology generated: %d cells, attempts=%d"), LastSolveResult.Cells.Num(), LastSolveResult.AttemptsUsed);
}

void ACanalTopologyGeneratorActor::ClearGenerated()
{
	WaterInstances->ClearInstances();
	BankInstances->ClearInstances();
	TowpathInstances->ClearInstances();
	LockInstances->ClearInstances();
	RoadInstances->ClearInstances();

	WaterPathSpline->ClearSplinePoints(false);
	WaterPathSpline->UpdateSpline();

	LastGenerationMetadata = FCanalGenerationMetadata();
}

bool ACanalTopologyGeneratorActor::HasGeneratedSpline() const
{
	return WaterPathSpline->GetNumberOfSplinePoints() >= 2;
}

bool ACanalTopologyGeneratorActor::ValidateTileSet(FString& OutError) const
{
	if (!TileSet)
	{
		OutError = TEXT("TileSet is null.");
		return false;
	}

	if (TileSet->Tiles.Num() == 0)
	{
		OutError = TEXT("TileSet has no tiles.");
		return false;
	}

	return true;
}

void ACanalTopologyGeneratorActor::RefreshInstanceMeshes()
{
	UStaticMesh* Fallback = DefaultMesh.Get();
	WaterInstances->SetStaticMesh(WaterMesh.Get() ? WaterMesh.Get() : Fallback);
	BankInstances->SetStaticMesh(BankMesh.Get() ? BankMesh.Get() : Fallback);
	TowpathInstances->SetStaticMesh(TowpathMesh.Get() ? TowpathMesh.Get() : Fallback);
	LockInstances->SetStaticMesh(LockMesh.Get() ? LockMesh.Get() : Fallback);
	RoadInstances->SetStaticMesh(RoadMesh.Get() ? RoadMesh.Get() : Fallback);
}

void ACanalTopologyGeneratorActor::AddSocketInstance(const ECanalSocketType SocketType, const FTransform& WorldTransform)
{
	switch (SocketType)
	{
	case ECanalSocketType::Water:
		WaterInstances->AddInstance(WorldTransform, true);
		break;
	case ECanalSocketType::Bank:
		BankInstances->AddInstance(WorldTransform, true);
		break;
	case ECanalSocketType::TowpathL:
	case ECanalSocketType::TowpathR:
		TowpathInstances->AddInstance(WorldTransform, true);
		break;
	case ECanalSocketType::Lock:
		LockInstances->AddInstance(WorldTransform, true);
		break;
	case ECanalSocketType::Road:
		RoadInstances->AddInstance(WorldTransform, true);
		break;
	default:
		break;
	}
}

void ACanalTopologyGeneratorActor::BuildSplineFromSolvedCells(
	const FCanalTileCompatibilityTable& Compatibility,
	const TArray<FHexWfcCellResult>& Cells)
{
	TMap<FHexAxialCoord, const FHexWfcCellResult*> CellByCoord;
	CellByCoord.Reserve(Cells.Num());
	for (const FHexWfcCellResult& Cell : Cells)
	{
		CellByCoord.Add(Cell.Coord, &Cell);
	}

	TArray<FHexAxialCoord> WaterCoords;
	for (const FHexWfcCellResult& Cell : Cells)
	{
		if (IsWaterCell(Compatibility, Cell))
		{
			WaterCoords.Add(Cell.Coord);
		}
	}

	if (WaterCoords.Num() < 2)
	{
		return;
	}

	auto FindPath = [&](const FHexAxialCoord& Start, const FHexAxialCoord& Goal, TArray<FHexAxialCoord>& OutPath) -> bool
	{
		TQueue<FHexAxialCoord> Queue;
		TMap<FHexAxialCoord, FHexAxialCoord> Parent;
		TSet<FHexAxialCoord> Visited;

		Queue.Enqueue(Start);
		Visited.Add(Start);

		bool bFound = false;
		while (!Queue.IsEmpty())
		{
			FHexAxialCoord Current;
			Queue.Dequeue(Current);
			if (Current == Goal)
			{
				bFound = true;
				break;
			}

			const FHexWfcCellResult* CurrentCell = CellByCoord.FindRef(Current);
			if (!CurrentCell)
			{
				continue;
			}

			for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
			{
				const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
				const FHexAxialCoord Neighbor = Current.Neighbor(Direction);
				if (Visited.Contains(Neighbor))
				{
					continue;
				}

				const FHexWfcCellResult* NeighborCell = CellByCoord.FindRef(Neighbor);
				if (!NeighborCell)
				{
					continue;
				}

				if (!IsWaterConnection(Compatibility, *CurrentCell, *NeighborCell, Direction))
				{
					continue;
				}

				Visited.Add(Neighbor);
				Parent.Add(Neighbor, Current);
				Queue.Enqueue(Neighbor);
			}
		}

		if (!bFound)
		{
			return false;
		}

		OutPath.Reset();
		FHexAxialCoord Cursor = Goal;
		OutPath.Add(Cursor);
		while (Cursor != Start)
		{
			const FHexAxialCoord* Prev = Parent.Find(Cursor);
			if (!Prev)
			{
				return false;
			}
			Cursor = *Prev;
			OutPath.Add(Cursor);
		}
		Algo::Reverse(OutPath);
		return true;
	};

	FHexAxialCoord Start = WaterCoords[0];
	FHexAxialCoord Farthest = Start;
	int32 BestDistance = -1;
	for (const FHexAxialCoord& Candidate : WaterCoords)
	{
		const int32 Dist = Start.DistanceTo(Candidate);
		if (Dist > BestDistance)
		{
			BestDistance = Dist;
			Farthest = Candidate;
		}
	}

	FHexAxialCoord End = Farthest;
	BestDistance = -1;
	for (const FHexAxialCoord& Candidate : WaterCoords)
	{
		const int32 Dist = Farthest.DistanceTo(Candidate);
		if (Dist > BestDistance)
		{
			BestDistance = Dist;
			End = Candidate;
		}
	}

	if (SolveConfig.EntryPort.bEnabled && SolveConfig.ExitPort.bEnabled &&
		CellByCoord.Contains(SolveConfig.EntryPort.Coord) && CellByCoord.Contains(SolveConfig.ExitPort.Coord))
	{
		Start = SolveConfig.EntryPort.Coord;
		End = SolveConfig.ExitPort.Coord;
	}

	TArray<FHexAxialCoord> Path;
	if (!FindPath(Start, End, Path))
	{
		return;
	}

	WaterPathSpline->ClearSplinePoints(false);
	for (int32 Index = 0; Index < Path.Num(); ++Index)
	{
		const FVector Point = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Path[Index], SplineZOffset));
		WaterPathSpline->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
		WaterPathSpline->SetSplinePointType(Index, ESplinePointType::CurveClamped, false);
	}
	WaterPathSpline->UpdateSpline();
}

FVector ACanalTopologyGeneratorActor::GetBoundaryPortWorldPosition(const FHexBoundaryPort& Port) const
{
	const FVector Center = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Port.Coord, PortDebugZOffset));
	const FVector NeighborPos = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Port.Coord.Neighbor(Port.Direction), PortDebugZOffset));
	const FVector Direction = (NeighborPos - Center).GetSafeNormal();
	return Center + Direction * (GridLayout.HexSize * SocketOffsetScale);
}

void ACanalTopologyGeneratorActor::DrawPortDebug() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (LastGenerationMetadata.bHasEntryPort)
	{
		const FVector Pos = GetBoundaryPortWorldPosition(LastGenerationMetadata.EntryPort);
		DrawDebugSphere(World, Pos, PortDebugRadius, 16, FColor::Green, false, PortDebugDuration);
		DrawDebugString(World, Pos + FVector(0.0f, 0.0f, PortDebugRadius + 10.0f), TEXT("Entry"), nullptr, FColor::Green, PortDebugDuration);
	}

	if (LastGenerationMetadata.bHasExitPort)
	{
		const FVector Pos = GetBoundaryPortWorldPosition(LastGenerationMetadata.ExitPort);
		DrawDebugSphere(World, Pos, PortDebugRadius, 16, FColor::Red, false, PortDebugDuration);
		DrawDebugString(World, Pos + FVector(0.0f, 0.0f, PortDebugRadius + 10.0f), TEXT("Exit"), nullptr, FColor::Red, PortDebugDuration);
	}
}

void ACanalTopologyGeneratorActor::DrawGridDebug(const FCanalTileCompatibilityTable& Compatibility) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const FHexWfcCellResult& Cell : LastSolveResult.Cells)
	{
		const FVector Center = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord));
		const bool bWaterCell = IsWaterCell(Compatibility, Cell);
		const FColor LineColor = bWaterCell ? FColor(64, 180, 255) : FColor(180, 180, 180);

		TArray<FVector> Corners;
		Corners.Reserve(6);
		for (int32 CornerIndex = 0; CornerIndex < 6; ++CornerIndex)
		{
			const float AngleDeg = 60.0f * CornerIndex - 30.0f;
			const float AngleRad = FMath::DegreesToRadians(AngleDeg);
			const FVector LocalOffset(
				GridLayout.HexSize * FMath::Cos(AngleRad),
				GridLayout.HexSize * FMath::Sin(AngleRad),
				0.0f);
			Corners.Add(GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord) + LocalOffset));
		}

		for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
		{
			const FVector& A = Corners[EdgeIndex];
			const FVector& B = Corners[(EdgeIndex + 1) % 6];
			DrawDebugLine(World, A, B, LineColor, false, GridDebugDuration, 0, GridDebugThickness);
		}

		FString Label = FString::Printf(TEXT("(%d,%d)"), Cell.Coord.Q, Cell.Coord.R);
		if (const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Cell.Variant.TileIndex))
		{
			Label = FString::Printf(TEXT("%s r%d"), *Tile->TileId.ToString(), Cell.Variant.RotationSteps);
		}
		DrawDebugString(
			World,
			Center + FVector(0.0f, 0.0f, GridDebugLabelZOffset),
			Label,
			nullptr,
			bWaterCell ? FColor::Cyan : FColor::White,
			GridDebugDuration);
	}
}

bool ACanalTopologyGeneratorActor::IsWaterConnection(
	const FCanalTileCompatibilityTable& Compatibility,
	const FHexWfcCellResult& A,
	const FHexWfcCellResult& B,
	const EHexDirection DirectionFromAToB) const
{
	const FCanalTopologyTileDefinition* ATile = Compatibility.GetTileDefinition(A.Variant.TileIndex);
	const FCanalTopologyTileDefinition* BTile = Compatibility.GetTileDefinition(B.Variant.TileIndex);
	if (!ATile || !BTile)
	{
		return false;
	}

	const ECanalSocketType AOut = ATile->GetSocket(DirectionFromAToB, A.Variant.RotationSteps);
	const ECanalSocketType BIn = BTile->GetSocket(OppositeHexDirection(DirectionFromAToB), B.Variant.RotationSteps);
	return AOut == BIn && IsWaterLikeSocket(AOut);
}

bool ACanalTopologyGeneratorActor::IsWaterCell(const FCanalTileCompatibilityTable& Compatibility, const FHexWfcCellResult& Cell) const
{
	const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Cell.Variant.TileIndex);
	if (!Tile)
	{
		return false;
	}

	for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
	{
		if (IsWaterLikeSocket(Tile->GetSocket(HexDirectionFromIndex(DirIndex), Cell.Variant.RotationSteps)))
		{
			return true;
		}
	}
	return false;
}

bool ACanalTopologyGeneratorActor::IsWaterLikeSocket(const ECanalSocketType Socket)
{
	return Socket == ECanalSocketType::Water || Socket == ECanalSocketType::Lock;
}
