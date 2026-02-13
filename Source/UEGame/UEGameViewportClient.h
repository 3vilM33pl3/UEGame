// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "UEGameViewportClient.generated.h"

UCLASS()
class UEGAME_API UUEGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual void PostRender(UCanvas* Canvas) override;

private:
	void CacheVersionText();

	FString VersionText;
	bool bVersionTextCached = false;
};
