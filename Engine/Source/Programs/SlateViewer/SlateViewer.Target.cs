// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

[SupportedPlatforms(UnrealPlatformClass.Desktop)]
[SupportedPlatforms(UnrealTargetPlatform.IOS)]
public class SlateViewerTarget : TargetRules
{
	public SlateViewerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Monolithic;

		LaunchModuleName = "SlateViewer";
		ExtraModuleNames.Add("EditorStyle");

		bCompileLeanAndMeanUE = true;

		// Don't need editor
		bBuildEditor = false;

		// SlateViewer doesn't ever compile with the engine linked in
		bCompileAgainstEngine = false;

		// We need CoreUObject compiled in as the source code access module requires it
		bCompileAgainstCoreUObject = true;

		// SlateViewer.exe has no exports, so no need to verify that a .lib and .exp file was emitted by
		// the linker.
		bHasExports = false;
	}
}
