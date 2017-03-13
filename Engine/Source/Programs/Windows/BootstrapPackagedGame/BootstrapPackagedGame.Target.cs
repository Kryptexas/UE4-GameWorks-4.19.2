// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Shipping)]
public class BootstrapPackagedGameTarget : TargetRules
{
	public BootstrapPackagedGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		LaunchModuleName = "BootstrapPackagedGame";

		bUseStaticCRT = true;
	}

	//
	// TargetRules interface.
	//

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		BuildConfiguration.bUseUnityBuild = false;
		BuildConfiguration.bUseSharedPCHs = false;
		BuildConfiguration.bUseMallocProfiler = false;

		// Disable all parts of the editor.
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;
		UEBuildConfiguration.bCompileICU = false;
		UEBuildConfiguration.bBuildEditor = false;
		UEBuildConfiguration.bBuildWithEditorOnlyData = false;
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;
	}
}
