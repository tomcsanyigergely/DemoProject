// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FastPacedShooter : ModuleRules
{
	public FastPacedShooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystemUtils", "AnimGraphRuntime" });
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UMG" });
	}
}
