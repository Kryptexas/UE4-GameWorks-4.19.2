// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class PythonScriptPlugin : ModuleRules
	{
		public PythonScriptPlugin(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Analytics",
					"Core",
					"CoreUObject",
					"Engine",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Python",
				}
			);

			if (Target.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.AddRange(
					new string[] {
						"DesktopPlatform",
						"EditorStyle",
						"LevelEditor",
						"Slate",
						"SlateCore",
						"UnrealEd",
					}
				);
			}
		}
	}
}
