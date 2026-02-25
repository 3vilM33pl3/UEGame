#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CanalDeckUiActor.generated.h"

class UCanalDeckMenuWidget;

UCLASS(BlueprintType, Blueprintable)
class UEGAME_API ACanalDeckUiActor : public AActor
{
	GENERATED_BODY()

public:
	ACanalDeckUiActor();

	UFUNCTION(BlueprintCallable, Category = "Canal|DeckUI")
	void ShowMenu();

	UFUNCTION(BlueprintCallable, Category = "Canal|DeckUI")
	void HideMenu();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	bool bCreateOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	int32 LocalPlayerIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	int32 ViewportZOrder = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UCanalDeckMenuWidget> DeckMenuWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canal|DeckUI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> ContextActor;

	UPROPERTY(Transient)
	TObjectPtr<UCanalDeckMenuWidget> ActiveWidget;
};
