#include "CanalGen/CanalDeckMenuWidget.h"

#include "CanalGen/CanalScenarioRunnerComponent.h"
#include "CanalGen/CanalSeedSessionComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "String/LexFromString.h"

void UCanalDeckMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureDefaultWidgetTree();
	ResolveComponents();

	if (GenerateButton)
	{
		GenerateButton->OnClicked.AddUniqueDynamic(this, &UCanalDeckMenuWidget::HandleGeneratePressed);
	}
	if (ScenarioButton)
	{
		ScenarioButton->OnClicked.AddUniqueDynamic(this, &UCanalDeckMenuWidget::HandleScenarioPressed);
	}
	if (StartCaptureButton)
	{
		StartCaptureButton->OnClicked.AddUniqueDynamic(this, &UCanalDeckMenuWidget::HandleStartCapturePressed);
	}
	if (StopCaptureButton)
	{
		StopCaptureButton->OnClicked.AddUniqueDynamic(this, &UCanalDeckMenuWidget::HandleStopCapturePressed);
	}

	if (GenerateButton)
	{
		GenerateButton->SetKeyboardFocus();
	}

	RefreshStatus();
}

void UCanalDeckMenuWidget::InitializeFromActor(AActor* InContextActor)
{
	ContextActor = InContextActor;
	ResolveComponents();
	RefreshStatus();
}

void UCanalDeckMenuWidget::HandleGeneratePressed()
{
	ResolveComponents();
	if (!SeedSessionComponent)
	{
		RefreshStatus(TEXT("Missing SeedSessionComponent"));
		return;
	}

	bool bUsedManualSeed = false;
	const int32 ManualSeed = ResolveManualSeedInput(bUsedManualSeed);
	const int32 Seed = bUsedManualSeed
		? SeedSessionComponent->GenerateSeedFromInput(ManualSeed, true)
		: SeedSessionComponent->GenerateNewSeed(true);

	RefreshStatus(FString::Printf(TEXT("Generated seed %d"), Seed));
}

void UCanalDeckMenuWidget::HandleScenarioPressed()
{
	ResolveComponents();
	if (!ScenarioRunnerComponent || !SeedSessionComponent)
	{
		RefreshStatus(TEXT("Missing ScenarioRunner or SeedSession"));
		return;
	}

	const bool bStarted = ScenarioRunnerComponent->RunScenario(SeedSessionComponent->GetCurrentSeed(), false);
	RefreshStatus(bStarted ? TEXT("Scenario started") : TEXT("Scenario failed"));
}

void UCanalDeckMenuWidget::HandleStartCapturePressed()
{
	ResolveComponents();
	if (!ScenarioRunnerComponent)
	{
		RefreshStatus(TEXT("Missing ScenarioRunnerComponent"));
		return;
	}

	const int32 Seed = SeedSessionComponent ? SeedSessionComponent->GetCurrentSeed() : 0;
	const bool bStarted = ScenarioRunnerComponent->RunScenario(Seed, false);
	RefreshStatus(bStarted ? TEXT("Capture started") : TEXT("Capture start failed"));
}

void UCanalDeckMenuWidget::HandleStopCapturePressed()
{
	ResolveComponents();
	if (!ScenarioRunnerComponent)
	{
		RefreshStatus(TEXT("Missing ScenarioRunnerComponent"));
		return;
	}

	ScenarioRunnerComponent->StopScenario();
	RefreshStatus(TEXT("Capture stopped"));
}

void UCanalDeckMenuWidget::EnsureDefaultWidgetTree()
{
	if (!WidgetTree || GenerateButton || ScenarioButton || StartCaptureButton || StopCaptureButton)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	PanelBorder->SetPadding(FMargin(28.0f));
	PanelBorder->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.05f, 0.88f));

	UCanvasPanelSlot* BorderSlot = RootCanvas->AddChildToCanvas(PanelBorder);
	BorderSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	BorderSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	BorderSlot->SetAutoSize(true);

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuContent"));
	PanelBorder->SetContent(Content);

	auto CreateButtonText = [&](const FName Name, const FString& Label) -> UTextBlock*
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Text->SetText(FText::FromString(Label));
		FSlateFontInfo FontInfo = Text->GetFont();
		FontInfo.Size = FMath::RoundToInt(ButtonFontSize);
		Text->SetFont(FontInfo);
		Text->SetJustification(ETextJustify::Center);
		return Text;
	};

	auto AddDeckButton = [&](const FName ButtonName, const FString& Label, TObjectPtr<UButton>& OutButton)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);

		if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Button->Slot))
		{
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UTextBlock* LabelText = CreateButtonText(
			FName(*FString::Printf(TEXT("%s_Text"), *ButtonName.ToString())),
			Label);
		Button->AddChild(LabelText);

		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("%s_SizeBox"), *ButtonName.ToString())));
		SizeBox->SetMinDesiredHeight(MinimumButtonHeight);
		SizeBox->SetWidthOverride(620.0f);
		SizeBox->AddChild(Button);

		UVerticalBoxSlot* Slot = Content->AddChildToVerticalBox(SizeBox);
		Slot->SetPadding(FMargin(0.0f, 6.0f));
		Slot->SetHorizontalAlignment(HAlign_Fill);

		OutButton = Button;
	};

	ManualSeedTextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("ManualSeedTextBox"));
	ManualSeedTextBox->SetHintText(FText::FromString(TEXT("Manual Seed (optional)")));
	ManualSeedTextBox->SetIsReadOnly(false);
	ManualSeedTextBox->SetJustification(ETextJustify::Center);

	USizeBox* SeedInputSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ManualSeedSizeBox"));
	SeedInputSize->SetMinDesiredHeight(60.0f);
	SeedInputSize->SetWidthOverride(620.0f);
	SeedInputSize->AddChild(ManualSeedTextBox);
	UVerticalBoxSlot* SeedSlot = Content->AddChildToVerticalBox(SeedInputSize);
	SeedSlot->SetPadding(FMargin(0.0f, 6.0f));

	AddDeckButton(TEXT("GenerateButton"), TEXT("Generate"), GenerateButton);
	AddDeckButton(TEXT("ScenarioButton"), TEXT("Scenario"), ScenarioButton);
	AddDeckButton(TEXT("StartCaptureButton"), TEXT("Start Capture"), StartCaptureButton);
	AddDeckButton(TEXT("StopCaptureButton"), TEXT("Stop Capture"), StopCaptureButton);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("Ready")));
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = FMath::RoundToInt(FMath::Max(20.0f, ButtonFontSize * 0.6f));
	StatusText->SetFont(StatusFont);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.95f)));
	StatusText->SetJustification(ETextJustify::Center);

	UVerticalBoxSlot* StatusSlot = Content->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 0.0f));
}

void UCanalDeckMenuWidget::ResolveComponents()
{
	auto ResolveFromActor = [&](AActor* SourceActor)
	{
		if (!SourceActor)
		{
			return;
		}

		if (!SeedSessionComponent)
		{
			SeedSessionComponent = Cast<UCanalSeedSessionComponent>(SourceActor->GetComponentByClass(UCanalSeedSessionComponent::StaticClass()));
		}
		if (!ScenarioRunnerComponent)
		{
			ScenarioRunnerComponent = Cast<UCanalScenarioRunnerComponent>(SourceActor->GetComponentByClass(UCanalScenarioRunnerComponent::StaticClass()));
		}
	};

	ResolveFromActor(ContextActor.Get());

	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		ResolveFromActor(OwningPC);
		ResolveFromActor(OwningPC->GetPawn());
	}

	if ((!SeedSessionComponent || !ScenarioRunnerComponent) && GetWorld())
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			ResolveFromActor(*It);
			if (SeedSessionComponent && ScenarioRunnerComponent)
			{
				break;
			}
		}
	}
}

int32 UCanalDeckMenuWidget::ResolveManualSeedInput(bool& bUsedManualSeed) const
{
	bUsedManualSeed = false;
	if (!ManualSeedTextBox)
	{
		return FallbackManualSeed;
	}

	const FString SeedText = ManualSeedTextBox->GetText().ToString().TrimStartAndEnd();
	if (SeedText.IsEmpty())
	{
		return FallbackManualSeed;
	}

	int32 ParsedSeed = 0;
	if (LexTryParseString(ParsedSeed, *SeedText))
	{
		bUsedManualSeed = true;
		return FMath::Max(0, ParsedSeed);
	}

	return FallbackManualSeed;
}

void UCanalDeckMenuWidget::RefreshStatus(const FString& OverrideMessage)
{
	if (!StatusText)
	{
		return;
	}

	if (!OverrideMessage.IsEmpty())
	{
		StatusText->SetText(FText::FromString(OverrideMessage));
		return;
	}

	const int32 Seed = SeedSessionComponent ? SeedSessionComponent->GetCurrentSeed() : -1;
	const bool bCaptureRunning = ScenarioRunnerComponent && ScenarioRunnerComponent->IsScenarioRunning();
	const FString Message = FString::Printf(TEXT("Seed: %d | Capture: %s"), Seed, bCaptureRunning ? TEXT("Running") : TEXT("Idle"));
	StatusText->SetText(FText::FromString(Message));
}
