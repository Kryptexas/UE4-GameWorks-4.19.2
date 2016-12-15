// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.IOS)]
public class UnrealLaunchDaemonTarget : TargetRules
{
	public UnrealLaunchDaemonTarget(TargetInfo Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		bUsesSlate = false;
		//PlatformType = TargetRules.TargetPlatformType.Mobile;
		//bRequiresUnrealHeaderGeneration = true;
		AdditionalPlugins.Add("UdpMessaging");
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
		OutBuildBinaryConfigurations.Add(
			new UEBuildBinaryConfiguration(	InType: UEBuildBinaryType.Executable,
											InModuleNames: new List<string>() { "UnrealLaunchDaemon" })
			);
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;
		UEBuildConfiguration.bBuildEditor = false;
		UEBuildConfiguration.bCompileAgainstEngine = false;

		OutLinkEnvironmentConfiguration.bHasExports = false;
	}
}
