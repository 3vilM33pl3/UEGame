#pragma once

#include <limits>

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CanalPerfCaptureSubsystem.generated.h"

USTRUCT(BlueprintType)
struct UEGAME_API FCanalPerfCaptureReport
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	FString MapName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float WarmupSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float CaptureSecondsRequested = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float CaptureSecondsMeasured = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	int32 FramesCaptured = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float AverageFrameTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float MinFrameTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float MaxFrameTimeMs = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	float AverageFPS = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	FString OutputJsonPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Canal|Perf")
	FString OutputCsvPath;
};

UCLASS()
class UEGAME_API UCanalPerfCaptureSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	UFUNCTION(BlueprintCallable, Category = "Canal|Perf")
	bool StartCapture(float WarmupSeconds, float CaptureSeconds, const FString& OutputJsonPath, bool bExitOnComplete);

	UFUNCTION(BlueprintPure, Category = "Canal|Perf")
	bool IsCaptureRunning() const { return bCaptureRunning; }

	UFUNCTION(BlueprintPure, Category = "Canal|Perf")
	FCanalPerfCaptureReport GetLastReport() const { return LastReport; }

private:
	void StartCaptureInternal(float WarmupSeconds, float CaptureSeconds, const FString& OutputJsonPath, bool bExitOnComplete);
	void FinalizeCapture();
	FString ResolveAbsoluteOutputPath(const FString& RequestedPath) const;

	bool bCaptureRunning = false;
	bool bExitAfterCapture = false;
	bool bInWarmup = false;

	float WarmupSecondsRemaining = 0.0f;
	float CaptureSecondsRemaining = 0.0f;

	FString ActiveOutputJsonPath;

	int32 FramesCaptured = 0;
	double TotalFrameTimeMs = 0.0;
	double MinFrameTimeMs = std::numeric_limits<double>::max();
	double MaxFrameTimeMs = 0.0;

	FCanalPerfCaptureReport LastReport;
};
