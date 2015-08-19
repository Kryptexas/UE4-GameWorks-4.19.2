// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SlateReflector : ModuleRules
{
	public SlateReflector(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"CoreUObject",
				"InputCore",
				"Slate",
				"SlateCore",
				"Json",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/SlateReflector/Private",
				"Developer/SlateReflector/Private/Models",
				"Developer/SlateReflector/Private/Widgets",
			}
		);

		// DesktopPlatform is only available for Editor and Program targets (running on a desktop platform)
		bool IsDesktopPlatformType = Target.Platform == UnrealBuildTool.UnrealTargetPlatform.Win32
			|| Target.Platform == UnrealBuildTool.UnrealTargetPlatform.Win64
			|| Target.Platform == UnrealBuildTool.UnrealTargetPlatform.Mac
			|| Target.Platform == UnrealBuildTool.UnrealTargetPlatform.Linux;
		if (Target.Type == TargetRules.TargetType.Editor || (Target.Type == TargetRules.TargetType.Program && IsDesktopPlatformType))
		{
			Definitions.Add("SLATE_REFLECTOR_HAS_DESKTOP_PLATFORM=1");

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"DesktopPlatform",
				}
			);
		}
		else
		{
			Definitions.Add("SLATE_REFLECTOR_HAS_DESKTOP_PLATFORM=0");
		}
	}
}
