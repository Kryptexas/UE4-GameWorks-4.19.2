// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win64, UnrealTargetPlatform.Mac)]
public class MinidumpDiagnosticsTarget : TargetRules
{
	public MinidumpDiagnosticsTarget( TargetInfo Target ) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		LaunchModuleName = "MinidumpDiagnostics";

		bCompileLeanAndMeanUE = true;
		bCompileICU = false;

		// Don't need editor
		bBuildEditor = false;

		// MinidumpDiagnostics doesn't ever compile with the engine linked in
		bCompileAgainstEngine = false;

		bIncludeADO = false;
		//bCompileICU = false;

		// MinidumpDiagnostics.exe has no exports, so no need to verify that a .lib and .exp file was emitted by the linker.
		bHasExports = false;

		// Do NOT produce additional console app exe
		bIsBuildingConsoleApplication = true;

		GlobalDefinitions.Add("MINIDUMPDIAGNOSTICS=1");
        GlobalDefinitions.Add("NOINITCRASHREPORTER=1");
    }
}
