// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class GoogleVR : ModuleRules
{
	public GoogleVR(TargetInfo Target)
	{
		Type = ModuleType.External;

		string GoogleVRSDKDir = UEBuildConfiguration.UEThirdPartySourceDirectory + "GoogleVR/";
		PublicSystemIncludePaths.AddRange(
			new string[] {
					GoogleVRSDKDir + "include",
					GoogleVRSDKDir + "include/vr/gvr/capi/include",
				}
			);

		string GoogleVRBaseLibPath = GoogleVRSDKDir + "lib/";

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			string GoogleVRArmLibPath = GoogleVRBaseLibPath + "android_arm/";
			string GoogleVRArm64LibPath = GoogleVRBaseLibPath + "android_arm64/";
			string GoogleVRx86LibPath = GoogleVRBaseLibPath + "android_x86/";
			string GoogleVRx86_64LibPath = GoogleVRBaseLibPath + "android_x86_64/";

			// toolchain will filter properly
			PublicLibraryPaths.Add(GoogleVRArmLibPath);
			PublicLibraryPaths.Add(GoogleVRArm64LibPath);
			PublicLibraryPaths.Add(GoogleVRx86LibPath);
			PublicLibraryPaths.Add(GoogleVRx86_64LibPath);

			PublicAdditionalLibraries.Add("gvr");
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			string GoogleVRIOSLibPath = GoogleVRBaseLibPath + "ios/";
			PublicLibraryPaths.Add(GoogleVRIOSLibPath);
			PublicAdditionalLibraries.Add(GoogleVRIOSLibPath + "libgvr.a");
		}
	}
}
