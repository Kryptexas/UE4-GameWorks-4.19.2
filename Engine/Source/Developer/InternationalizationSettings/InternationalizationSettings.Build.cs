// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class InternationalizationSettings : ModuleRules
	{
        public InternationalizationSettings(TargetInfo Target)
		{
            PrivateDependencyModuleNames.AddRange(
                new string[] {
				    "Core",
				    "CoreUObject",
				    "InputCore",
				    "Engine",
				    "Slate",
				    "EditorStyle",
				    "PropertyEditor",
				    "SharedSettingsWidgets",
                }
            );

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Settings",
				}
			);
		}
	}
}
