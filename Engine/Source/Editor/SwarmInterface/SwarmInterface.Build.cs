// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SwarmInterface : ModuleRules
{
	public SwarmInterface(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"Editor/SwarmInterface/Public"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux)
		{
			PrivateDependencyModuleNames.Add("Messaging");
		}
	}
}
