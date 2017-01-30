// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealFrontendTarget : TargetRules
{
	public UnrealFrontendTarget( TargetInfo Target ) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Modular;
		AdditionalPlugins.Add("UdpMessaging");
		LaunchModuleName = "UnrealFrontend";
	}

	//
	// TargetRules interface.
	//

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
