// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Mac)]
public class UE4EditorServicesTarget : TargetRules
{
	public UE4EditorServicesTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		LaunchModuleName = "UE4EditorServices";

		bCompileLeanAndMeanUE = true;
		bUseMallocProfiler = false;

		// No editor needed
		bCompileICU = false;
		bBuildEditor = false;
		// Editor-only data, however, is needed
		bBuildWithEditorOnlyData = false;

		// Currently this app is not linking against the engine, so we'll compile out references from Core to the rest of the engine
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = false;

		// We still need to support old versions of the engine that are compatible with OS X 10.9
		bEnableOSX109Support = true;
	}
}
