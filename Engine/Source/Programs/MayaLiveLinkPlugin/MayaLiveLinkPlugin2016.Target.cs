// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public abstract class MayaLiveLinkPluginTargetBase : TargetRules
{
	public MayaLiveLinkPluginTargetBase(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;

		OverrideExecutableFileExtension = ".mll";   // Maya requires plugin binaries to have the ".mll" extension
		bShouldCompileAsDLL = true;
		LinkType = TargetLinkType.Monolithic;

		// We only need minimal use of the engine for this plugin
		bCompileLeanAndMeanUE = true;
		bUseMallocProfiler = false;
		bBuildEditor = false;
		bBuildWithEditorOnlyData = true;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = true;
        bCompileICU = false;
        bHasExports = true;

		bBuildInSolutionByDefault = false;
	}
}

public class MayaLiveLinkPlugin2016Target : MayaLiveLinkPluginTargetBase
{
	public MayaLiveLinkPlugin2016Target(TargetInfo Target) : base(Target)
	{
		LaunchModuleName = "MayaLiveLinkPlugin2016";
	}
}