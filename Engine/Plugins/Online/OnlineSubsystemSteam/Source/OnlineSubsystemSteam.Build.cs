// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemSteam : ModuleRules
{
	public OnlineSubsystemSteam(ReadOnlyTargetRules Target) : base(Target)
	{
		string SteamVersion = "Steamv139";
		bool bSteamSDKFound = Directory.Exists(Target.UEThirdPartySourceDirectory + "Steamworks/" + SteamVersion) == true;

		PublicDefinitions.Add("STEAMSDK_FOUND=" + (bSteamSDKFound ? "1" : "0"));
		PublicDefinitions.Add("WITH_STEAMWORKS=" + (bSteamSDKFound ? "1" : "0"));

		PublicDefinitions.Add("ONLINESUBSYSTEMSTEAM_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
				"Json"
			}
		);

		AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
	}
}
