#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CanalGen/CanalPrototypeTileSet.h"
#include "CanalGen/CanalScenarioInterface.h"
#include "CanalGen/CanalTopologyGeneratorActor.h"
#include "CanalGen/CanalTopologyTileSetAsset.h"
#include "CanalGen/CanalTopologyTileTypes.h"
#include "CanalGen/HexGridTypes.h"
#include "CanalGen/HexWfcSolver.h"

namespace
{
	UCanalTopologyTileSetAsset* BuildPrototypeTileSetAsset(FAutomationTestBase& Test)
	{
		UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
		if (!TileSetAsset)
		{
			Test.AddError(TEXT("Failed to allocate transient tile set asset."));
			return nullptr;
		}

		TileSetAsset->Tiles = FCanalPrototypeTileSet::BuildV0();
		FString Error;
		if (!TileSetAsset->BuildCompatibilityCache(Error))
		{
			Test.AddError(FString::Printf(TEXT("Failed to build prototype compatibility cache: %s"), *Error));
			return nullptr;
		}

		return TileSetAsset;
	}

	UCanalTopologyTileSetAsset* BuildFullWaterTileSetAsset(FAutomationTestBase& Test)
	{
		FCanalTopologyTileDefinition FullWaterTile;
		FullWaterTile.TileId = TEXT("all_water_m1_spline");
		FullWaterTile.Sockets = {
			ECanalSocketType::Water,
			ECanalSocketType::Water,
			ECanalSocketType::Water,
			ECanalSocketType::Water,
			ECanalSocketType::Water,
			ECanalSocketType::Water};
		FullWaterTile.Weight = 1.0f;
		FullWaterTile.bAllowAsBoundaryPort = true;

		UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
		if (!TileSetAsset)
		{
			Test.AddError(TEXT("Failed to allocate transient full-water tile set asset."));
			return nullptr;
		}

		TileSetAsset->Tiles = {FullWaterTile};
		FString Error;
		if (!TileSetAsset->BuildCompatibilityCache(Error))
		{
			Test.AddError(FString::Printf(TEXT("Failed to build full-water compatibility cache: %s"), *Error));
			return nullptr;
		}

		return TileSetAsset;
	}

	FHexWfcSolveConfig MakeM1RelaxedSolveConfig()
	{
		FHexWfcSolveConfig Config;
		Config.MaxAttempts = 8;
		Config.MaxPropagationSteps = 100000;
		Config.bRequireEntryExitPath = false;
		Config.bRequireSingleWaterComponent = false;
		Config.bAutoSelectBoundaryPorts = true;
		Config.bDisallowUnassignedBoundaryWater = false;
		return Config;
	}

	bool ValidateSolvedAdjacency(
		FAutomationTestBase& Test,
		const FCanalTileCompatibilityTable& Compatibility,
		const TArray<FHexWfcCellResult>& Cells)
	{
		for (const FHexWfcCellResult& Cell : Cells)
		{
			const FCanalTopologyTileDefinition* TileA = Compatibility.GetTileDefinition(Cell.Variant.TileIndex);
			if (!TileA)
			{
				Test.AddError(FString::Printf(TEXT("Missing tile definition for variant index %d."), Cell.Variant.TileIndex));
				return false;
			}

			for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
			{
				const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
				const FHexAxialCoord NeighborCoord = Cell.Coord.Neighbor(Direction);
				const FHexWfcCellResult* NeighborCell = Cells.FindByPredicate([&NeighborCoord](const FHexWfcCellResult& Candidate)
				{
					return Candidate.Coord == NeighborCoord;
				});
				if (!NeighborCell)
				{
					continue;
				}

				const FCanalTopologyTileDefinition* TileB = Compatibility.GetTileDefinition(NeighborCell->Variant.TileIndex);
				if (!TileB)
				{
					Test.AddError(FString::Printf(TEXT("Missing neighbor tile definition for variant index %d."), NeighborCell->Variant.TileIndex));
					return false;
				}

				const ECanalSocketType SocketA = TileA->GetSocket(Direction, Cell.Variant.RotationSteps);
				const ECanalSocketType SocketB = TileB->GetSocket(OppositeHexDirection(Direction), NeighborCell->Variant.RotationSteps);
				if (SocketA != SocketB)
				{
					Test.AddError(
						FString::Printf(
							TEXT("Socket mismatch at cell (%d,%d) toward direction %d."),
							Cell.Coord.Q,
							Cell.Coord.R,
							DirIndex));
					return false;
				}
			}
		}

		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexCoordDistanceAndNeighborsTest,
	"UEGame.Canal.Hex.DistanceAndNeighbors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexCoordDistanceAndNeighborsTest::RunTest(const FString& Parameters)
{
	const FHexAxialCoord Origin(0, 0);

	for (int32 DirIndex = 0; DirIndex < 6; ++DirIndex)
	{
		const EHexDirection Direction = HexDirectionFromIndex(DirIndex);
		const FHexAxialCoord Neighbor = Origin.Neighbor(Direction);
		TestEqual(TEXT("Every direct neighbor has distance 1."), Origin.DistanceTo(Neighbor), 1);
	}

	const FHexAxialCoord A(2, -1);
	const FHexAxialCoord B(-1, 2);
	TestEqual(TEXT("Distance is symmetric."), A.DistanceTo(B), B.DistanceTo(A));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexLayoutTransformTest,
	"UEGame.Canal.Hex.WorldTransform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexLayoutTransformTest::RunTest(const FString& Parameters)
{
	FHexGridLayout Layout;
	Layout.HexSize = 100.0f;

	const FVector Origin = Layout.AxialToWorld(FHexAxialCoord(0, 0));
	TestEqual(TEXT("Origin maps to world origin X."), static_cast<double>(Origin.X), 0.0);
	TestEqual(TEXT("Origin maps to world origin Y."), static_cast<double>(Origin.Y), 0.0);

	const FVector East = Layout.AxialToWorld(FHexAxialCoord(1, 0));
	TestTrue(TEXT("East X matches sqrt(3)*size."), FMath::IsNearlyEqual(East.X, FMath::Sqrt(3.0f) * 100.0f, 0.001f));
	TestTrue(TEXT("East Y is ~0."), FMath::IsNearlyEqual(East.Y, 0.0f, 0.001f));

	const FVector SouthEast = Layout.AxialToWorld(FHexAxialCoord(0, 1));
	TestTrue(TEXT("SouthEast X is half-step."), FMath::IsNearlyEqual(SouthEast.X, FMath::Sqrt(3.0f) * 50.0f, 0.001f));
	TestTrue(TEXT("SouthEast Y is 1.5*size."), FMath::IsNearlyEqual(SouthEast.Y, 150.0f, 0.001f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalTileCompatibilityBuildTest,
	"UEGame.Canal.Tile.CompatibilityBuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalTileCompatibilityBuildTest::RunTest(const FString& Parameters)
{
	const TArray<FCanalTopologyTileDefinition> TileSet = FCanalPrototypeTileSet::BuildV0();

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	const bool bBuilt = Compatibility.Build(TileSet, &Error);
	TestTrue(FString::Printf(TEXT("Compatibility builds successfully (%s)."), *Error), bBuilt);
	TestTrue(TEXT("Compatibility table has variants."), Compatibility.GetAllVariants().Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalPrototypeTileSetCoverageTest,
	"UEGame.Canal.Tile.PrototypeV0Coverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalPrototypeTileSetCoverageTest::RunTest(const FString& Parameters)
{
	const TArray<FCanalTopologyTileDefinition> Tiles = FCanalPrototypeTileSet::BuildV0();
	TestTrue(TEXT("Prototype v0 tile count should be in 10-20 range."), Tiles.Num() >= 10 && Tiles.Num() <= 20);

	auto HasTile = [&Tiles](const TCHAR* TileId) -> bool
	{
		return Tiles.ContainsByPredicate([&](const FCanalTopologyTileDefinition& Tile)
		{
			return Tile.TileId == FName(TileId);
		});
	};

	TestTrue(TEXT("Has straight tile."), HasTile(TEXT("water_straight_ew")));
	TestTrue(TEXT("Has gentle bend tile."), HasTile(TEXT("water_bend_gentle")));
	TestTrue(TEXT("Has hard bend tile."), HasTile(TEXT("water_bend_hard")));
	TestTrue(TEXT("Has junction tile."), HasTile(TEXT("water_t_junction")));
	TestTrue(TEXT("Has lock boundary tile."), HasTile(TEXT("lock_gate")));

	const FCanalTopologyTileDefinition* LockGate = Tiles.FindByPredicate([](const FCanalTopologyTileDefinition& Tile)
	{
		return Tile.TileId == FName(TEXT("lock_gate"));
	});
	TestTrue(TEXT("lock_gate should allow boundary port usage."), LockGate && LockGate->bAllowAsBoundaryPort);

	for (const FCanalTopologyTileDefinition& Tile : Tiles)
	{
		FString ValidationError;
		const bool bValid = Tile.EnsureValid(ValidationError);
		TestTrue(
			FString::Printf(TEXT("Tile %s should validate (%s)."), *Tile.TileId.ToString(), *ValidationError),
			bValid);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalTileCompatibilityDescribeTest,
	"UEGame.Canal.Tile.CompatibilityDescribe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalTileCompatibilityDescribeTest::RunTest(const FString& Parameters)
{
	UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
	TileSetAsset->Tiles = FCanalPrototypeTileSet::BuildV0();

	FString Error;
	if (!TileSetAsset->BuildCompatibilityCache(Error))
	{
		AddError(FString::Printf(TEXT("Failed to build compatibility cache: %s"), *Error));
		return false;
	}

	const FString Description = TileSetAsset->DescribeCompatibility(0, 0, EHexDirection::East);
	TestTrue(TEXT("Description should include compatibility summary."), Description.Contains(TEXT("compatible variants"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Description should list tile IDs."), Description.Contains(TEXT("tileId"), ESearchCase::IgnoreCase));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcSolveSmokeTest,
	"UEGame.Canal.WFC.SolveSmoke",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcSolveSmokeTest::RunTest(const FString& Parameters)
{
	const TArray<FCanalTopologyTileDefinition> TileSet = FCanalPrototypeTileSet::BuildV0();

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	if (!Compatibility.Build(TileSet, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to build compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 5;
	Grid.Height = 4;

	FHexWfcSolveConfig Config;
	Config.Seed = 12345;
	Config.MaxPropagationSteps = 200000;
	Config.bDisallowUnassignedBoundaryWater = false;

	const FHexWfcSolver Solver(Compatibility);
	const FHexWfcSolveResult Result = Solver.Solve(Grid, Config);

	TestTrue(FString::Printf(TEXT("WFC should solve without contradiction. Message: %s"), *Result.Message), Result.bSolved && !Result.bContradiction);
	TestEqual(TEXT("Collapsed cell count should equal grid area."), Result.CollapsedCells, Grid.Width * Grid.Height);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcEntryExitAndComponentConstraintsTest,
	"UEGame.Canal.WFC.EntryExitAndConnectedWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcEntryExitAndComponentConstraintsTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition FullWaterTile;
	FullWaterTile.TileId = TEXT("all_water");
	FullWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water};
	FullWaterTile.Weight = 1.0f;
	FullWaterTile.bAllowAsBoundaryPort = true;

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	if (!Compatibility.Build({FullWaterTile}, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to build full-water compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 4;
	Grid.Height = 3;

	FHexWfcSolveConfig Config;
	Config.Seed = 42;
	Config.MaxAttempts = 2;
	Config.bRequireEntryExitPath = true;
	Config.bRequireSingleWaterComponent = true;
	Config.EntryPort.bEnabled = true;
	Config.EntryPort.Coord = FHexAxialCoord(0, 1);
	Config.EntryPort.Direction = EHexDirection::West;
	Config.ExitPort.bEnabled = true;
	Config.ExitPort.Coord = FHexAxialCoord(3, 1);
	Config.ExitPort.Direction = EHexDirection::East;

	const FHexWfcSolver Solver(Compatibility);
	const FHexWfcSolveResult Result = Solver.Solve(Grid, Config);
	TestTrue(FString::Printf(TEXT("Constrainted full-water solve should pass. Message: %s"), *Result.Message), Result.bSolved);
	TestFalse(TEXT("Constrainted solve should not report contradiction."), Result.bContradiction);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcBoundarySocketPolicyTest,
	"UEGame.Canal.WFC.BoundarySocketPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcBoundarySocketPolicyTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition NonBoundaryWaterTile;
	NonBoundaryWaterTile.TileId = TEXT("edge_water_not_allowed");
	NonBoundaryWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank};
	NonBoundaryWaterTile.Weight = 1.0f;
	NonBoundaryWaterTile.bAllowAsBoundaryPort = false;

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	if (!Compatibility.Build({NonBoundaryWaterTile}, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to build compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 1;
	Grid.Height = 1;

	FHexWfcSolveConfig Config;
	Config.MaxAttempts = 1;
	Config.bRequireEntryExitPath = false;
	Config.bDisallowUnassignedBoundaryWater = true;

	const FHexWfcSolver Solver(Compatibility);
	const FHexWfcSolveResult Result = Solver.Solve(Grid, Config);
	TestFalse(TEXT("Solver should reject unassigned boundary water sockets."), Result.bSolved);
	TestTrue(TEXT("Failure should report boundary policy."), Result.Message.Contains(TEXT("boundary water socket"), ESearchCase::IgnoreCase));

	NonBoundaryWaterTile.bAllowAsBoundaryPort = true;
	if (!Compatibility.Build({NonBoundaryWaterTile}, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to rebuild compatibility: %s"), *Error));
		return false;
	}

	const FHexWfcSolver ExplicitBoundarySolver(Compatibility);
	const FHexWfcSolveResult ExplicitResult = ExplicitBoundarySolver.Solve(Grid, Config);
	TestTrue(FString::Printf(TEXT("Explicit boundary-allowed tile should solve. Message: %s"), *ExplicitResult.Message), ExplicitResult.bSolved);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcAutoBoundaryPortSelectionTest,
	"UEGame.Canal.WFC.AutoBoundaryPortSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcAutoBoundaryPortSelectionTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition FullWaterTile;
	FullWaterTile.TileId = TEXT("all_water_explicit_boundary");
	FullWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water};
	FullWaterTile.Weight = 1.0f;
	FullWaterTile.bAllowAsBoundaryPort = true;

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	if (!Compatibility.Build({FullWaterTile}, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to build full-water compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 4;
	Grid.Height = 2;

	FHexWfcSolveConfig Config;
	Config.MaxAttempts = 1;
	Config.bRequireEntryExitPath = true;
	Config.bAutoSelectBoundaryPorts = true;
	Config.EntryPort.bEnabled = false;
	Config.ExitPort.bEnabled = false;

	const FHexWfcSolver Solver(Compatibility);
	const FHexWfcSolveResult Result = Solver.Solve(Grid, Config);

	TestTrue(FString::Printf(TEXT("Auto boundary port selection should solve. Message: %s"), *Result.Message), Result.bSolved);
	TestTrue(TEXT("Auto selection should return resolved ports."), Result.bHasResolvedPorts);
	TestTrue(TEXT("Resolved ports should be distinct cells."),
		Result.ResolvedEntryPort.Coord != Result.ResolvedExitPort.Coord);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcSingleComponentValidationFailureTest,
	"UEGame.Canal.WFC.SingleComponentValidationFailure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcSingleComponentValidationFailureTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition IsolatedWaterTile;
	IsolatedWaterTile.TileId = TEXT("isolated_water");
	IsolatedWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank};
	IsolatedWaterTile.Weight = 1.0f;

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	if (!Compatibility.Build({IsolatedWaterTile}, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to build isolated-water compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 1;
	Grid.Height = 2;

	FHexWfcSolveConfig Config;
	Config.MaxAttempts = 2;
	Config.bRequireSingleWaterComponent = true;
	Config.bDisallowUnassignedBoundaryWater = false;

	const FHexWfcSolver Solver(Compatibility);
	const FHexWfcSolveResult Result = Solver.Solve(Grid, Config);
	TestFalse(TEXT("Solver should reject disconnected water components."), Result.bSolved);
	TestTrue(TEXT("Result should classify failure as single-component validation."), Result.bFailedSingleWaterComponent);
	TestTrue(TEXT("Failure message should mention connected component."),
		Result.Message.Contains(TEXT("connected component"), ESearchCase::IgnoreCase));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcBatchSingleComponentFailureStatsTest,
	"UEGame.Canal.WFC.BatchSingleComponentFailureStats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcBatchSingleComponentFailureStatsTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition IsolatedWaterTile;
	IsolatedWaterTile.TileId = TEXT("isolated_water_batch");
	IsolatedWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank};
	IsolatedWaterTile.Weight = 1.0f;

	UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
	TileSetAsset->Tiles = {IsolatedWaterTile};

	FString Error;
	if (!TileSetAsset->BuildCompatibilityCache(Error))
	{
		AddError(FString::Printf(TEXT("Failed to build tile set compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 1;
	Grid.Height = 2;

	FHexWfcSolveConfig Config;
	Config.MaxAttempts = 1;
	Config.bRequireSingleWaterComponent = true;
	Config.bDisallowUnassignedBoundaryWater = false;

	FHexWfcBatchConfig BatchConfig;
	BatchConfig.StartSeed = 1000;
	BatchConfig.NumSeeds = 5;

	const FHexWfcBatchStats Stats = UCanalWfcBlueprintLibrary::RunHexWfcBatch(TileSetAsset, Grid, Config, BatchConfig);
	TestEqual(TEXT("All seeds should be processed."), Stats.NumSeedsProcessed, BatchConfig.NumSeeds);
	TestEqual(TEXT("Solved + failed should match processed seeds."), Stats.NumSolved + Stats.NumFailed, BatchConfig.NumSeeds);
	TestTrue(TEXT("Batch should report at least one single-component validation failure."), Stats.NumSingleWaterComponentFailures > 0);
	TestTrue(TEXT("Single-component failures cannot exceed failed runs."),
		Stats.NumSingleWaterComponentFailures <= Stats.NumFailed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcSolveTimeBudgetTest,
	"UEGame.Canal.WFC.TimeBudget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcSolveTimeBudgetTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition FullWaterTile;
	FullWaterTile.TileId = TEXT("all_water_time_budget");
	FullWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water};
	FullWaterTile.Weight = 1.0f;
	FullWaterTile.bAllowAsBoundaryPort = true;

	FCanalTileCompatibilityTable Compatibility;
	FString Error;
	if (!Compatibility.Build({FullWaterTile}, &Error))
	{
		AddError(FString::Printf(TEXT("Failed to build compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 64;
	Grid.Height = 64;

	FHexWfcSolveConfig Config;
	Config.Seed = 7;
	Config.MaxAttempts = 4;
	Config.MaxSolveTimeSeconds = 0.000001f;
	Config.bDisallowUnassignedBoundaryWater = false;

	const FHexWfcSolver Solver(Compatibility);
	const FHexWfcSolveResult Result = Solver.Solve(Grid, Config);
	TestFalse(TEXT("Solver should stop on tiny time budget."), Result.bSolved);
	TestTrue(TEXT("Result should report time budget exceeded."), Result.bTimeBudgetExceeded);
	TestTrue(TEXT("Failure message should mention time budget."), Result.Message.Contains(TEXT("time budget"), ESearchCase::IgnoreCase));
	TestTrue(TEXT("Solve time should be non-negative."), Result.SolveTimeSeconds >= 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcBatchHarnessStatsTest,
	"UEGame.Canal.WFC.BatchHarnessStats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcBatchHarnessStatsTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition FullWaterTile;
	FullWaterTile.TileId = TEXT("all_water_batch");
	FullWaterTile.Sockets = {
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water,
		ECanalSocketType::Water};
	FullWaterTile.Weight = 1.0f;
	FullWaterTile.bAllowAsBoundaryPort = true;

	UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
	TileSetAsset->Tiles = {FullWaterTile};

	FString Error;
	if (!TileSetAsset->BuildCompatibilityCache(Error))
	{
		AddError(FString::Printf(TEXT("Failed to build tile set compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 3;
	Grid.Height = 2;

	FHexWfcSolveConfig Config;
	Config.MaxAttempts = 1;
	Config.bDisallowUnassignedBoundaryWater = false;

	FHexWfcBatchConfig BatchConfig;
	BatchConfig.StartSeed = 100;
	BatchConfig.NumSeeds = 6;

	const FHexWfcBatchStats Stats = UCanalWfcBlueprintLibrary::RunHexWfcBatch(TileSetAsset, Grid, Config, BatchConfig);
	TestEqual(TEXT("Batch should process all requested seeds."), Stats.NumSeedsProcessed, BatchConfig.NumSeeds);
	TestEqual(TEXT("All batch runs should solve with full-water tile."), Stats.NumSolved, BatchConfig.NumSeeds);
	TestEqual(TEXT("No failed runs expected."), Stats.NumFailed, 0);

	const FHexWfcAttemptHistogramBin* AttemptBin = Stats.AttemptHistogram.FindByPredicate([](const FHexWfcAttemptHistogramBin& Bin)
	{
		return Bin.Attempts == 1;
	});
	TestTrue(TEXT("Histogram should include attempt=1 bin."), AttemptBin != nullptr);
	TestEqual(TEXT("attempt=1 count should match seeds."), AttemptBin ? AttemptBin->Count : -1, BatchConfig.NumSeeds);

	const FHexWfcTileHistogramBin* TileBin = Stats.TileHistogram.FindByPredicate([](const FHexWfcTileHistogramBin& Bin)
	{
		return Bin.TileId == FName(TEXT("all_water_batch"));
	});
	const int32 ExpectedTileCount = Grid.Width * Grid.Height * BatchConfig.NumSeeds;
	TestTrue(TEXT("Tile histogram should include generated tile ID."), TileBin != nullptr);
	TestEqual(TEXT("Tile histogram count should match solved cells."), TileBin ? TileBin->Count : -1, ExpectedTileCount);
	TestTrue(TEXT("Single-tile histogram fraction should be 1."), TileBin && FMath::IsNearlyEqual(TileBin->Fraction, 1.0f, KINDA_SMALL_NUMBER));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalScenarioMetadataSetterTest,
	"UEGame.Canal.Scenario.MetadataSetter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalScenarioMetadataSetterTest::RunTest(const FString& Parameters)
{
	ACanalTopologyGeneratorActor* Generator = NewObject<ACanalTopologyGeneratorActor>(GetTransientPackage());
	if (!Generator)
	{
		AddError(TEXT("Failed to allocate topology generator actor."));
		return false;
	}

	Generator->SetScenarioMetadata(TEXT("CaptureRun"), 45.5f);
	TestEqual(TEXT("Scenario name should be recorded to generation metadata."), Generator->LastGenerationMetadata.ScenarioName, FName(TEXT("CaptureRun")));
	TestTrue(TEXT("Scenario duration should be recorded to generation metadata."), FMath::IsNearlyEqual(Generator->LastGenerationMetadata.ScenarioDurationSeconds, 45.5f));

	FCanalScenarioRequest Request;
	TestTrue(TEXT("Scenario request defaults to generated spline usage."), Request.bUseGeneratedWaterSpline);
	TestTrue(TEXT("Scenario request default duration should be positive."), Request.RequestedDurationSeconds > 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHexWfcBiomeWeightMultipliersTest,
	"UEGame.Canal.WFC.BiomeWeightMultipliers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHexWfcBiomeWeightMultipliersTest::RunTest(const FString& Parameters)
{
	FCanalTopologyTileDefinition TileA;
	TileA.TileId = TEXT("tile_a");
	TileA.Sockets = {
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank,
		ECanalSocketType::Bank};
	TileA.Weight = 1.0f;

	FCanalTopologyTileDefinition TileB = TileA;
	TileB.TileId = TEXT("tile_b");

	UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
	TileSetAsset->Tiles = {TileA, TileB};

	FString Error;
	if (!TileSetAsset->BuildCompatibilityCache(Error))
	{
		AddError(FString::Printf(TEXT("Failed to build tile set compatibility: %s"), *Error));
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 1;
	Grid.Height = 1;

	FHexWfcSolveConfig Config;
	Config.MaxAttempts = 1;
	Config.BiomeProfile = TEXT("test_bias");
	Config.bDisallowUnassignedBoundaryWater = true;

	FHexTileWeightMultiplier DisableA;
	DisableA.TileId = TileA.TileId;
	DisableA.Multiplier = 0.0f;
	Config.BiomeWeightMultipliers = {DisableA};

	FHexWfcBatchConfig BatchConfig;
	BatchConfig.StartSeed = 1;
	BatchConfig.NumSeeds = 48;

	const FHexWfcBatchStats Stats = UCanalWfcBlueprintLibrary::RunHexWfcBatch(TileSetAsset, Grid, Config, BatchConfig);
	TestEqual(TEXT("Batch should solve all biased runs."), Stats.NumSolved, BatchConfig.NumSeeds);

	const FHexWfcTileHistogramBin* TileABin = Stats.TileHistogram.FindByPredicate([](const FHexWfcTileHistogramBin& Bin)
	{
		return Bin.TileId == FName(TEXT("tile_a"));
	});
	const FHexWfcTileHistogramBin* TileBBin = Stats.TileHistogram.FindByPredicate([](const FHexWfcTileHistogramBin& Bin)
	{
		return Bin.TileId == FName(TEXT("tile_b"));
	});

	TestTrue(TEXT("Disabled tile_a should not be selected."), TileABin == nullptr || TileABin->Count == 0);
	TestTrue(TEXT("Biased tile_b should be selected for every cell."), TileBBin != nullptr && TileBBin->Count == BatchConfig.NumSeeds);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalM1Reliability128RelaxedTest,
	"UEGame.Canal.M1.Reliability128Relaxed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalM1Reliability128RelaxedTest::RunTest(const FString& Parameters)
{
	UCanalTopologyTileSetAsset* TileSetAsset = BuildPrototypeTileSetAsset(*this);
	if (!TileSetAsset)
	{
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 16;
	Grid.Height = 8;

	FHexWfcSolveConfig Config = MakeM1RelaxedSolveConfig();

	FHexWfcBatchConfig BatchConfig;
	BatchConfig.StartSeed = 2000;
	BatchConfig.NumSeeds = 20;

	const FHexWfcBatchStats Stats = UCanalWfcBlueprintLibrary::RunHexWfcBatch(TileSetAsset, Grid, Config, BatchConfig);
	const double SolveRate = Stats.NumSeedsProcessed > 0
		? static_cast<double>(Stats.NumSolved) / static_cast<double>(Stats.NumSeedsProcessed)
		: 0.0;
	TestTrue(
		FString::Printf(TEXT("128-hex solve rate should be >= 90%% (got %.2f%%)."), SolveRate * 100.0),
		SolveRate >= 0.90);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalM1Reliability512RelaxedTest,
	"UEGame.Canal.M1.Reliability512Relaxed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalM1Reliability512RelaxedTest::RunTest(const FString& Parameters)
{
	UCanalTopologyTileSetAsset* TileSetAsset = BuildPrototypeTileSetAsset(*this);
	if (!TileSetAsset)
	{
		return false;
	}

	FHexWfcGridConfig Grid;
	Grid.Width = 32;
	Grid.Height = 16;

	FHexWfcSolveConfig Config = MakeM1RelaxedSolveConfig();

	FHexWfcBatchConfig BatchConfig;
	BatchConfig.StartSeed = 5000;
	BatchConfig.NumSeeds = 10;

	const FHexWfcBatchStats Stats = UCanalWfcBlueprintLibrary::RunHexWfcBatch(TileSetAsset, Grid, Config, BatchConfig);
	const double SolveRate = Stats.NumSeedsProcessed > 0
		? static_cast<double>(Stats.NumSolved) / static_cast<double>(Stats.NumSeedsProcessed)
		: 0.0;
	TestTrue(
		FString::Printf(TEXT("512-hex solve rate should be >= 90%% (got %.2f%%)."), SolveRate * 100.0),
		SolveRate >= 0.90);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCanalM1SplineAndSocketSmokeTest,
	"UEGame.Canal.M1.SplineAndSockets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanalM1SplineAndSocketSmokeTest::RunTest(const FString& Parameters)
{
	UCanalTopologyTileSetAsset* TileSetAsset = BuildFullWaterTileSetAsset(*this);
	if (!TileSetAsset)
	{
		return false;
	}

	ACanalTopologyGeneratorActor* Generator = NewObject<ACanalTopologyGeneratorActor>(GetTransientPackage());
	if (!Generator)
	{
		AddError(TEXT("Failed to allocate topology generator actor."));
		return false;
	}

	Generator->TileSet = TileSetAsset;
	Generator->GridConfig.Width = 16;
	Generator->GridConfig.Height = 8;
	Generator->SolveConfig = FHexWfcSolveConfig();
	Generator->SolveConfig.Seed = 42;
	Generator->SolveConfig.MaxAttempts = 2;
	Generator->SolveConfig.bRequireEntryExitPath = true;
	Generator->SolveConfig.bRequireSingleWaterComponent = true;
	Generator->SolveConfig.bAutoSelectBoundaryPorts = false;
	Generator->SolveConfig.EntryPort.bEnabled = true;
	Generator->SolveConfig.EntryPort.Coord = FHexAxialCoord(0, 4);
	Generator->SolveConfig.EntryPort.Direction = EHexDirection::West;
	Generator->SolveConfig.ExitPort.bEnabled = true;
	Generator->SolveConfig.ExitPort.Coord = FHexAxialCoord(15, 4);
	Generator->SolveConfig.ExitPort.Direction = EHexDirection::East;
	Generator->bGenerateSpline = true;

	Generator->GenerateTopology();
	TestTrue(TEXT("Generator should solve in relaxed M1 profile."), Generator->LastSolveResult.bSolved);
	TestTrue(TEXT("Generator should produce a usable spline path."), Generator->HasGeneratedSpline());

	TArray<FVector> SplinePoints;
	Generator->GetGeneratedSplinePoints(SplinePoints, true);
	TestTrue(TEXT("Spline should contain at least two points."), SplinePoints.Num() >= 2);

	const bool bAdjacencyValid = ValidateSolvedAdjacency(
		*this,
		TileSetAsset->GetCompatibilityTable(),
		Generator->LastSolveResult.Cells);
	TestTrue(TEXT("All neighboring sockets should match in solved output."), bAdjacencyValid);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
