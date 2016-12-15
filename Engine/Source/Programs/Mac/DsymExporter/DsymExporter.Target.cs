// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Mac)]
public class DsymExporterTarget : TargetRules
{
	public DsymExporterTarget( TargetInfo Target )
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
	}

	// TargetRules interface.

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		OutBuildBinaryConfigurations.Add(
			new UEBuildBinaryConfiguration(	InType: UEBuildBinaryType.Executable,
				InModuleNames: new List<string>() { "DsymExporter" })
			);
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

		// DsymExporter doesn't ever compile with the engine linked in
		UEBuildConfiguration.bCompileAgainstEngine = false;

		// DsymExporter has no exports, so no need to verify that a .lib and .exp file was emitted by the linker.
		OutLinkEnvironmentConfiguration.bHasExports = false;

        UEBuildConfiguration.bCompileAgainstCoreUObject = false;

		// Do NOT produce additional console app exe
		OutLinkEnvironmentConfiguration.bIsBuildingConsoleApplication = true;
	}
}
