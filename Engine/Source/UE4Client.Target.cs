// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE4ClientTarget : TargetRules
{

    public UE4ClientTarget(TargetInfo Target)
	{
		Type = TargetType.Client;
        bOutputToEngineBinaries = true;
	}

	//
	// TargetRules interface.
	//
	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		base.SetupBinaries(Target, ref OutBuildBinaryConfigurations, ref OutExtraModuleNames);
		OutExtraModuleNames.Add("UE4Game");
	}

	public override void SetupGlobalEnvironment(
        TargetInfo Target,
        ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
        ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
        )
    {
        if (UnrealBuildTool.UnrealBuildTool.BuildingRocket())
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
        return new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Win32 /*, UnrealTargetPlatform.IOS, UnrealTargetPlatform.XboxOne, UnrealTargetPlatform.PS4 */};
    }

    public override List<UnrealTargetConfiguration> GUBP_GetConfigs_MonolithicOnly(UnrealTargetPlatform HostPlatform, UnrealTargetPlatform Platform)
    {
        return new List<UnrealTargetConfiguration> { UnrealTargetConfiguration.Development };
    }
}
