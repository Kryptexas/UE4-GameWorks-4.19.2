// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OodleHandlerComponent : ModuleRules
{
    public OodleHandlerComponent(TargetInfo Target)
    {
		BinariesSubFolder = "NotForLicensees";
		
		PrivateIncludePaths.Add("OodleHandlerComponent/Private");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"PacketHandler",
				"Core",
				"CoreUObject",
				"Engine"
			});


		bool bHaveOodleSDK = false;

		if ( ( Target.Platform == UnrealTargetPlatform.Win64 ) || ( Target.Platform == UnrealTargetPlatform.Win32 ) )
        {
			// Check the NotForLicensees folder first
			string OodleNotForLicenseesLibDir = System.IO.Path.Combine( UEBuildConfiguration.UEThirdPartySourceDirectory, "..", "..",
				"Plugins", "Runtime", "PacketHandlers", "CompressionComponents", "Oodle", "Source", "ThirdParty", "NotForLicensees",
				"Oodle", "win", "lib" );

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
	        AddThirdPartyPrivateStaticDependencies(Target,"Oodle");
	        PublicIncludePathModuleNames.Add("Oodle");
			Definitions.Add( "HAS_OODLE_SDK=1" );
		}
		else
		{
			Definitions.Add( "HAS_OODLE_SDK=0" );
		}
    }
}
