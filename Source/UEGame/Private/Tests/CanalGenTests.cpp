#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "CanalGen/CanalPrototypeTileSet.h"
#include "CanalGen/CanalTopologyTileTypes.h"
#include "CanalGen/HexGridTypes.h"
#include "CanalGen/HexWfcSolver.h"

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

#endif // WITH_DEV_AUTOMATION_TESTS
