// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CommonMenuExtensions : ModuleRules
{
	// TODO: Is this a minimal enough list?
	public CommonMenuExtensions(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePathModuleNames.AddRange(
			new string[] {
			}
		);

		PublicIncludePathModuleNames.AddRange(
			new string[] {
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"LauncherPlatform",
				"InputCore",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"Engine",
				"MessageLog",
				"UnrealEd", 
				"RenderCore",
				"EngineSettings",
				"HierarchicalLODOutliner",
				"HierarchicalLODUtilities",
				"MaterialShaderQualitySettings"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
			}
		);

	}
}
