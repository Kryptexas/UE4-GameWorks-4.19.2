// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Desktop)]
[SupportedPlatforms(UnrealTargetPlatform.IOS)]
public class SlateViewerTarget : TargetRules
{
	public SlateViewerTarget(TargetInfo Target)
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
											InModuleNames: new List<string>() { "SlateViewer" } )
			);

        OutExtraModuleNames.Add("EditorStyle");
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

		// SlateViewer doesn't ever compile with the engine linked in
		UEBuildConfiguration.bCompileAgainstEngine = false;

		// We need CoreUObject compiled in as the source code access module requires it
		UEBuildConfiguration.bCompileAgainstCoreUObject = true;

		// SlateViewer.exe has no exports, so no need to verify that a .lib and .exp file was emitted by
		// the linker.
		OutLinkEnvironmentConfiguration.bHasExports = false;
	}
}
