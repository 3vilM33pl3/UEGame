#include "CanalGen/CanalTopologyGeneratorActor.h"

#include "CanalGen/CanalTopologyTileSetAsset.h"
#include "Algo/Reverse.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FRotator GetSunRotationForPreset(const ECanalTimeOfDayPreset Preset)
	{
		switch (Preset)
		{
		case ECanalTimeOfDayPreset::Dawn:
			return FRotator(-8.0f, 90.0f, 0.0f);
		case ECanalTimeOfDayPreset::Noon:
			return FRotator(-65.0f, 180.0f, 0.0f);
		case ECanalTimeOfDayPreset::Dusk:
			return FRotator(-8.0f, 270.0f, 0.0f);
		case ECanalTimeOfDayPreset::Night:
		default:
			return FRotator(8.0f, 0.0f, 0.0f);
		}
	}

	float GetSunIntensityForPreset(const ECanalTimeOfDayPreset Preset)
	{
		switch (Preset)
		{
		case ECanalTimeOfDayPreset::Dawn:
			return 3.0f;
		case ECanalTimeOfDayPreset::Noon:
			return 10.0f;
		case ECanalTimeOfDayPreset::Dusk:
			return 2.5f;
		case ECanalTimeOfDayPreset::Night:
		default:
			return 0.05f;
		}
	}

	FLinearColor GetSunColorForPreset(const ECanalTimeOfDayPreset Preset)
	{
		switch (Preset)
		{
		case ECanalTimeOfDayPreset::Dawn:
			return FLinearColor(1.0f, 0.74f, 0.56f);
		case ECanalTimeOfDayPreset::Noon:
			return FLinearColor(1.0f, 0.98f, 0.92f);
		case ECanalTimeOfDayPreset::Dusk:
			return FLinearColor(1.0f, 0.62f, 0.44f);
		case ECanalTimeOfDayPreset::Night:
		default:
			return FLinearColor(0.42f, 0.48f, 0.64f);
		}
	}

	constexpr TCHAR kTintParamName[] = TEXT("Tint");
	constexpr TCHAR kBaseTintParamName[] = TEXT("BaseTint");
	constexpr TCHAR kColorTintParamName[] = TEXT("ColorTint");
	constexpr TCHAR kWetnessParamName[] = TEXT("Wetness");
	constexpr TCHAR kRoughnessParamName[] = TEXT("Roughness");

	const FName kPropTagBollard(TEXT("bollard"));
	const FName kPropTagRing(TEXT("ring"));
	const FName kPropTagSign(TEXT("sign"));
	const FName kPropTagLamp(TEXT("lamp"));
	const FName kPropTagBench(TEXT("bench"));
	const FName kPropTagReeds(TEXT("reeds"));
	const FName kPropTagBin(TEXT("bin"));
	const FName kPropTagFence(TEXT("fence"));

	void SetMaterialRandomizationParams(UMaterialInstanceDynamic* Material, const FLinearColor& Tint, const float Wetness)
	{
		if (!Material)
		{
			return;
		}

		Material->SetVectorParameterValue(kTintParamName, Tint);
		Material->SetVectorParameterValue(kBaseTintParamName, Tint);
		Material->SetVectorParameterValue(kColorTintParamName, Tint);
		Material->SetScalarParameterValue(kWetnessParamName, Wetness);
		Material->SetScalarParameterValue(kRoughnessParamName, FMath::Clamp(1.0f - Wetness, 0.05f, 1.0f));
	}
}

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
	BollardPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BollardPropInstances"));
	RingPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("RingPropInstances"));
	SignPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("SignPropInstances"));
	LampPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("LampPropInstances"));
	BenchPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BenchPropInstances"));
	ReedsPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("ReedsPropInstances"));
	BinPropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BinPropInstances"));
	FencePropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FencePropInstances"));
	WaterPathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WaterPathSpline"));

	WaterInstances->SetupAttachment(SceneRoot);
	BankInstances->SetupAttachment(SceneRoot);
	TowpathInstances->SetupAttachment(SceneRoot);
	LockInstances->SetupAttachment(SceneRoot);
	RoadInstances->SetupAttachment(SceneRoot);
	BollardPropInstances->SetupAttachment(SceneRoot);
	RingPropInstances->SetupAttachment(SceneRoot);
	SignPropInstances->SetupAttachment(SceneRoot);
	LampPropInstances->SetupAttachment(SceneRoot);
	BenchPropInstances->SetupAttachment(SceneRoot);
	ReedsPropInstances->SetupAttachment(SceneRoot);
	BinPropInstances->SetupAttachment(SceneRoot);
	FencePropInstances->SetupAttachment(SceneRoot);
	WaterPathSpline->SetupAttachment(SceneRoot);

	BollardPropInstances->ComponentTags = {kPropTagBollard};
	RingPropInstances->ComponentTags = {kPropTagRing};
	SignPropInstances->ComponentTags = {kPropTagSign};
	LampPropInstances->ComponentTags = {kPropTagLamp};
	BenchPropInstances->ComponentTags = {kPropTagBench};
	ReedsPropInstances->ComponentTags = {kPropTagReeds};
	BinPropInstances->ComponentTags = {kPropTagBin};
	FencePropInstances->ComponentTags = {kPropTagFence};

	WaterPathSpline->SetClosedLoop(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (CubeMesh.Succeeded())
	{
		DefaultMesh = CubeMesh.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WaterMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BankMaterialFinder(TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TowpathMaterialFinder(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterialFinder(TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (WaterMaterialFinder.Succeeded())
	{
		WaterMaterialProfile.Material = WaterMaterialFinder.Object;
	}
	else if (FallbackMaterialFinder.Succeeded())
	{
		WaterMaterialProfile.Material = FallbackMaterialFinder.Object;
	}
	if (BankMaterialFinder.Succeeded())
	{
		BankMaterialProfile.Material = BankMaterialFinder.Object;
	}
	else if (FallbackMaterialFinder.Succeeded())
	{
		BankMaterialProfile.Material = FallbackMaterialFinder.Object;
	}
	if (TowpathMaterialFinder.Succeeded())
	{
		TowpathMaterialProfile.Material = TowpathMaterialFinder.Object;
	}
	else if (FallbackMaterialFinder.Succeeded())
	{
		TowpathMaterialProfile.Material = FallbackMaterialFinder.Object;
	}

	WaterMaterialProfile.Tint = FLinearColor(0.16f, 0.38f, 0.66f);
	WaterMaterialProfile.Wetness = 0.85f;
	WaterMaterialProfile.TintJitter = 0.10f;
	WaterMaterialProfile.WetnessJitter = 0.08f;
	BankMaterialProfile.Tint = FLinearColor(0.48f, 0.40f, 0.30f);
	BankMaterialProfile.Wetness = 0.38f;
	BankMaterialProfile.TintJitter = 0.09f;
	BankMaterialProfile.WetnessJitter = 0.10f;
	TowpathMaterialProfile.Tint = FLinearColor(0.58f, 0.52f, 0.42f);
	TowpathMaterialProfile.Wetness = 0.25f;
	TowpathMaterialProfile.TintJitter = 0.08f;
	TowpathMaterialProfile.WetnessJitter = 0.10f;

	UStaticMesh* BollardMesh = CylinderMesh.Succeeded() ? CylinderMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* RingMesh = SphereMesh.Succeeded() ? SphereMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* SignMesh = CubeMesh.Succeeded() ? CubeMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* LampMesh = ConeMesh.Succeeded() ? ConeMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* BenchMesh = CubeMesh.Succeeded() ? CubeMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* ReedsMesh = ConeMesh.Succeeded() ? ConeMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* BinMesh = CylinderMesh.Succeeded() ? CylinderMesh.Object.Get() : DefaultMesh.Get();
	UStaticMesh* FenceMesh = PlaneMesh.Succeeded() ? PlaneMesh.Object.Get() : DefaultMesh.Get();

	TowpathPropDefinitions = {
		{FName(TEXT("prop_bollard")), kPropTagBollard, BollardMesh, FVector(0.10f, 0.10f, 0.18f), 10.0f, 1.0f},
		{FName(TEXT("prop_ring")), kPropTagRing, RingMesh, FVector(0.10f, 0.10f, 0.04f), 16.0f, 0.8f},
		{FName(TEXT("prop_sign")), kPropTagSign, SignMesh, FVector(0.05f, 0.16f, 0.24f), 18.0f, 0.9f},
		{FName(TEXT("prop_lamp")), kPropTagLamp, LampMesh, FVector(0.09f, 0.09f, 0.28f), 20.0f, 0.7f},
		{FName(TEXT("prop_bench")), kPropTagBench, BenchMesh, FVector(0.22f, 0.08f, 0.08f), 8.0f, 0.95f},
		{FName(TEXT("prop_reeds")), kPropTagReeds, ReedsMesh, FVector(0.07f, 0.07f, 0.22f), 4.0f, 1.0f},
		{FName(TEXT("prop_bin")), kPropTagBin, BinMesh, FVector(0.10f, 0.10f, 0.14f), 9.0f, 0.85f},
		{FName(TEXT("prop_fence")), kPropTagFence, FenceMesh, FVector(0.16f, 0.16f, 0.20f), 12.0f, 0.9f}};

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
	ApplyEnvironmentSettings();

	if (bGenerateOnBeginPlay)
	{
		GenerateTopology();
	}
}

void ACanalTopologyGeneratorActor::GenerateTopology()
{
	ClearGenerated();

	if (bApplyEnvironmentOnGenerate)
	{
		ApplyEnvironmentSettings();
	}

	FString ValidationError;
	if (!ValidateTileSet(ValidationError))
	{
		UE_LOG(LogTemp, Warning, TEXT("Canal generation aborted: %s"), *ValidationError);
		return;
	}

	RefreshInstanceMeshes();

	const int32 MasterSeed = SolveConfig.Seed;
	const int32 TopologySeed = bDeriveSeedStreamsFromMaster ? DeriveDeterministicStreamSeed(MasterSeed, 0x544F504Fu) : MasterSeed; // 'TOPO'
	const int32 DressingSeed = bDeriveSeedStreamsFromMaster ? DeriveDeterministicStreamSeed(MasterSeed, 0x44524553u) : MasterSeed; // 'DRES'

	FHexWfcSolveConfig TopologySolveConfig = SolveConfig;
	TopologySolveConfig.Seed = TopologySeed;

	LastGenerationMetadata = FCanalGenerationMetadata();
	LastGenerationMetadata.MasterSeed = MasterSeed;
	LastGenerationMetadata.TopologySeed = TopologySeed;
	LastGenerationMetadata.DressingSeed = DressingSeed;
	LastGenerationMetadata.BiomeProfile = TopologySolveConfig.BiomeProfile;
	LastGenerationMetadata.TimeOfDayPreset = TimeOfDayPreset;
	LastGenerationMetadata.FogDensity = FogDensity;

	LastSolveResult = UCanalWfcBlueprintLibrary::SolveHexWfc(TileSet, GridConfig, TopologySolveConfig);
	if (!LastSolveResult.bSolved)
	{
		UE_LOG(LogTemp, Warning, TEXT("Canal solve failed: %s"), *LastSolveResult.Message);
		return;
	}

	if (LastSolveResult.bHasResolvedPorts)
	{
		LastGenerationMetadata.bHasEntryPort = true;
		LastGenerationMetadata.EntryPort = LastSolveResult.ResolvedEntryPort;
		LastGenerationMetadata.bHasExitPort = true;
		LastGenerationMetadata.ExitPort = LastSolveResult.ResolvedExitPort;
	}
	else
	{
		LastGenerationMetadata.bHasEntryPort = TopologySolveConfig.EntryPort.bEnabled;
		LastGenerationMetadata.EntryPort = TopologySolveConfig.EntryPort;
		LastGenerationMetadata.bHasExitPort = TopologySolveConfig.ExitPort.bEnabled;
		LastGenerationMetadata.ExitPort = TopologySolveConfig.ExitPort;
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

	ApplyPrototypeMaterials(DressingSeed);
	SpawnTowpathProps(DressingSeed);

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
	if (ShouldRenderSemanticOverlay(false))
	{
		DrawSemanticOverlay(Compatibility);
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
	BollardPropInstances->ClearInstances();
	RingPropInstances->ClearInstances();
	SignPropInstances->ClearInstances();
	LampPropInstances->ClearInstances();
	BenchPropInstances->ClearInstances();
	ReedsPropInstances->ClearInstances();
	BinPropInstances->ClearInstances();
	FencePropInstances->ClearInstances();

	WaterPathSpline->ClearSplinePoints(false);
	WaterPathSpline->UpdateSpline();

	LastWaterMaterialRuntime = FCanalResolvedMaterialProfile();
	LastBankMaterialRuntime = FCanalResolvedMaterialProfile();
	LastTowpathMaterialRuntime = FCanalResolvedMaterialProfile();
	WaterRuntimeMaterial = nullptr;
	BankRuntimeMaterial = nullptr;
	TowpathRuntimeMaterial = nullptr;

	LastGenerationMetadata = FCanalGenerationMetadata();
}

bool ACanalTopologyGeneratorActor::HasGeneratedSpline() const
{
	return WaterPathSpline->GetNumberOfSplinePoints() >= 2;
}

void ACanalTopologyGeneratorActor::GetGeneratedSplinePoints(TArray<FVector>& OutPoints, const bool bWorldSpace) const
{
	OutPoints.Reset();

	const int32 NumPoints = WaterPathSpline->GetNumberOfSplinePoints();
	OutPoints.Reserve(NumPoints);

	const ESplineCoordinateSpace::Type Space = bWorldSpace ? ESplineCoordinateSpace::World : ESplineCoordinateSpace::Local;
	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		OutPoints.Add(WaterPathSpline->GetLocationAtSplinePoint(Index, Space));
	}
}

void ACanalTopologyGeneratorActor::SetScenarioMetadata(const FName InScenarioName, const float InScenarioDurationSeconds)
{
	LastGenerationMetadata.ScenarioName = InScenarioName;
	LastGenerationMetadata.ScenarioDurationSeconds = FMath::Max(0.0f, InScenarioDurationSeconds);
}

void ACanalTopologyGeneratorActor::SetTimeOfDayPreset(const ECanalTimeOfDayPreset NewPreset, const bool bApplyNow)
{
	TimeOfDayPreset = NewPreset;
	if (bApplyNow)
	{
		ApplyEnvironmentSettings();
	}
}

void ACanalTopologyGeneratorActor::SetFogDensity(const float NewFogDensity, const bool bApplyNow)
{
	FogDensity = FMath::Max(0.0f, NewFogDensity);
	if (bApplyNow)
	{
		ApplyEnvironmentSettings();
	}
}

void ACanalTopologyGeneratorActor::ApplyEnvironmentSettings()
{
	LastGenerationMetadata.TimeOfDayPreset = TimeOfDayPreset;
	LastGenerationMetadata.FogDensity = FogDensity;

	if (DirectionalLightActor)
	{
		DirectionalLightActor->SetActorRotation(GetSunRotationForPreset(TimeOfDayPreset));
		if (ULightComponent* LightComponent = DirectionalLightActor->GetLightComponent())
		{
			LightComponent->SetIntensity(GetSunIntensityForPreset(TimeOfDayPreset));
			LightComponent->SetLightColor(GetSunColorForPreset(TimeOfDayPreset));
		}
	}

	if (ExponentialHeightFogActor)
	{
		if (UExponentialHeightFogComponent* FogComponent = ExponentialHeightFogActor->GetComponent())
		{
			FogComponent->SetFogDensity(FogDensity);
		}
	}
}

int32 ACanalTopologyGeneratorActor::GetTotalTowpathPropCount() const
{
	return BollardPropInstances->GetInstanceCount()
		+ RingPropInstances->GetInstanceCount()
		+ SignPropInstances->GetInstanceCount()
		+ LampPropInstances->GetInstanceCount()
		+ BenchPropInstances->GetInstanceCount()
		+ ReedsPropInstances->GetInstanceCount()
		+ BinPropInstances->GetInstanceCount()
		+ FencePropInstances->GetInstanceCount();
}

int32 ACanalTopologyGeneratorActor::GetTowpathPropCountByTag(const FName SemanticTag) const
{
	if (UHierarchicalInstancedStaticMeshComponent* const Component = ResolveTowpathPropComponent(SemanticTag))
	{
		return Component->GetInstanceCount();
	}

	return 0;
}

void ACanalTopologyGeneratorActor::GetTowpathPropSemanticTags(TArray<FName>& OutTags) const
{
	OutTags.Reset();
	for (const FCanalTowpathPropDefinition& Definition : TowpathPropDefinitions)
	{
		if (!Definition.SemanticTag.IsNone())
		{
			OutTags.AddUnique(Definition.SemanticTag);
		}
	}
}

void ACanalTopologyGeneratorActor::ApplyPrototypeMaterials(const int32 DressingSeed)
{
	FRandomStream Random(DeriveDeterministicStreamSeed(DressingSeed, 0x4D41544Cu)); // 'MATL'

	ApplyMaterialProfile(
		WaterInstances,
		WaterRuntimeMaterial,
		WaterMaterialProfile,
		Random,
		LastWaterMaterialRuntime);
	ApplyMaterialProfile(
		BankInstances,
		BankRuntimeMaterial,
		BankMaterialProfile,
		Random,
		LastBankMaterialRuntime);
	ApplyMaterialProfile(
		TowpathInstances,
		TowpathRuntimeMaterial,
		TowpathMaterialProfile,
		Random,
		LastTowpathMaterialRuntime);
}

void ACanalTopologyGeneratorActor::ApplyMaterialProfile(
	UHierarchicalInstancedStaticMeshComponent* Component,
	TObjectPtr<UMaterialInstanceDynamic>& OutRuntimeMaterial,
	const FCanalPrototypeMaterialProfile& Profile,
	FRandomStream& Random,
	FCanalResolvedMaterialProfile& OutResolvedProfile)
{
	if (!Component)
	{
		return;
	}

	UMaterialInterface* SourceMaterial = Profile.Material.Get();
	if (!SourceMaterial)
	{
		SourceMaterial = Component->GetMaterial(0);
	}
	if (!SourceMaterial)
	{
		return;
	}

	const auto Jitter = [&Random](const float Magnitude) -> float
	{
		return Magnitude > 0.0f ? Random.FRandRange(-Magnitude, Magnitude) : 0.0f;
	};

	FLinearColor ResolvedTint = Profile.Tint;
	ResolvedTint.R = FMath::Clamp(ResolvedTint.R + Jitter(Profile.TintJitter), 0.0f, 1.0f);
	ResolvedTint.G = FMath::Clamp(ResolvedTint.G + Jitter(Profile.TintJitter), 0.0f, 1.0f);
	ResolvedTint.B = FMath::Clamp(ResolvedTint.B + Jitter(Profile.TintJitter), 0.0f, 1.0f);
	ResolvedTint.A = 1.0f;

	const float ResolvedWetness = FMath::Clamp(Profile.Wetness + Jitter(Profile.WetnessJitter), 0.0f, 1.0f);

	OutRuntimeMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
	if (!OutRuntimeMaterial)
	{
		return;
	}

	SetMaterialRandomizationParams(OutRuntimeMaterial, ResolvedTint, ResolvedWetness);
	Component->SetMaterial(0, OutRuntimeMaterial);
	OutResolvedProfile.Tint = ResolvedTint;
	OutResolvedProfile.Wetness = ResolvedWetness;
}

void ACanalTopologyGeneratorActor::RefreshTowpathPropMeshes()
{
	auto ApplyMesh = [this](const FName SemanticTag, UHierarchicalInstancedStaticMeshComponent* Component)
	{
		if (!Component)
		{
			return;
		}

		const FCanalTowpathPropDefinition* Definition = TowpathPropDefinitions.FindByPredicate(
			[&SemanticTag](const FCanalTowpathPropDefinition& Candidate)
			{
				return Candidate.SemanticTag == SemanticTag;
			});
		if (!Definition)
		{
			return;
		}

		UStaticMesh* Mesh = Definition->Mesh.Get() ? Definition->Mesh.Get() : DefaultMesh.Get();
		Component->SetStaticMesh(Mesh);
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	};

	ApplyMesh(kPropTagBollard, BollardPropInstances);
	ApplyMesh(kPropTagRing, RingPropInstances);
	ApplyMesh(kPropTagSign, SignPropInstances);
	ApplyMesh(kPropTagLamp, LampPropInstances);
	ApplyMesh(kPropTagBench, BenchPropInstances);
	ApplyMesh(kPropTagReeds, ReedsPropInstances);
	ApplyMesh(kPropTagBin, BinPropInstances);
	ApplyMesh(kPropTagFence, FencePropInstances);
}

void ACanalTopologyGeneratorActor::SpawnTowpathProps(const int32 DressingSeed)
{
	if (!bSpawnTowpathProps || TowpathPropDensity <= 0.0f)
	{
		return;
	}

	const int32 TowpathInstanceCount = TowpathInstances->GetInstanceCount();
	if (TowpathInstanceCount == 0)
	{
		return;
	}

	FRandomStream Random(DeriveDeterministicStreamSeed(DressingSeed, 0x50524F50u)); // 'PROP'

	TArray<int32> CandidateIndices;
	CandidateIndices.Reserve(TowpathInstanceCount);
	for (int32 InstanceIndex = 0; InstanceIndex < TowpathInstanceCount; ++InstanceIndex)
	{
		if (Random.FRand() <= TowpathPropDensity)
		{
			CandidateIndices.Add(InstanceIndex);
		}
	}
	if (CandidateIndices.Num() == 0)
	{
		return;
	}

	// Coverage-first placement so each configured semantic prop type appears at least once when possible.
	for (const FCanalTowpathPropDefinition& Definition : TowpathPropDefinitions)
	{
		if (Definition.SemanticTag.IsNone() || Definition.Weight <= 0.0f || CandidateIndices.Num() == 0)
		{
			continue;
		}

		const int32 Picked = Random.RandRange(0, CandidateIndices.Num() - 1);
		const int32 TowpathInstanceIndex = CandidateIndices[Picked];
		CandidateIndices.RemoveAtSwap(Picked);
		PlaceTowpathPropAtInstance(Definition, TowpathInstanceIndex, Random);
	}

	while (CandidateIndices.Num() > 0)
	{
		const FCanalTowpathPropDefinition* Definition = PickWeightedTowpathProp(Random);
		if (!Definition)
		{
			break;
		}

		const int32 Picked = Random.RandRange(0, CandidateIndices.Num() - 1);
		const int32 TowpathInstanceIndex = CandidateIndices[Picked];
		CandidateIndices.RemoveAtSwap(Picked);
		PlaceTowpathPropAtInstance(*Definition, TowpathInstanceIndex, Random);
	}
}

UHierarchicalInstancedStaticMeshComponent* ACanalTopologyGeneratorActor::ResolveTowpathPropComponent(const FName SemanticTag) const
{
	if (SemanticTag == kPropTagBollard)
	{
		return BollardPropInstances;
	}
	if (SemanticTag == kPropTagRing)
	{
		return RingPropInstances;
	}
	if (SemanticTag == kPropTagSign)
	{
		return SignPropInstances;
	}
	if (SemanticTag == kPropTagLamp)
	{
		return LampPropInstances;
	}
	if (SemanticTag == kPropTagBench)
	{
		return BenchPropInstances;
	}
	if (SemanticTag == kPropTagReeds)
	{
		return ReedsPropInstances;
	}
	if (SemanticTag == kPropTagBin)
	{
		return BinPropInstances;
	}
	if (SemanticTag == kPropTagFence)
	{
		return FencePropInstances;
	}
	return nullptr;
}

bool ACanalTopologyGeneratorActor::PlaceTowpathPropAtInstance(
	const FCanalTowpathPropDefinition& Definition,
	const int32 TowpathInstanceIndex,
	FRandomStream& Random)
{
	UHierarchicalInstancedStaticMeshComponent* TargetComponent = ResolveTowpathPropComponent(Definition.SemanticTag);
	if (!TargetComponent)
	{
		return false;
	}

	FTransform TowpathTransform;
	if (!TowpathInstances->GetInstanceTransform(TowpathInstanceIndex, TowpathTransform, true))
	{
		return false;
	}

	FVector Location = TowpathTransform.GetLocation();
	Location += TowpathTransform.GetUnitAxis(EAxis::Z) * (TowpathPropZOffset + Definition.VerticalOffset);
	Location += TowpathTransform.GetUnitAxis(EAxis::Y) * Random.FRandRange(-TowpathPropLateralJitter, TowpathPropLateralJitter);

	FRotator Rotation = TowpathTransform.Rotator();
	Rotation.Yaw += Random.FRandRange(-TowpathPropYawJitter, TowpathPropYawJitter);

	const FTransform PropTransform(Rotation, Location, Definition.Scale);
	TargetComponent->AddInstance(PropTransform, true);
	return true;
}

const FCanalTowpathPropDefinition* ACanalTopologyGeneratorActor::PickWeightedTowpathProp(FRandomStream& Random) const
{
	float TotalWeight = 0.0f;
	for (const FCanalTowpathPropDefinition& Definition : TowpathPropDefinitions)
	{
		if (Definition.Weight > 0.0f && !Definition.SemanticTag.IsNone())
		{
			TotalWeight += Definition.Weight;
		}
	}
	if (TotalWeight <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	float Draw = Random.FRandRange(0.0f, TotalWeight);
	for (const FCanalTowpathPropDefinition& Definition : TowpathPropDefinitions)
	{
		if (Definition.Weight <= 0.0f || Definition.SemanticTag.IsNone())
		{
			continue;
		}

		Draw -= Definition.Weight;
		if (Draw <= 0.0f)
		{
			return &Definition;
		}
	}

	return TowpathPropDefinitions.Num() > 0 ? &TowpathPropDefinitions.Last() : nullptr;
}

bool ACanalTopologyGeneratorActor::ShouldRenderSemanticOverlay(const bool bForDatasetCapture) const
{
	if (!bDrawSemanticOverlay)
	{
		return false;
	}

	if (bForDatasetCapture && !bAllowSemanticOverlayInDatasetCapture)
	{
		return false;
	}

	return true;
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
	RefreshTowpathPropMeshes();
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

void ACanalTopologyGeneratorActor::DrawSemanticOverlay(const FCanalTileCompatibilityTable& Compatibility) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TMap<FHexAxialCoord, const FHexWfcCellResult*> CellByCoord;
	CellByCoord.Reserve(LastSolveResult.Cells.Num());
	for (const FHexWfcCellResult& Cell : LastSolveResult.Cells)
	{
		CellByCoord.Add(Cell.Coord, &Cell);
	}

	auto SocketColor = [](const ECanalSocketType Socket) -> FColor
	{
		switch (Socket)
		{
		case ECanalSocketType::Water:
			return FColor(32, 180, 255);
		case ECanalSocketType::TowpathL:
			return FColor(64, 220, 96);
		case ECanalSocketType::TowpathR:
			return FColor(245, 210, 32);
		case ECanalSocketType::Lock:
			return FColor(255, 140, 32);
		case ECanalSocketType::Road:
			return FColor(255, 64, 210);
		case ECanalSocketType::Bank:
		default:
			return FColor(110, 110, 110);
		}
	};

	for (const FHexWfcCellResult& Cell : LastSolveResult.Cells)
	{
		const FCanalTopologyTileDefinition* Tile = Compatibility.GetTileDefinition(Cell.Variant.TileIndex);
		if (!Tile)
		{
			continue;
		}

		const FVector CellCenter = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord, SemanticOverlayZOffset));
		for (int32 DirectionIndex = 0; DirectionIndex < 6; ++DirectionIndex)
		{
			const EHexDirection Direction = HexDirectionFromIndex(DirectionIndex);
			const ECanalSocketType Socket = Tile->GetSocket(Direction, Cell.Variant.RotationSteps);
			if (Socket == ECanalSocketType::Bank)
			{
				continue;
			}

			const FVector NeighborCenter = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord.Neighbor(Direction), SemanticOverlayZOffset));
			const FVector DirectionVector = (NeighborCenter - CellCenter).GetSafeNormal();
			const FVector SocketPosition = CellCenter + DirectionVector * (GridLayout.HexSize * SocketOffsetScale);
			const FColor Color = SocketColor(Socket);

			DrawDebugLine(World, CellCenter, SocketPosition, Color, false, SemanticOverlayDuration, 0, SemanticOverlayThickness);
			DrawDebugPoint(World, SocketPosition, 8.0f, Color, false, SemanticOverlayDuration, 0);
		}
	}

	// Draw contiguous water channels explicitly so path connectivity is obvious at a glance.
	for (const FHexWfcCellResult& Cell : LastSolveResult.Cells)
	{
		const FVector CellCenter = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(Cell.Coord, SemanticOverlayZOffset));
		for (int32 DirectionIndex = 0; DirectionIndex < 6; ++DirectionIndex)
		{
			const EHexDirection Direction = HexDirectionFromIndex(DirectionIndex);
			const FHexAxialCoord NeighborCoord = Cell.Coord.Neighbor(Direction);
			const FHexWfcCellResult* NeighborCell = CellByCoord.FindRef(NeighborCoord);
			if (!NeighborCell)
			{
				continue;
			}

			if (NeighborCell->Coord.R < Cell.Coord.R || (NeighborCell->Coord.R == Cell.Coord.R && NeighborCell->Coord.Q < Cell.Coord.Q))
			{
				continue;
			}

			if (!IsWaterConnection(Compatibility, Cell, *NeighborCell, Direction))
			{
				continue;
			}

			const FVector NeighborCenter = GetActorTransform().TransformPosition(GridLayout.AxialToWorld(NeighborCoord, SemanticOverlayZOffset));
			DrawDebugLine(World, CellCenter, NeighborCenter, FColor(32, 190, 255), false, SemanticOverlayDuration, 0, SemanticOverlayThickness + 1.0f);
		}
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

int32 ACanalTopologyGeneratorActor::DeriveDeterministicStreamSeed(const int32 MasterSeed, const uint32 StreamDiscriminator)
{
	uint32 Value = static_cast<uint32>(MasterSeed);
	Value ^= StreamDiscriminator + 0x9E3779B9u + (Value << 6) + (Value >> 2);
	Value ^= (Value >> 16);
	Value *= 0x7FEB352Du;
	Value ^= (Value >> 15);
	Value *= 0x846CA68Bu;
	Value ^= (Value >> 16);
	if (Value == 0u)
	{
		Value = StreamDiscriminator ^ 0xA511E9B3u;
	}

	return static_cast<int32>(Value & 0x7FFFFFFFu);
}
