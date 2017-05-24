// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64)]
[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Shipping)]
public class UnrealWatchdogTarget : TargetRules
{
	public UnrealWatchdogTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		UndecoratedConfiguration = UnrealTargetConfiguration.Shipping;
		LaunchModuleName = "UnrealWatchdog";
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
		// Lean and mean
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;
		BuildConfiguration.bUseMallocProfiler = false;

        UEBuildConfiguration.bUseLoggingInShipping = true;

        // No editor needed
        UEBuildConfiguration.bBuildEditor = false;
		UEBuildConfiguration.bBuildWithEditorOnlyData = false;

		// Currently this app is not linking against the engine, so we'll compile out references from Core to the rest of the engine
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;
		UEBuildConfiguration.bBuildDeveloperTools = false;

		// Console application, not a Windows app (sets entry point to main(), instead of WinMain())
		OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;
	}
}
