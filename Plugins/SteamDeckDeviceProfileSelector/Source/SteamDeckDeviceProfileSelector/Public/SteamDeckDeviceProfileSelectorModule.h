// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDeviceProfileSelectorModule.h"

class FSteamDeckDeviceProfileSelectorModule final : public IDeviceProfileSelectorModule
{
public:
	virtual const FString GetRuntimeDeviceProfileName() override;
};
