// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemSteam : ModuleRules
{
	public OnlineSubsystemSteam(TargetInfo Target)
	{
		Definitions.Add("ONLINESUBSYSTEMSTEAM_PACKAGE=1");

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"OnlineSubsystemUtils",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"Sockets", 
				"Voice",
				"OnlineSubsystem", 
			}
		);

		AddThirdPartyPrivateStaticDependencies(Target, "Steamworks");

		Definitions.Add("WITH_STEAMGC=0");

#if false
		if (Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees") == true &&
			Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees/SteamGC") == true)
		{
			AddThirdPartyPrivateStaticDependencies(Target, 
				"ProtoBuf",
				"SteamGC"
				);
			Definitions.Add("WITH_STEAMGC=1");
		}
		else
		{
			Definitions.Add("WITH_STEAMGC=0");
		}
#endif
	}
}
