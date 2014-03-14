// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


using UnrealBuildTool;


public class Facebook : ModuleRules
{
    public Facebook(TargetInfo Target)
    {
        Type = ModuleType.External;

        string FacebookPath = UEBuildConfiguration.UEThirdPartyDirectory + "Facebook/";
        Definitions.Add("WITH_FACEBOOK=1");

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            FacebookPath += "Facebook-IOS-3.7/";

            PublicIncludePaths.Add(FacebookPath + "Include");

			string LibDir = FacebookPath + "Lib/Release" + Target.Architecture;

            PublicLibraryPaths.Add(LibDir);
            PublicAdditionalLibraries.Add("Facebook-IOS-3.7");

            PublicAdditionalShadowFiles.Add(LibDir + "/libFacebook-IOS-3.7.a");

            // Needed for the facebook sdk to link.
            PublicFrameworks.Add("CoreGraphics");
        }
    }
}
