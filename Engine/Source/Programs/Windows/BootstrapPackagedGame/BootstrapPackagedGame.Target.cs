// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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

		bUseUnityBuild = false;
		bUseSharedPCHs = false;
		bUseMallocProfiler = false;

		// Disable all parts of the editor.
		bCompileLeanAndMeanUE = true;
		bCompileICU = false;
		bBuildEditor = false;
		bBuildWithEditorOnlyData = false;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = false;
	}
}
