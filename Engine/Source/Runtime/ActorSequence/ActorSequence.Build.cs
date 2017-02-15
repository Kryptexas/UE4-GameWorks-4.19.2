// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActorSequence : ModuleRules
{
	public ActorSequence(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Runtime/ActorSequence/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"MovieScene",
			}
		);
	}
}
