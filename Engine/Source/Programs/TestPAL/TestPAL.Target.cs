// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Server)]
public class TestPALTarget : TargetRules
{
	public TestPALTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		LaunchModuleName = "TestPAL";

		// Lean and mean
		bCompileLeanAndMeanUE = true;

		// No editor or editor-only data is needed
		bBuildEditor = false;
		//bBuildWithEditorOnlyData = false;

		// Compile out references from Core to the rest of the engine
		bCompileAgainstEngine = false;	// compiling without engine is broken (overridden functions do not override base class)
		bCompileAgainstCoreUObject = false;

		// Logs are still useful to print the results
		bUseLoggingInShipping = true;

		// Make a console application under Windows, so entry point is main() everywhere
		bIsBuildingConsoleApplication = true;
	}
}
