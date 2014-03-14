// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SwarmInterface : ModuleRules
{
	public SwarmInterface(TargetInfo Target)
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"Editor/SwarmInterface/Public"
			}
			);
		PublicDependencyModuleNames.AddRange( new string[] { "Core", "CoreUObject" } );

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicDependencyModuleNames.AddRange( new string[] { "Messaging" } );
		}
	}
}
