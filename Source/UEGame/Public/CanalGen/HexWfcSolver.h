#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CanalGen/CanalTopologyTileSetAsset.h"
#include "HexWfcSolver.generated.h"

USTRUCT(BlueprintType)
struct UEGAME_API FHexWfcGridConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC", meta = (ClampMin = "1"))
	int32 Width = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC", meta = (ClampMin = "1"))
	int32 Height = 8;

	bool EnsureValid(FString& OutError) const;
	bool Contains(const FHexAxialCoord& Coord) const;
};

USTRUCT(BlueprintType)
struct UEGAME_API FHexBoundaryPort
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	FHexAxialCoord Coord;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	EHexDirection Direction = EHexDirection::East;
};

USTRUCT(BlueprintType)
struct UEGAME_API FHexWfcSolveConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC", meta = (ClampMin = "1"))
	int32 MaxAttempts = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC", meta = (ClampMin = "1"))
	int32 MaxPropagationSteps = 100000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	bool bRequireEntryExitPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	bool bRequireSingleWaterComponent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	FHexBoundaryPort EntryPort;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	FHexBoundaryPort ExitPort;

	// If true and Entry/Exit are required but not fully specified, pick a valid pair from boundary water sockets.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	bool bAutoSelectBoundaryPorts = true;

	// If true, reject solutions with boundary water sockets that are not an explicit port and not allowed by tile definition.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|WFC")
	bool bDisallowUnassignedBoundaryWater = true;
};

USTRUCT(BlueprintType)
struct UEGAME_API FHexWfcCellResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	FHexAxialCoord Coord;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	FCanalTileVariantRef Variant;
};

USTRUCT(BlueprintType)
struct UEGAME_API FHexWfcSolveResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	bool bSolved = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	bool bContradiction = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	int32 AttemptsUsed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	int32 CollapsedCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	int32 TotalCells = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	int32 PropagationSteps = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	TArray<FHexWfcCellResult> Cells;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	bool bHasResolvedPorts = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	FHexBoundaryPort ResolvedEntryPort;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|WFC")
	FHexBoundaryPort ResolvedExitPort;
};

class UEGAME_API FHexWfcSolver
{
public:
	explicit FHexWfcSolver(const FCanalTileCompatibilityTable& InCompatibility);

	FHexWfcSolveResult Solve(const FHexWfcGridConfig& Grid, const FHexWfcSolveConfig& Config) const;

private:
	struct FCellState
	{
		TArray<FCanalTileVariantKey> Candidates;

		bool IsCollapsed() const
		{
			return Candidates.Num() == 1;
		}
	};

	bool SelectLowestEntropyCell(
		const TMap<FHexAxialCoord, FCellState>& States,
		FHexAxialCoord& OutCoord,
		int32& OutEntropy) const;

	FCanalTileVariantKey ChooseVariant(
		const TArray<FCanalTileVariantKey>& Candidates,
		FRandomStream& Random) const;

	bool IsVariantAllowedByAnySource(
		const FCanalTileVariantKey& Candidate,
		const TArray<FCanalTileVariantKey>& SourceCandidates,
		EHexDirection SourceToTargetDirection) const;

	bool ValidateSolvedState(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid,
		const FHexWfcSolveConfig& Config,
		FHexBoundaryPort& OutResolvedEntry,
		FHexBoundaryPort& OutResolvedExit,
		FString& OutError) const;

	bool ResolveBoundaryPorts(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid,
		const FHexWfcSolveConfig& Config,
		FHexBoundaryPort& OutEntry,
		FHexBoundaryPort& OutExit,
		FString& OutError) const;

	bool ValidateBoundarySockets(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid,
		const FHexBoundaryPort& Entry,
		const FHexBoundaryPort& Exit,
		FString& OutError) const;

	void CollectBoundaryWaterSockets(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid,
		bool bRequireExplicitBoundaryFlag,
		TArray<FHexBoundaryPort>& OutPorts) const;

	static bool IsSamePort(const FHexBoundaryPort& A, const FHexBoundaryPort& B);

	bool ValidateBoundaryPort(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid,
		const FHexBoundaryPort& Port,
		FString& OutError) const;

	bool HasEntryExitPath(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid,
		const FHexBoundaryPort& Entry,
		const FHexBoundaryPort& Exit) const;

	bool HasSingleWaterComponent(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexWfcGridConfig& Grid) const;

	bool AreCellsWaterConnected(
		const TMap<FHexAxialCoord, FCellState>& States,
		const FHexAxialCoord& Source,
		const FHexAxialCoord& Target,
		EHexDirection Direction) const;

	bool IsWaterCell(const TMap<FHexAxialCoord, FCellState>& States, const FHexAxialCoord& Coord) const;

	static bool IsWaterLikeSocket(ECanalSocketType Socket);

	const FCanalTileCompatibilityTable& Compatibility;
};

UCLASS()
class UEGAME_API UCanalWfcBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Canal|WFC")
	static FHexWfcSolveResult SolveHexWfc(
		const UCanalTopologyTileSetAsset* TileSet,
		const FHexWfcGridConfig& Grid,
		const FHexWfcSolveConfig& Config);
};
