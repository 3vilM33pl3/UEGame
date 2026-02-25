#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CanalSeedSessionSaveGame.generated.h"

UCLASS()
class UEGAME_API UCanalSeedSessionSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 CurrentSeed = 1337;

	UPROPERTY()
	TArray<int32> RecentSeeds;
};
