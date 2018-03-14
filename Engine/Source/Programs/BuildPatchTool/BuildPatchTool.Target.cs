// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64, UnrealTargetPlatform.Mac, UnrealTargetPlatform.Linux)]
[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Shipping)]
public class BuildPatchToolTarget : TargetRules
{
	public BuildPatchToolTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		LaunchModuleName = "BuildPatchTool";
		bOutputPubliclyDistributable = true;
		UndecoratedConfiguration = UnrealTargetConfiguration.Shipping;

		bBuildEditor = false;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = false;
		bCompileLeanAndMeanUE = true;
		bUseLoggingInShipping = true;
		bUseChecksInShipping = true;
		bIsBuildingConsoleApplication = true;
		bHasExports = false;
	}
}
