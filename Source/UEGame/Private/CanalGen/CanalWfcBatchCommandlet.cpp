#include "CanalGen/CanalWfcBatchCommandlet.h"

#include "CanalGen/CanalPrototypeTileSet.h"
#include "CanalGen/CanalTopologyTileSetAsset.h"
#include "CanalGen/HexWfcSolver.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

UCanalWfcBatchCommandlet::UCanalWfcBatchCommandlet()
{
	LogToConsole = true;
	IsClient = false;
	IsServer = false;
	IsEditor = true;
}

int32 UCanalWfcBatchCommandlet::Main(const FString& Params)
{
	int32 GridWidth = 16;
	int32 GridHeight = 12;
	int32 StartSeed = 1;
	int32 NumSeeds = 1000;
	int32 MaxAttempts = 8;
	int32 MaxPropagationSteps = 100000;
	float MaxSolveTimeSeconds = 0.0f;
	float MaxBatchTimeSeconds = 0.0f;
	FString OutputDir = FPaths::ProjectSavedDir() / TEXT("BatchReports");
	FString OutputPrefix = TEXT("wfc_batch");
	FString BiomeProfileString = TEXT("default");
	bool bRequireEntryExitPath = true;
	bool bRequireSingleWaterComponent = true;
	bool bAutoSelectBoundaryPorts = true;
	bool bDisallowUnassignedBoundaryWater = true;

	FParse::Value(*Params, TEXT("GridWidth="), GridWidth);
	FParse::Value(*Params, TEXT("GridHeight="), GridHeight);
	FParse::Value(*Params, TEXT("StartSeed="), StartSeed);
	FParse::Value(*Params, TEXT("NumSeeds="), NumSeeds);
	FParse::Value(*Params, TEXT("MaxAttempts="), MaxAttempts);
	FParse::Value(*Params, TEXT("MaxPropagationSteps="), MaxPropagationSteps);
	FParse::Value(*Params, TEXT("MaxSolveTimeSeconds="), MaxSolveTimeSeconds);
	FParse::Value(*Params, TEXT("MaxBatchTimeSeconds="), MaxBatchTimeSeconds);
	FParse::Value(*Params, TEXT("OutputDir="), OutputDir);
	FParse::Value(*Params, TEXT("OutputPrefix="), OutputPrefix);
	FParse::Value(*Params, TEXT("BiomeProfile="), BiomeProfileString);
	FParse::Bool(*Params, TEXT("RequireEntryExitPath="), bRequireEntryExitPath);
	FParse::Bool(*Params, TEXT("RequireSingleWaterComponent="), bRequireSingleWaterComponent);
	FParse::Bool(*Params, TEXT("AutoSelectBoundaryPorts="), bAutoSelectBoundaryPorts);
	FParse::Bool(*Params, TEXT("DisallowUnassignedBoundaryWater="), bDisallowUnassignedBoundaryWater);

	if (GridWidth <= 0 || GridHeight <= 0 || NumSeeds <= 0 || MaxAttempts <= 0 || MaxPropagationSteps <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid parameters. Require positive GridWidth/GridHeight/NumSeeds/MaxAttempts/MaxPropagationSteps."));
		return 1;
	}

	UCanalTopologyTileSetAsset* TileSetAsset = NewObject<UCanalTopologyTileSetAsset>(GetTransientPackage());
	TileSetAsset->Tiles = FCanalPrototypeTileSet::BuildV0();

	FString BuildError;
	if (!TileSetAsset->BuildCompatibilityCache(BuildError))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to build prototype tile compatibility: %s"), *BuildError);
		return 2;
	}

	FHexWfcGridConfig GridConfig;
	GridConfig.Width = GridWidth;
	GridConfig.Height = GridHeight;

	FHexWfcSolveConfig SolveConfig;
	SolveConfig.Seed = StartSeed;
	SolveConfig.MaxAttempts = MaxAttempts;
	SolveConfig.MaxPropagationSteps = MaxPropagationSteps;
	SolveConfig.MaxSolveTimeSeconds = FMath::Max(0.0f, MaxSolveTimeSeconds);
	SolveConfig.bRequireEntryExitPath = bRequireEntryExitPath;
	SolveConfig.bRequireSingleWaterComponent = bRequireSingleWaterComponent;
	SolveConfig.bAutoSelectBoundaryPorts = bAutoSelectBoundaryPorts;
	SolveConfig.bDisallowUnassignedBoundaryWater = bDisallowUnassignedBoundaryWater;
	SolveConfig.BiomeProfile = FName(*BiomeProfileString);

	FHexWfcBatchConfig BatchConfig;
	BatchConfig.StartSeed = StartSeed;
	BatchConfig.NumSeeds = NumSeeds;
	BatchConfig.MaxBatchTimeSeconds = FMath::Max(0.0f, MaxBatchTimeSeconds);

	const FHexWfcBatchStats Stats = UCanalWfcBlueprintLibrary::RunHexWfcBatch(TileSetAsset, GridConfig, SolveConfig, BatchConfig);

	IFileManager::Get().MakeDirectory(*OutputDir, true);

	const FString Timestamp = FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
	const FString BasePath = OutputDir / FString::Printf(TEXT("%s_%s"), *OutputPrefix, *Timestamp);
	const FString JsonPath = BasePath + TEXT(".json");
	const FString CsvPath = BasePath + TEXT(".csv");

	FString Json;
	Json += TEXT("{\n");
	Json += FString::Printf(TEXT("  \"grid_width\": %d,\n"), GridConfig.Width);
	Json += FString::Printf(TEXT("  \"grid_height\": %d,\n"), GridConfig.Height);
	Json += FString::Printf(TEXT("  \"start_seed\": %d,\n"), BatchConfig.StartSeed);
	Json += FString::Printf(TEXT("  \"num_seeds_requested\": %d,\n"), Stats.NumSeedsRequested);
	Json += FString::Printf(TEXT("  \"num_seeds_processed\": %d,\n"), Stats.NumSeedsProcessed);
	Json += FString::Printf(TEXT("  \"num_solved\": %d,\n"), Stats.NumSolved);
	Json += FString::Printf(TEXT("  \"num_failed\": %d,\n"), Stats.NumFailed);
	Json += FString::Printf(TEXT("  \"num_contradictions\": %d,\n"), Stats.NumContradictions);
	Json += FString::Printf(TEXT("  \"num_time_budget_exceeded\": %d,\n"), Stats.NumTimeBudgetExceeded);
	Json += FString::Printf(TEXT("  \"num_single_component_failures\": %d,\n"), Stats.NumSingleWaterComponentFailures);
	Json += FString::Printf(TEXT("  \"contradiction_rate\": %.6f,\n"), Stats.ContradictionRate);
	Json += FString::Printf(TEXT("  \"average_attempts_used\": %.6f,\n"), Stats.AverageAttemptsUsed);
	Json += FString::Printf(TEXT("  \"average_solve_time_seconds\": %.6f,\n"), Stats.AverageSolveTimeSeconds);
	Json += FString::Printf(TEXT("  \"elapsed_batch_time_seconds\": %.6f,\n"), Stats.ElapsedBatchTimeSeconds);
	Json += FString::Printf(TEXT("  \"batch_time_limit_exceeded\": %s,\n"), Stats.bBatchTimeLimitExceeded ? TEXT("true") : TEXT("false"));
	Json += FString::Printf(TEXT("  \"biome_profile\": \"%s\",\n"), *SolveConfig.BiomeProfile.ToString());
	Json += FString::Printf(TEXT("  \"require_entry_exit_path\": %s,\n"), SolveConfig.bRequireEntryExitPath ? TEXT("true") : TEXT("false"));
	Json += FString::Printf(TEXT("  \"require_single_water_component\": %s,\n"), SolveConfig.bRequireSingleWaterComponent ? TEXT("true") : TEXT("false"));
	Json += FString::Printf(TEXT("  \"auto_select_boundary_ports\": %s,\n"), SolveConfig.bAutoSelectBoundaryPorts ? TEXT("true") : TEXT("false"));
	Json += FString::Printf(TEXT("  \"disallow_unassigned_boundary_water\": %s,\n"), SolveConfig.bDisallowUnassignedBoundaryWater ? TEXT("true") : TEXT("false"));

	Json += TEXT("  \"attempt_histogram\": [\n");
	for (int32 Index = 0; Index < Stats.AttemptHistogram.Num(); ++Index)
	{
		const FHexWfcAttemptHistogramBin& Bin = Stats.AttemptHistogram[Index];
		Json += FString::Printf(TEXT("    {\"attempts\": %d, \"count\": %d}%s\n"), Bin.Attempts, Bin.Count, (Index + 1 < Stats.AttemptHistogram.Num()) ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	Json += TEXT("  \"tile_histogram\": [\n");
	for (int32 Index = 0; Index < Stats.TileHistogram.Num(); ++Index)
	{
		const FHexWfcTileHistogramBin& Bin = Stats.TileHistogram[Index];
		Json += FString::Printf(
			TEXT("    {\"tile_id\": \"%s\", \"count\": %d, \"fraction\": %.6f}%s\n"),
			*Bin.TileId.ToString(),
			Bin.Count,
			Bin.Fraction,
			(Index + 1 < Stats.TileHistogram.Num()) ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ]\n");
	Json += TEXT("}\n");

	FString Csv;
	Csv += TEXT("metric,value\n");
	Csv += FString::Printf(TEXT("grid_width,%d\n"), GridConfig.Width);
	Csv += FString::Printf(TEXT("grid_height,%d\n"), GridConfig.Height);
	Csv += FString::Printf(TEXT("start_seed,%d\n"), BatchConfig.StartSeed);
	Csv += FString::Printf(TEXT("num_seeds_requested,%d\n"), Stats.NumSeedsRequested);
	Csv += FString::Printf(TEXT("num_seeds_processed,%d\n"), Stats.NumSeedsProcessed);
	Csv += FString::Printf(TEXT("num_solved,%d\n"), Stats.NumSolved);
	Csv += FString::Printf(TEXT("num_failed,%d\n"), Stats.NumFailed);
	Csv += FString::Printf(TEXT("num_contradictions,%d\n"), Stats.NumContradictions);
	Csv += FString::Printf(TEXT("num_time_budget_exceeded,%d\n"), Stats.NumTimeBudgetExceeded);
	Csv += FString::Printf(TEXT("num_single_component_failures,%d\n"), Stats.NumSingleWaterComponentFailures);
	Csv += FString::Printf(TEXT("contradiction_rate,%.6f\n"), Stats.ContradictionRate);
	Csv += FString::Printf(TEXT("average_attempts_used,%.6f\n"), Stats.AverageAttemptsUsed);
	Csv += FString::Printf(TEXT("average_solve_time_seconds,%.6f\n"), Stats.AverageSolveTimeSeconds);
	Csv += FString::Printf(TEXT("elapsed_batch_time_seconds,%.6f\n"), Stats.ElapsedBatchTimeSeconds);
	Csv += FString::Printf(TEXT("batch_time_limit_exceeded,%s\n"), Stats.bBatchTimeLimitExceeded ? TEXT("true") : TEXT("false"));
	Csv += FString::Printf(TEXT("biome_profile,%s\n"), *SolveConfig.BiomeProfile.ToString());
	Csv += FString::Printf(TEXT("require_entry_exit_path,%s\n"), SolveConfig.bRequireEntryExitPath ? TEXT("true") : TEXT("false"));
	Csv += FString::Printf(TEXT("require_single_water_component,%s\n"), SolveConfig.bRequireSingleWaterComponent ? TEXT("true") : TEXT("false"));
	Csv += FString::Printf(TEXT("auto_select_boundary_ports,%s\n"), SolveConfig.bAutoSelectBoundaryPorts ? TEXT("true") : TEXT("false"));
	Csv += FString::Printf(TEXT("disallow_unassigned_boundary_water,%s\n"), SolveConfig.bDisallowUnassignedBoundaryWater ? TEXT("true") : TEXT("false"));

	Csv += TEXT("\n");
	Csv += TEXT("attempts,count\n");
	for (const FHexWfcAttemptHistogramBin& Bin : Stats.AttemptHistogram)
	{
		Csv += FString::Printf(TEXT("%d,%d\n"), Bin.Attempts, Bin.Count);
	}

	Csv += TEXT("\n");
	Csv += TEXT("tile_id,count,fraction\n");
	for (const FHexWfcTileHistogramBin& Bin : Stats.TileHistogram)
	{
		Csv += FString::Printf(TEXT("%s,%d,%.6f\n"), *Bin.TileId.ToString(), Bin.Count, Bin.Fraction);
	}

	if (!FFileHelper::SaveStringToFile(Json, *JsonPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write JSON report: %s"), *JsonPath);
		return 3;
	}

	if (!FFileHelper::SaveStringToFile(Csv, *CsvPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write CSV report: %s"), *CsvPath);
		return 4;
	}

	UE_LOG(LogTemp, Display, TEXT("WFC batch complete: solved=%d/%d contradictions=%d"), Stats.NumSolved, Stats.NumSeedsProcessed, Stats.NumContradictions);
	UE_LOG(LogTemp, Display, TEXT("Reports written: %s and %s"), *JsonPath, *CsvPath);

	return 0;
}
