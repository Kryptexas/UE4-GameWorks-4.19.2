// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class ShaderCacheToolTarget : TargetRules
{
	public ShaderCacheToolTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;

		LaunchModuleName = "ShaderCacheTool";
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		// Lean and mean
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;

		// Never use malloc profiling in Unreal Header Tool.  We set this because often UHT is compiled right before the engine
		// automatically by Unreal Build Tool, but if bUseMallocProfiler is defined, UHT can operate incorrectly.
		BuildConfiguration.bUseMallocProfiler = false;

		// No editor needed
		UEBuildConfiguration.bBuildEditor = false;
		// Editor-only data, however, is needed
		UEBuildConfiguration.bBuildWithEditorOnlyData = true;

		// Currently this app is not linking against the engine, so we'll compile out references from Core to the rest of the engine
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;

		// UnrealHeaderTool is a console application, not a Windows app (sets entry point to main(), instead of WinMain())
		OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;

		//UEBuildConfiguration.bCompileWithStatsWithoutEngine = true;
		BuildConfiguration.bDebugBuildsActuallyUseDebugCRT = true;
	}
}
