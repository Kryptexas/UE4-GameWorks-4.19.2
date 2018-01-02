// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SymbolDebugger : ModuleRules
{
	public SymbolDebugger(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ApplicationCore",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"StandaloneRenderer",
				"DesktopPlatform",
				"CrashDebugHelper",
				"SourceControl",
                "PerforceSourceControl",
				"MessageLog",
				"Projects",
			}
		);

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include

		// Need database support!
		PublicDefinitions.Add("WITH_DATABASE_SUPPORT=1");
	}
}
