// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SlackIntegrations : ModuleRules
{
    public SlackIntegrations(ReadOnlyTargetRules Target) : base(Target)
    {
		PublicIncludePaths.Add("Developer/SlackIntegrations/Public");

		PrivateIncludePaths.Add("Developer/SlackIntegrations/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
            new string[] {
				"HTTP",
			}
		);
    }
}

