// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ProjectSettingsViewer : ModuleRules
	{
		public ProjectSettingsViewer(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"Engine",
					"EngineSettings",
					"SettingsEditor",
					"Slate",
					"UnrealEd",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[]
				{
					"Editor/ProjectSettingsViewer/Private",
				}
			);
		}
	}
}
