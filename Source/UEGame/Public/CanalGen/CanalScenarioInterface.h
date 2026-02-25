#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CanalScenarioInterface.generated.h"

USTRUCT(BlueprintType)
struct UEGAME_API FCanalScenarioRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario")
	FName ScenarioName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario", meta = (ClampMin = "0.0"))
	float RequestedDurationSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario")
	bool bUseGeneratedWaterSpline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|Scenario")
	TArray<FVector> RequestedCameraPathPoints;
};

UINTERFACE(BlueprintType)
class UEGAME_API UCanalScenarioInterface : public UInterface
{
	GENERATED_BODY()
};

class UEGAME_API ICanalScenarioInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Canal|Scenario")
	void SetupScenario(int32 Seed);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Canal|Scenario")
	void BeginCapture();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Canal|Scenario")
	void EndCapture();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Canal|Scenario")
	FCanalScenarioRequest GetScenarioRequest() const;
};
