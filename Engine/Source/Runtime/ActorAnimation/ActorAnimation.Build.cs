// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ActorAnimation : ModuleRules
{
	public ActorAnimation(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Runtime/ActorAnimation/Private");

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
