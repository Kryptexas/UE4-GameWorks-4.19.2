// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Matinee : ModuleRules
{
	public Matinee(TargetInfo Target)
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"Editor/DistCurveEditor/Public",
				"Editor/UnrealEd/Public",
				"Developer/DesktopPlatform/Public",
			}
			);

		PrivateIncludePaths.Add("Editor/UnrealEd/Private");	//compatibility for FBX exporter

		PrivateIncludePathModuleNames.Add("UnrealEd");	//compatibility for FBX exporter

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
                "EditorStyle",
                "InputCore",
				"LevelEditor",
				"DistCurveEditor",
				"UnrealEd",
				"RenderCore",
				"AssetRegistry",
                "ContentBrowser"
			}
			);

		AddThirdPartyPrivateStaticDependencies(Target, "FBX");

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"PropertyEditor",
                "ContentBrowser"
				}
			);
	}
}
