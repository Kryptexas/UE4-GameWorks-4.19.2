// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MixedRealityCalibration : ModuleRules
{
	public MixedRealityCalibration(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.AddRange(
			new string[] {
				"MixedRealityCalibration/Public"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"MixedRealityCalibration/Private"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] {
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"MixedRealityFramework",
				"MediaAssets",
				"InputCore",
				"OpenCV"
			}
		);
	}
}
