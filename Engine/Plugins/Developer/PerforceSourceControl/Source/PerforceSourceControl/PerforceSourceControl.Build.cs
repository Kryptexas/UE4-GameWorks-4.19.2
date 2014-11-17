// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PerforceSourceControl : ModuleRules
{
	public PerforceSourceControl(TargetInfo Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"SourceControl",
			}
		);

		AddThirdPartyPrivateStaticDependencies(Target, "Perforce");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			AddThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
		}
	}
}
