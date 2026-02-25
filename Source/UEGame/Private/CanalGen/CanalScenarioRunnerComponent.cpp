#include "CanalGen/CanalScenarioRunnerComponent.h"

#include "CanalGen/CanalTopologyGeneratorActor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UCanalScenarioRunnerComponent::UCanalScenarioRunnerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UCanalScenarioRunnerComponent::RunScenario(const int32 Seed, const bool bRegenerateTopology)
{
	AActor* ResolvedScenarioActor = ResolveScenarioActor();
	if (!ValidateScenarioActor(ResolvedScenarioActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("Scenario runner requires a ScenarioActor implementing UCanalScenarioInterface."));
		return false;
	}

	if (bScenarioRunning)
	{
		StopScenario();
	}

	const int32 ResolvedSeed = Seed >= 0 ? Seed : FMath::Rand();
	ACanalTopologyGeneratorActor* Generator = ResolveGeneratorActor();
	if (bRegenerateTopology && Generator)
	{
		Generator->SolveConfig.Seed = ResolvedSeed;
		Generator->GenerateTopology();
	}

	ICanalScenarioInterface::Execute_SetupScenario(ResolvedScenarioActor, ResolvedSeed);

	ActiveScenarioRequest = BuildScenarioRequest(ResolvedScenarioActor);
	ActiveSeed = ResolvedSeed;
	bScenarioRunning = true;

	if (Generator)
	{
		Generator->SetScenarioMetadata(ActiveScenarioRequest.ScenarioName, ActiveScenarioRequest.RequestedDurationSeconds);
	}

	ICanalScenarioInterface::Execute_BeginCapture(ResolvedScenarioActor);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScenarioEndTimerHandle);
		if (ActiveScenarioRequest.RequestedDurationSeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				ScenarioEndTimerHandle,
				this,
				&UCanalScenarioRunnerComponent::FinishScenarioByTimer,
				ActiveScenarioRequest.RequestedDurationSeconds,
				false);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Scenario started: name=%s seed=%d duration=%.2fs"),
		*ActiveScenarioRequest.ScenarioName.ToString(),
		ActiveSeed,
		ActiveScenarioRequest.RequestedDurationSeconds);

	return true;
}

void UCanalScenarioRunnerComponent::StopScenario()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScenarioEndTimerHandle);
	}

	if (bScenarioRunning)
	{
		if (AActor* ResolvedScenarioActor = ResolveScenarioActor())
		{
			if (ValidateScenarioActor(ResolvedScenarioActor))
			{
				ICanalScenarioInterface::Execute_EndCapture(ResolvedScenarioActor);
			}
		}

		bScenarioRunning = false;
		UE_LOG(LogTemp, Log, TEXT("Scenario stopped."));
	}
}

void UCanalScenarioRunnerComponent::GetEffectiveCameraPathPoints(TArray<FVector>& OutPoints) const
{
	OutPoints = ActiveScenarioRequest.RequestedCameraPathPoints;
	if (OutPoints.Num() > 0 || !ActiveScenarioRequest.bUseGeneratedWaterSpline)
	{
		return;
	}

	if (const ACanalTopologyGeneratorActor* Generator = ResolveGeneratorActor())
	{
		Generator->GetGeneratedSplinePoints(OutPoints, true);
	}
}

void UCanalScenarioRunnerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopScenario();
	Super::EndPlay(EndPlayReason);
}

void UCanalScenarioRunnerComponent::FinishScenarioByTimer()
{
	StopScenario();
}

AActor* UCanalScenarioRunnerComponent::ResolveScenarioActor() const
{
	if (ScenarioActor)
	{
		return ScenarioActor.Get();
	}

	if (bAutoFindScenarioActorOnOwner && GetOwner())
	{
		return GetOwner();
	}

	return nullptr;
}

ACanalTopologyGeneratorActor* UCanalScenarioRunnerComponent::ResolveGeneratorActor() const
{
	if (TopologyGeneratorActor)
	{
		return TopologyGeneratorActor.Get();
	}

	if (ACanalTopologyGeneratorActor* OwnerGenerator = Cast<ACanalTopologyGeneratorActor>(GetOwner()))
	{
		return OwnerGenerator;
	}

	if (!bAutoFindTopologyGeneratorActor)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ACanalTopologyGeneratorActor> It(World); It; ++It)
	{
		if (*It)
		{
			return *It;
		}
	}

	return nullptr;
}

bool UCanalScenarioRunnerComponent::ValidateScenarioActor(const AActor* Candidate) const
{
	return Candidate != nullptr && Candidate->GetClass()->ImplementsInterface(UCanalScenarioInterface::StaticClass());
}

FCanalScenarioRequest UCanalScenarioRunnerComponent::BuildScenarioRequest(AActor* InScenarioActor) const
{
	FCanalScenarioRequest Request = ICanalScenarioInterface::Execute_GetScenarioRequest(InScenarioActor);
	if (Request.ScenarioName.IsNone())
	{
		Request.ScenarioName = InScenarioActor->GetFName();
	}
	Request.RequestedDurationSeconds = FMath::Max(0.0f, Request.RequestedDurationSeconds);
	return Request;
}
