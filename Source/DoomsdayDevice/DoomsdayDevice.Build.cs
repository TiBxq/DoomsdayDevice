// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DoomsdayDevice : ModuleRules
{
	public DoomsdayDevice(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"Flow"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DoomsdayDevice",
			"DoomsdayDevice/Variant_Horror",
			"DoomsdayDevice/Variant_Horror/UI",
			"DoomsdayDevice/Variant_Shooter",
			"DoomsdayDevice/Variant_Shooter/AI",
			"DoomsdayDevice/Variant_Shooter/UI",
			"DoomsdayDevice/Variant_Shooter/Weapons"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Slate",
			"SlateCore",
            "Mover",
            "NavigationSystem",
            "GameplayTags",
            "DeveloperSettings"
        });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
