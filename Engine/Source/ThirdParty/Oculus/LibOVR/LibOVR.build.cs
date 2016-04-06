// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LibOVR : ModuleRules
{
	// Version of LibOVR
	public const int LibOVR_Major = 1;
	public       int LibOVR_Minor = 0;
	public const int LibOVR_Patch = 0;

    public LibOVR(TargetInfo Target)
    {
        Type = ModuleType.External;

		string OculusSDKBaseDir =
			System.IO.Path.Combine( UEBuildConfiguration.UEThirdPartySourceDirectory + "Oculus/LibOVR/" +
									"/LibOVR_" + LibOVR_Major + "_" +
									LibOVR_Minor + "_" + LibOVR_Patch );

		PublicIncludePaths.Add(OculusSDKBaseDir + "/Include");
		PublicIncludePaths.Add(OculusSDKBaseDir + "/Src");
    }
}
