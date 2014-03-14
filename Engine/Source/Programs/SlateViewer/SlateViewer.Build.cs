// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SlateViewer : ModuleRules
{
	public SlateViewer(TargetInfo Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateIncludePathModuleNames.AddRange(new string[] { "Slate" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "Slate", "StandaloneRenderer", "Projects" });

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PrivateDependencyModuleNames.Add("NetworkFile");
			PrivateDependencyModuleNames.Add("StreamingFile");
		}
	}
}
