// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CrashReportClientTarget : TargetRules
{
	public CrashReportClientTarget(TargetInfo Target)
	{
		Type = TargetType.Program;
	}

	//
	// TargetRules interface.
	//
	public override bool GetSupportedPlatforms(ref List<UnrealTargetPlatform> OutPlatforms)
	{
        OutPlatforms.Add(UnrealTargetPlatform.Win32);
        OutPlatforms.Add(UnrealTargetPlatform.Win64);
		OutPlatforms.Add(UnrealTargetPlatform.Mac);
		OutPlatforms.Add(UnrealTargetPlatform.Linux);
		return true;
	}

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutBuildBinaryConfigurations.Add(
			new UEBuildBinaryConfiguration(	InType: UEBuildBinaryType.Executable,
											InModuleNames: new List<string>() { "CrashReportClient" })
			);

		if (Target.Platform != UnrealTargetPlatform.Linux)
		{
			OutExtraModuleNames.Add("EditorStyle");
		}
	}

    public override bool ShouldCompileMonolithic(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
    {
        return true;
    }

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		UEBuildConfiguration.bCompileNetworkProfiler = false;

		UEBuildConfiguration.bCompileLeanAndMeanUE = true;

		// Don't need editor
		UEBuildConfiguration.bBuildEditor = false;

		// CrashReportClient doesn't ever compile with the engine linked in
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;

		UEBuildConfiguration.bIncludeADO = (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32);

		// CrashReportClient.exe has no exports, so no need to verify that a .lib and .exp file was emitted by
		// the linker.
		OutLinkEnvironmentConfiguration.bHasExports = false;

		// Do NOT produce additional console app exe
		OutLinkEnvironmentConfiguration.bBuildAdditionalConsoleApplication = false;

        if (UnrealBuildTool.UnrealBuildTool.BuildingRocket())
        {
            // Tag it as a Rocket build
            OutCPPEnvironmentConfiguration.Definitions.Add("UE_ROCKET=1");
        }
	}
}
