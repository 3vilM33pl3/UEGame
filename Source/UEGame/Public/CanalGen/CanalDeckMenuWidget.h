#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CanalDeckMenuWidget.generated.h"

class UButton;
class UCanalScenarioRunnerComponent;
class UCanalSeedSessionComponent;
class UEditableTextBox;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class UEGAME_API UCanalDeckMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintCallable, Category = "Canal|DeckUI")
	void InitializeFromActor(AActor* InContextActor);

protected:
	UFUNCTION()
	void HandleGeneratePressed();

	UFUNCTION()
	void HandleScenarioPressed();

	UFUNCTION()
	void HandleStartCapturePressed();

	UFUNCTION()
	void HandleStopCapturePressed();

private:
	void EnsureDefaultWidgetTree();
	void ResolveComponents();
	int32 ResolveManualSeedInput(bool& bUsedManualSeed) const;
	void RefreshStatus(const FString& OverrideMessage = FString());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	float ButtonFontSize = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	float MinimumButtonHeight = 82.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	int32 FallbackManualSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanalSeedSessionComponent> SeedSessionComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanalScenarioRunnerComponent> ScenarioRunnerComponent;

	UPROPERTY(Transient)
	TObjectPtr<AActor> ContextActor;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GenerateButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ScenarioButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartCaptureButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> StopCaptureButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> ManualSeedTextBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;
};
