// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealVersionSelectorTarget : TargetRules
{
	public UnrealVersionSelectorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		LaunchModuleName = "UnrealVersionSelector";
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
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;
		BuildConfiguration.bUseMallocProfiler = false;

		// No editor needed
		UEBuildConfiguration.bCompileICU = false;
		UEBuildConfiguration.bBuildEditor = false;
		// Editor-only data, however, is needed
		UEBuildConfiguration.bBuildWithEditorOnlyData = false;

		// Currently this app is not linking against the engine, so we'll compile out references from Core to the rest of the engine
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;

		// UnrealHeaderTool is a console application, not a Windows app (sets entry point to main(), instead of WinMain())
		//OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;
	}
}
