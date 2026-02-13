// Copyright Epic Games, Inc. All Rights Reserved.

#include "SteamDeckDeviceProfileSelectorModule.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProperties.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FSteamDeckDeviceProfileSelectorModule, SteamDeckDeviceProfileSelector);

static bool FileContainsTokenCaseInsensitive(const FString& Path, const FString& Token)
{
	FString Contents;
	if (!FFileHelper::LoadFileToString(Contents, *Path))
	{
		return false;
	}

	return Contents.Contains(Token, ESearchCase::IgnoreCase);
}

static bool IsSteamDeckHardware()
{
#if PLATFORM_LINUX
	// SteamOS reports VARIANT_ID=steamdeck in /etc/os-release.
	if (FileContainsTokenCaseInsensitive(TEXT("/etc/os-release"), TEXT("VARIANT_ID=steamdeck")))
	{
		return true;
	}

	// DMI product names seen on Steam Deck hardware.
	FString ProductName;
	if (FFileHelper::LoadFileToString(ProductName, TEXT("/sys/devices/virtual/dmi/id/product_name")))
	{
		ProductName = ProductName.TrimStartAndEnd();
		if (ProductName.Equals(TEXT("Jupiter"), ESearchCase::IgnoreCase) ||
			ProductName.Equals(TEXT("Galileo"), ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
#endif

	return false;
}

const FString FSteamDeckDeviceProfileSelectorModule::GetRuntimeDeviceProfileName()
{
#if PLATFORM_LINUX
	if (IsSteamDeckHardware())
	{
		UE_LOG(LogInit, Log, TEXT("Selected Device Profile: [SteamDeck]"));
		return TEXT("SteamDeck");
	}
#endif

	const FString ProfileName = FPlatformProperties::PlatformName();
	UE_LOG(LogInit, Log, TEXT("Selected Device Profile: [%s]"), *ProfileName);
	return ProfileName;
}
