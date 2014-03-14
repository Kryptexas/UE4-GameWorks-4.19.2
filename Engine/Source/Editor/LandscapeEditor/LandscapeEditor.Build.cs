// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LandscapeEditor : ModuleRules
{
	public LandscapeEditor(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Slate",
                "EditorStyle",
				"Engine",
				"RenderCore",
                "InputCore",
				"UnrealEd",
				"PropertyEditor",
				"ImageWrapper",
                "EditorWidgets",
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"DesktopPlatform",
				"ContentBrowser",
                "AssetTools",
			}
			);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"MainFrame",
				"DesktopPlatform",
				"ImageWrapper",
			}
			);

		// KissFFT is used by the smooth tool.
		if (UEBuildConfiguration.bCompileLeanAndMeanUE == false &&
			(Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Mac))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "Kiss_FFT");
		}
		else
		{
			Definitions.Add("WITH_KISSFFT=0");
		}
	}
}
