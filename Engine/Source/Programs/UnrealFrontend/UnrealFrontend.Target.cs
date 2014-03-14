// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealFrontendTarget : TargetRules
{
	public UnrealFrontendTarget( TargetInfo Target )
	{
		Type = TargetType.Program;
	}

	//
	// TargetRules interface.
	//

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames)
	{
		OutBuildBinaryConfigurations.Add(
			new UEBuildBinaryConfiguration(
				InType: UEBuildBinaryType.Executable,
				InModuleNames: new List<string>() {
					"UnrealFrontend"
				}
			)
		);
	}

	public override bool ShouldCompileMonolithic(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
	{
		return false;
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration)
	{
		UEBuildConfiguration.bBuildEditor = false;
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = true;
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;
		UEBuildConfiguration.bCompileNetworkProfiler = false;
		UEBuildConfiguration.bForceBuildTargetPlatforms = true;
		UEBuildConfiguration.bCompileWithStatsWithoutEngine = true;
		UEBuildConfiguration.bCompileWithPluginSupport = true;

        OutLinkEnvironmentConfiguration.bBuildAdditionalConsoleApplication = false;
		OutLinkEnvironmentConfiguration.bHasExports = false;

		if (UnrealBuildTool.UnrealBuildTool.BuildingRocket())
		{
			// Tag it as a Rocket build
			OutCPPEnvironmentConfiguration.Definitions.Add("UE_ROCKET=1");
		}
	}
    public override bool GUBP_AlwaysBuildWithBaseEditor()
    {
        return true;
    }
    public override bool GUBP_NeedsPlatformSpecificDLLs()
    {
        return true;
    }
}
