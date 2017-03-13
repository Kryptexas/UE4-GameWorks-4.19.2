// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

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
                "Internationalization",
                "HTTP",
                "Serialization",
				"Localization",
				"MainFrame",
			}
		);

        PrivateIncludePathModuleNames.AddRange(
            new string[] {
				"LocalizationService",
				"Json",
                "Internationalization",
				"HTTP",
			}
        );
	}
}
