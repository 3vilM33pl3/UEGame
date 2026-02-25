#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CanalGen/CanalTopologyTileTypes.h"
#include "CanalGen/HexWfcSolver.h"
#include "CanalTopologyGeneratorActor.generated.h"

class UCanalTopologyTileSetAsset;
class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class USplineComponent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct UEGAME_API FCanalGenerationMetadata
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	int32 MasterSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	int32 TopologySeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	int32 DressingSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	FName BiomeProfile = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	bool bHasEntryPort = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	FHexBoundaryPort EntryPort;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	bool bHasExitPort = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	FHexBoundaryPort ExitPort;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	int32 SplinePointCount = 0;
};

UCLASS(BlueprintType, Blueprintable)
class UEGAME_API ACanalTopologyGeneratorActor : public AActor
{
	GENERATED_BODY()

public:
	ACanalTopologyGeneratorActor();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Canal|Generation")
	void GenerateTopology();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Canal|Generation")
	void ClearGenerated();

	UFUNCTION(BlueprintPure, Category = "Canal|Generation")
	bool HasGeneratedSpline() const;

	UFUNCTION(BlueprintPure, Category = "Canal|Debug")
	bool ShouldRenderSemanticOverlay(bool bForDatasetCapture = false) const;

protected:
	virtual void BeginPlay() override;

private:
	bool ValidateTileSet(FString& OutError) const;
	void RefreshInstanceMeshes();
	void AddSocketInstance(ECanalSocketType SocketType, const FTransform& WorldTransform);
	void BuildSplineFromSolvedCells(const FCanalTileCompatibilityTable& Compatibility, const TArray<FHexWfcCellResult>& Cells);
	FVector GetBoundaryPortWorldPosition(const FHexBoundaryPort& Port) const;
	void DrawPortDebug() const;
	void DrawGridDebug(const FCanalTileCompatibilityTable& Compatibility) const;
	void DrawSemanticOverlay(const FCanalTileCompatibilityTable& Compatibility) const;
	bool IsWaterConnection(
		const FCanalTileCompatibilityTable& Compatibility,
		const FHexWfcCellResult& A,
		const FHexWfcCellResult& B,
		EHexDirection DirectionFromAToB) const;
	bool IsWaterCell(const FCanalTileCompatibilityTable& Compatibility, const FHexWfcCellResult& Cell) const;
	static bool IsWaterLikeSocket(ECanalSocketType Socket);
	static int32 DeriveDeterministicStreamSeed(int32 MasterSeed, uint32 StreamDiscriminator);

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WaterInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BankInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TowpathInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LockInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RoadInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<USplineComponent> WaterPathSpline;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	TObjectPtr<UCanalTopologyTileSetAsset> TileSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	FHexWfcGridConfig GridConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	FHexWfcSolveConfig SolveConfig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	FHexGridLayout GridLayout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation|Determinism")
	bool bDeriveSeedStreamsFromMaster = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation", meta = (ClampMin = "0.1"))
	float SocketOffsetScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	float SplineZOffset = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	bool bGenerateOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Generation")
	bool bGenerateSpline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug")
	bool bDrawPortDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug")
	bool bDrawGridDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug")
	bool bDrawSemanticOverlay = false;

	// Dataset capture should keep semantic overlays off by default.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug")
	bool bAllowSemanticOverlayInDatasetCapture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float PortDebugZOffset = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float PortDebugRadius = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float PortDebugDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float GridDebugDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float GridDebugThickness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug")
	float GridDebugLabelZOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float SemanticOverlayDuration = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug", meta = (ClampMin = "0.0"))
	float SemanticOverlayThickness = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Debug")
	float SemanticOverlayZOffset = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	TObjectPtr<UStaticMesh> DefaultMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	TObjectPtr<UStaticMesh> WaterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	TObjectPtr<UStaticMesh> BankMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	TObjectPtr<UStaticMesh> TowpathMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	TObjectPtr<UStaticMesh> LockMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	TObjectPtr<UStaticMesh> RoadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Meshes")
	FVector InstanceScale = FVector(0.25f, 0.25f, 0.10f);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	FHexWfcSolveResult LastSolveResult;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	FCanalGenerationMetadata LastGenerationMetadata;
};
