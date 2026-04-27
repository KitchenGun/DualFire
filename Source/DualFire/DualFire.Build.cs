// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DualFire : ModuleRules
{
	public DualFire(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(new string[] { "DualFire", "DualFire/Player", "DualFire/Core", "DualFire/Weapon", "DualFire/Weapon/Projectile", "DualFire/Enemy" });

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "DataRegistry" });

		PrivateDependencyModuleNames.AddRange(new string[] { "CommonUI", "CommonInput" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
