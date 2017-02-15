// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win64, UnrealTargetPlatform.Mac)]
public class SymbolDebuggerTarget : TargetRules
{
	public SymbolDebuggerTarget(TargetInfo Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
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
											InModuleNames: new List<string>() { "SymbolDebugger" })
			);

        OutExtraModuleNames.Add("EditorStyle");
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		OutCPPEnvironmentConfiguration.Definitions.Add("WITH_DATABASE_SUPPORT=1");

		UEBuildConfiguration.bCompileLeanAndMeanUE = true;

		// Don't need editor
		UEBuildConfiguration.bBuildEditor = false;

		// SymbolDebugger doesn't ever compile with the engine linked in
		UEBuildConfiguration.bCompileAgainstEngine = false;
		UEBuildConfiguration.bCompileAgainstCoreUObject = true;

		UEBuildConfiguration.bIncludeADO = true;

		// SymbolDebugger.exe has no exports, so no need to verify that a .lib and .exp file was emitted by
		// the linker.
		OutLinkEnvironmentConfiguration.bHasExports = false;
	}
}
