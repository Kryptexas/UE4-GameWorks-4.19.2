// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64, UnrealTargetPlatform.Mac, UnrealTargetPlatform.Linux)]
[SupportedConfigurations(UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development, UnrealTargetConfiguration.Shipping)]
public class CrashReportClientTarget : TargetRules
{
	public CrashReportClientTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;
		UndecoratedConfiguration = UnrealTargetConfiguration.Shipping;

		LaunchModuleName = "CrashReportClient";

		if (Target.Platform != UnrealTargetPlatform.Linux)
		{
			ExtraModuleNames.Add("EditorStyle");
		}

        bOutputPubliclyDistributable = true;

		bCompileLeanAndMeanUE = true;

		// Don't need editor
		bBuildEditor = false;

		// CrashReportClient doesn't ever compile with the engine linked in
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = true;
		bUseLoggingInShipping = true;

		bIncludeADO = false;
		
		// Do not include ICU for Linux (this is a temporary workaround, separate headless CrashReportClient target should be created, see UECORE-14 for details).
		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			bCompileICU = false;
		}

		// CrashReportClient.exe has no exports, so no need to verify that a .lib and .exp file was emitted by
		// the linker.
		bHasExports = false;

		bUseChecksInShipping = true;

		// Epic Games Launcher needs to run on OS X 10.9, so CrashReportClient needs this as well
		bEnableOSX109Support = true;

		GlobalDefinitions.Add("NOINITCRASHREPORTER=1");
	}
}
