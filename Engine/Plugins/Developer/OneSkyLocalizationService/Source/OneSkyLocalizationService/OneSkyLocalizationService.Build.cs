// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OneSkyLocalizationService : ModuleRules
{
    public OneSkyLocalizationService(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"CoreUObject",
                "Engine",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
                "LocalizationService",
                "Json",
                "HTTP",
                "Serialization",
				"Localization",
				"LocalizationCommandletExecution",
				"MainFrame",
			}
		);
	}
}
