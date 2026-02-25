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
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ADirectionalLight;
class AExponentialHeightFog;

UENUM(BlueprintType)
enum class ECanalTimeOfDayPreset : uint8
{
	Dawn = 0,
	Noon = 1,
	Dusk = 2,
	Night = 3
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	ECanalTimeOfDayPreset TimeOfDayPreset = ECanalTimeOfDayPreset::Noon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	float FogDensity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	FName ScenarioName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Generation")
	float ScenarioDurationSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct UEGAME_API FCanalPrototypeMaterialProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials")
	TObjectPtr<UMaterialInterface> Material = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Wetness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TintJitter = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WetnessJitter = 0.15f;
};

USTRUCT(BlueprintType)
struct UEGAME_API FCanalResolvedMaterialProfile
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Materials")
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Materials")
	float Wetness = 0.5f;
};

USTRUCT(BlueprintType)
struct UEGAME_API FCanalTowpathPropDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props")
	FName PropId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props")
	FName SemanticTag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props")
	FVector Scale = FVector(0.22f, 0.22f, 0.22f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props", meta = (ClampMin = "0.0"))
	float VerticalOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Weight = 1.0f;
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

	UFUNCTION(BlueprintCallable, Category = "Canal|Generation")
	void GetGeneratedSplinePoints(TArray<FVector>& OutPoints, bool bWorldSpace = true) const;

	UFUNCTION(BlueprintPure, Category = "Canal|Debug")
	bool ShouldRenderSemanticOverlay(bool bForDatasetCapture = false) const;

	UFUNCTION(BlueprintCallable, Category = "Canal|Scenario")
	void SetScenarioMetadata(FName InScenarioName, float InScenarioDurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "Canal|Environment")
	void SetTimeOfDayPreset(ECanalTimeOfDayPreset NewPreset, bool bApplyNow = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Environment")
	void SetFogDensity(float NewFogDensity, bool bApplyNow = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Environment")
	void ApplyEnvironmentSettings();

	UFUNCTION(BlueprintPure, Category = "Canal|Props")
	int32 GetTotalTowpathPropCount() const;

	UFUNCTION(BlueprintPure, Category = "Canal|Props")
	int32 GetTowpathPropCountByTag(FName SemanticTag) const;

	UFUNCTION(BlueprintCallable, Category = "Canal|Props")
	void GetTowpathPropSemanticTags(TArray<FName>& OutTags) const;

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
	void ApplyPrototypeMaterials(int32 DressingSeed);
	void ApplyMaterialProfile(
		UHierarchicalInstancedStaticMeshComponent* Component,
		TObjectPtr<UMaterialInstanceDynamic>& OutRuntimeMaterial,
		const FCanalPrototypeMaterialProfile& Profile,
		FRandomStream& Random,
		FCanalResolvedMaterialProfile& OutResolvedProfile);
	void RefreshTowpathPropMeshes();
	void SpawnTowpathProps(int32 DressingSeed);
	UHierarchicalInstancedStaticMeshComponent* ResolveTowpathPropComponent(FName SemanticTag) const;
	bool PlaceTowpathPropAtInstance(const FCanalTowpathPropDefinition& Definition, int32 TowpathInstanceIndex, FRandomStream& Random);
	const FCanalTowpathPropDefinition* PickWeightedTowpathProp(FRandomStream& Random) const;
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
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BollardPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RingPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SignPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> LampPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BenchPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ReedsPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BinPropInstances;

	UPROPERTY(VisibleAnywhere, Category = "Canal|Components")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FencePropInstances;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials")
	FCanalPrototypeMaterialProfile WaterMaterialProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials")
	FCanalPrototypeMaterialProfile BankMaterialProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Materials")
	FCanalPrototypeMaterialProfile TowpathMaterialProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props")
	bool bSpawnTowpathProps = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TowpathPropDensity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props", meta = (ClampMin = "0.0"))
	float TowpathPropLateralJitter = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props", meta = (ClampMin = "0.0"))
	float TowpathPropYawJitter = 22.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props", meta = (ClampMin = "0.0"))
	float TowpathPropZOffset = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Props")
	TArray<FCanalTowpathPropDefinition> TowpathPropDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Environment")
	ECanalTimeOfDayPreset TimeOfDayPreset = ECanalTimeOfDayPreset::Noon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Environment", meta = (ClampMin = "0.0"))
	float FogDensity = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Environment")
	bool bApplyEnvironmentOnGenerate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Environment")
	TObjectPtr<ADirectionalLight> DirectionalLightActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Environment")
	TObjectPtr<AExponentialHeightFog> ExponentialHeightFogActor;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Materials")
	FCanalResolvedMaterialProfile LastWaterMaterialRuntime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Materials")
	FCanalResolvedMaterialProfile LastBankMaterialRuntime;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Materials")
	FCanalResolvedMaterialProfile LastTowpathMaterialRuntime;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> WaterRuntimeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BankRuntimeMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> TowpathRuntimeMaterial;
};
