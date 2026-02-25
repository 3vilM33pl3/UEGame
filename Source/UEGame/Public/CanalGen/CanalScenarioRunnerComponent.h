#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CanalGen/CanalScenarioInterface.h"
#include "CanalScenarioRunnerComponent.generated.h"

class ACanalTopologyGeneratorActor;
class AActor;

UCLASS(ClassGroup = (Canal), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class UEGAME_API UCanalScenarioRunnerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanalScenarioRunnerComponent();

	UFUNCTION(BlueprintCallable, Category = "Canal|Scenario")
	bool RunScenario(int32 Seed, bool bRegenerateTopology = true);

	UFUNCTION(BlueprintCallable, Category = "Canal|Scenario")
	void StopScenario();

	UFUNCTION(BlueprintPure, Category = "Canal|Scenario")
	bool IsScenarioRunning() const { return bScenarioRunning; }

	UFUNCTION(BlueprintPure, Category = "Canal|Scenario")
	int32 GetActiveSeed() const { return ActiveSeed; }

	UFUNCTION(BlueprintPure, Category = "Canal|Scenario")
	FCanalScenarioRequest GetActiveScenarioRequest() const { return ActiveScenarioRequest; }

	UFUNCTION(BlueprintCallable, Category = "Canal|Scenario")
	void GetEffectiveCameraPathPoints(TArray<FVector>& OutPoints) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void FinishScenarioByTimer();
	AActor* ResolveScenarioActor() const;
	ACanalTopologyGeneratorActor* ResolveGeneratorActor() const;
	bool ValidateScenarioActor(const AActor* Candidate) const;
	FCanalScenarioRequest BuildScenarioRequest(AActor* InScenarioActor) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> ScenarioActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ACanalTopologyGeneratorActor> TopologyGeneratorActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	bool bAutoFindScenarioActorOnOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	bool bAutoFindTopologyGeneratorActor = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	bool bScenarioRunning = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	int32 ActiveSeed = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Canal|Scenario", meta = (AllowPrivateAccess = "true"))
	FCanalScenarioRequest ActiveScenarioRequest;

	FTimerHandle ScenarioEndTimerHandle;
};
