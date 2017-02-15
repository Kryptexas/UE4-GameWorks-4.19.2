// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64, UnrealTargetPlatform.Mac, UnrealTargetPlatform.Linux)]
[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Shipping)]
public class CrashReportClientTarget : TargetRules
{
	public CrashReportClientTarget(TargetInfo Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		UndecoratedConfiguration = UnrealTargetConfiguration.Shipping;
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
											InModuleNames: new List<string>() { "CrashReportClient" })
			);

		if (Target.Platform != UnrealTargetPlatform.Linux)
		{
			OutExtraModuleNames.Add("EditorStyle");
		}
	}

    public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;

		// Don't need editor
		UEBuildConfiguration.bBuildEditor = false;

		// CrashReportClient doesn't ever compile with the engine linked in
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = true;
		UEBuildConfiguration.bUseLoggingInShipping = true;

		UEBuildConfiguration.bIncludeADO = false;
		
		// Do not include ICU for Linux (this is a temporary workaround, separate headless CrashReportClient target should be created, see UECORE-14 for details).
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			UEBuildConfiguration.bCompileICU = false;
		}

		// CrashReportClient.exe has no exports, so no need to verify that a .lib and .exp file was emitted by
		// the linker.
		OutLinkEnvironmentConfiguration.bHasExports = false;

		UEBuildConfiguration.bUseChecksInShipping = true;

		// Epic Games Launcher needs to run on OS X 10.9, so CrashReportClient needs this as well
		OutCPPEnvironmentConfiguration.bEnableOSX109Support = true;
	}
}
