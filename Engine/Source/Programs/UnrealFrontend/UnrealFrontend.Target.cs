// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealFrontendTarget : TargetRules
{
	public UnrealFrontendTarget( TargetInfo Target )
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Modular;
		AdditionalPlugins.Add("UdpMessaging");
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

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration)
	{
		UEBuildConfiguration.bBuildEditor = false;
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = true;
		UEBuildConfiguration.bForceBuildTargetPlatforms = true;
		UEBuildConfiguration.bCompileWithStatsWithoutEngine = true;
		UEBuildConfiguration.bCompileWithPluginSupport = true;

		OutLinkEnvironmentConfiguration.bHasExports = false;
	}
}
