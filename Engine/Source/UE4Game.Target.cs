// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE4GameTarget : TargetRules
{
	public UE4GameTarget( TargetInfo Target )
	{
		Type = TargetType.Game;
		// Output to Engine/Binaries/<PLATFORM> even if built as monolithic
		bOutputToEngineBinaries = true;
	}

	public override bool GetSupportedPlatforms(ref List<UnrealTargetPlatform> OutPlatforms)
	{
		return UnrealBuildTool.UnrealBuildTool.GetAllPlatforms(ref OutPlatforms, false);
	}

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutExtraModuleNames.Add("UE4Game");

		if (UnrealBuildTool.UnrealBuildTool.BuildingRocket())
		{
			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
			{
				OutExtraModuleNames.Add("OnlineSubsystemNull");
				OutExtraModuleNames.Add("OnlineSubsystemAmazon");
				if (UEBuildConfiguration.bCompileSteamOSS == true)
				{
					OutExtraModuleNames.Add("OnlineSubsystemSteam");
				}
				OutExtraModuleNames.Add("OnlineSubsystemFacebook");
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				OutExtraModuleNames.Add("OnlineSubsystemNull");
				if (UEBuildConfiguration.bCompileSteamOSS == true)
				{
					OutExtraModuleNames.Add("OnlineSubsystemSteam");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.IOS)
			{
				OutExtraModuleNames.Add("OnlineSubsystemFacebook");
				OutExtraModuleNames.Add("OnlineSubsystemIOS");
			}
			else if (Target.Platform == UnrealTargetPlatform.Android)
			{
				// @todo Rocket: Add Android online subsystem
			}
		}
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		if( UnrealBuildTool.UnrealBuildTool.BuildingRocket() )
		{
			UEBuildConfiguration.bCompileLeanAndMeanUE = true;

			// Don't need editor or editor only data
			UEBuildConfiguration.bBuildEditor = false;
			UEBuildConfiguration.bBuildWithEditorOnlyData = false;

			UEBuildConfiguration.bCompileAgainstEngine = true;

			// Tag it as a Rocket build
			OutCPPEnvironmentConfiguration.Definitions.Add("UE_ROCKET=1");

			// no exports, so no need to verify that a .lib and .exp file was emitted by the linker.
			OutLinkEnvironmentConfiguration.bHasExports = false;
		}
		else
		{ 
			// Tag it as a UE4Game build
			OutCPPEnvironmentConfiguration.Definitions.Add("UE4GAME=1");
		}
	}
    public override List<UnrealTargetPlatform> GUBP_GetPlatforms_MonolithicOnly(UnrealTargetPlatform HostPlatform)
    {
        if (HostPlatform == UnrealTargetPlatform.Mac)
        {
            return new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.IOS };
        }
        return new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Win32, UnrealTargetPlatform.IOS, UnrealTargetPlatform.XboxOne, UnrealTargetPlatform.PS4, UnrealTargetPlatform.Android };
    }
}
