// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SteamDeckDeviceProfileSelector : ModuleRules
{
	public SteamDeckDeviceProfileSelector(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine"
			}
		);
	}
}
