// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64, UnrealTargetPlatform.Mac, UnrealTargetPlatform.Linux)]
[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Shipping)]
public class BuildPatchToolTarget : TargetRules
{
	public BuildPatchToolTarget(TargetInfo Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		bOutputPubliclyDistributable = true;
		UndecoratedConfiguration = UnrealTargetConfiguration.Shipping;
	}

	//
	// TargetRules interface.
	//

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames)
	{
		OutBuildBinaryConfigurations.Add(
			new UEBuildBinaryConfiguration(
				InType: UEBuildBinaryType.Executable,
				InModuleNames: new List<string>() {
					"BuildPatchTool"
				}
			)
		);
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration)
	{
		UEBuildConfiguration.bBuildEditor = false;
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = false;
		UEBuildConfiguration.bCompileLeanAndMeanUE = true;
		UEBuildConfiguration.bUseLoggingInShipping = true;
		UEBuildConfiguration.bUseChecksInShipping = true;
		OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;
		OutLinkEnvironmentConfiguration.bHasExports = false;
	}
}
