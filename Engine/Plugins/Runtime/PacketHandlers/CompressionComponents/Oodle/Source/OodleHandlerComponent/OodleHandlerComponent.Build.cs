// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OodleHandlerComponent : ModuleRules
{
    public OodleHandlerComponent(ReadOnlyTargetRules Target) : base(Target)
    {
        ShortName = "OodleHC";

        BinariesSubFolder = "NotForLicensees";
		
		PrivateIncludePaths.Add("OodleHandlerComponent/Private");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"PacketHandler",
				"Core",
				"CoreUObject",
				"Engine",
                "Analytics"
			});


		bool bHaveOodleSDK = false;
		string OodleNotForLicenseesLibDir = "";

		// Check the NotForLicensees folder first
		if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
        {
			OodleNotForLicenseesLibDir = System.IO.Path.Combine( Target.UEThirdPartySourceDirectory, "..", "..",
				"Plugins", "Runtime", "PacketHandlers", "CompressionComponents", "Oodle", "Source", "ThirdParty", "NotForLicensees",
				"Oodle", "255", "win", "lib" );
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			OodleNotForLicenseesLibDir = System.IO.Path.Combine( Target.UEThirdPartySourceDirectory, "..", "..",
				"Plugins", "Runtime", "PacketHandlers", "CompressionComponents", "Oodle", "Source", "ThirdParty", "NotForLicensees",
				"Oodle", "255", "linux", "lib" );
		}
		else if ( Target.Platform == UnrealTargetPlatform.PS4 )
		{
			OodleNotForLicenseesLibDir = System.IO.Path.Combine( Target.UEThirdPartySourceDirectory, "..", "..",
				"Plugins", "Runtime", "PacketHandlers", "CompressionComponents", "Oodle", "Source", "ThirdParty", "NotForLicensees",
				"Oodle", "255", "ps4", "lib" );
		}
        else if (Target.Platform == UnrealTargetPlatform.XboxOne)
        {
            OodleNotForLicenseesLibDir = System.IO.Path.Combine(Target.UEThirdPartySourceDirectory, "..", "..",
                "Plugins", "Runtime", "PacketHandlers", "CompressionComponents", "Oodle", "Source", "ThirdParty", "NotForLicensees",
                "Oodle", "255", "XboxOne", "lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            OodleNotForLicenseesLibDir = System.IO.Path.Combine(Target.UEThirdPartySourceDirectory, "..", "..",
            "Plugins", "Runtime", "PacketHandlers", "CompressionComponents", "Oodle", "Source", "ThirdParty", "NotForLicensees",
            "Oodle", "255", "Mac", "lib");
        }

        if (OodleNotForLicenseesLibDir.Length > 0)
		{
			try
			{
				bHaveOodleSDK = System.IO.Directory.Exists( OodleNotForLicenseesLibDir );
			}
			catch ( System.Exception )
			{
			}
        }

		if ( bHaveOodleSDK )
		{
	        AddEngineThirdPartyPrivateStaticDependencies(Target, "Oodle");
	        PublicIncludePathModuleNames.Add("Oodle");
			PublicDefinitions.Add( "HAS_OODLE_SDK=1" );
		}
		else
		{
			PublicDefinitions.Add( "HAS_OODLE_SDK=0" );
		}
    }
}
