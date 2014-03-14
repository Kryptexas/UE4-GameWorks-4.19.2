// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MessageLog : ModuleRules
{
	public MessageLog(TargetInfo Target)
	{
        PrivateIncludePaths.AddRange(
            new string[] {
				"Developer/MessageLog/Private",
				"Developer/MessageLog/Private/Presentation",
				"Developer/MessageLog/Private/UserInterface",
                "Developer/MessageLog/Private/Model",
			}
            );

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Slate",
                "InputCore",
                "EditorStyle"
			}
			);

		if (UEBuildConfiguration.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Documentation"
				}
				);		
		}
	}
}
