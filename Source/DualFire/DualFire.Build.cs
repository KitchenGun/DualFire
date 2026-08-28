// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DualFire : ModuleRules
{
	public DualFire(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add("DualFire");

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "CommonUI", "UMG", "Paper2D" });

		PrivateDependencyModuleNames.AddRange(new string[] { "CommonInput", "EngineSettings", "GameplayTags", "Slate", "SlateCore" });
	}
}
