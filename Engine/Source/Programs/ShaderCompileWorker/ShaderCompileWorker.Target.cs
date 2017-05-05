// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.Collections.Generic;
using System.IO;
using UnrealBuildTool;

[SupportedPlatforms(UnrealPlatformClass.Editor)]
public class ShaderCompileWorkerTarget : TargetRules
{
	public ShaderCompileWorkerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Modular;

		LaunchModuleName = "ShaderCompileWorker";
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
		// Turn off various third party features we don't need

		// Currently we force Lean and Mean mode
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;

		// ShaderCompileWorker isn't localized, so doesn't need ICU
		UEBuildConfiguration.bCompileICU = false;

		// Currently this app is not linking against the engine, so we'll compile out references from Core to the rest of the engine
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;
		UEBuildConfiguration.bBuildWithEditorOnlyData = true;
		UEBuildConfiguration.bCompileCEF3 = false;

		// Never use malloc profiling in ShaderCompileWorker.
		BuildConfiguration.bUseMallocProfiler = false;

		// Force all shader formats to be built and included.
        UEBuildConfiguration.bForceBuildShaderFormats = true;

		// ShaderCompileWorker is a console application, not a Windows app (sets entry point to main(), instead of WinMain())
		OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;

		// Disable logging, as the workers are spawned often and logging will just slow them down
		OutCPPEnvironmentConfiguration.Definitions.Add("ALLOW_LOG_FILE=0");

        // Linking against wer.lib/wer.dll causes XGE to bail when the worker is run on a Windows 8 machine, so turn this off.
        OutCPPEnvironmentConfiguration.Definitions.Add("ALLOW_WINDOWS_ERROR_REPORT_LIB=0");
	}
}
