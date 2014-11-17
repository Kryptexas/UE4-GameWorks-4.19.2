// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class EditorSettingsViewer : ModuleRules
	{
		public EditorSettingsViewer(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"CoreUObject",
					"Engine",
					"InputBindingEditor",
					"MessageLog",
					"SettingsEditor",
					"Slate",
					"SlateCore",
					"UnrealEd",
                    "InternationalizationSettings"
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Settings",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Editor/EditorSettingsViewer/Private",
				}
			);
		}
	}
}
