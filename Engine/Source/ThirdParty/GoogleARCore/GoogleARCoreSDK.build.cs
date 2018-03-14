// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class GoogleARCoreSDK : ModuleRules
{
	public GoogleARCoreSDK(ReadOnlyTargetRules Target) : base(Target)
	{
        Type = ModuleType.External;

		string ARCoreSDKDir = Target.UEThirdPartySourceDirectory + "GoogleARCore/";
		PublicSystemIncludePaths.AddRange(
			new string[] {
					ARCoreSDKDir + "include/",
				}
			);

		string ARCoreSDKBaseLibPath = ARCoreSDKDir + "lib/";
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			string ARCoreSDKArmLibPath = ARCoreSDKBaseLibPath + "armeabi-v7a/";
			string ARCoreSDKArm64LibPath = ARCoreSDKBaseLibPath + "arm64-v8a/";

			// toolchain will filter properly
			PublicLibraryPaths.Add(ARCoreSDKArmLibPath);
			PublicLibraryPaths.Add(ARCoreSDKArm64LibPath);

			PublicAdditionalLibraries.Add("arcore_sdk");
		}
	}
}
