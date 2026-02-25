#include "CanalGen/CanalDeckUiActor.h"

#include "CanalGen/CanalDeckMenuWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ACanalDeckUiActor::ACanalDeckUiActor()
{
	PrimaryActorTick.bCanEverTick = false;
	DeckMenuWidgetClass = UCanalDeckMenuWidget::StaticClass();
}

void ACanalDeckUiActor::BeginPlay()
{
	Super::BeginPlay();

	if (bCreateOnBeginPlay)
	{
		ShowMenu();
	}
}

void ACanalDeckUiActor::ShowMenu()
{
	if (ActiveWidget)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, LocalPlayerIndex);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("Deck UI actor could not find local player controller index %d."), LocalPlayerIndex);
		return;
	}

	TSubclassOf<UCanalDeckMenuWidget> WidgetClassToCreate = DeckMenuWidgetClass;
	if (!WidgetClassToCreate)
	{
		WidgetClassToCreate = UCanalDeckMenuWidget::StaticClass();
	}
	ActiveWidget = CreateWidget<UCanalDeckMenuWidget>(PC, WidgetClassToCreate);
	if (!ActiveWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("Deck UI actor failed to create deck menu widget."));
		return;
	}

	ActiveWidget->InitializeFromActor(ContextActor ? ContextActor.Get() : this);
	ActiveWidget->AddToViewport(ViewportZOrder);

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->SetShowMouseCursor(true);
}

void ACanalDeckUiActor::HideMenu()
{
	if (!ActiveWidget)
	{
		return;
	}

	ActiveWidget->RemoveFromParent();
	ActiveWidget = nullptr;
}
