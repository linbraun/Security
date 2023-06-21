// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GAM312_Final : ModuleRules
{
	public GAM312_Final(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "GameplayTasks", "AIModule", "NavigationSystem", "UMG", "Slate", "SlateCore" });
	}
}
