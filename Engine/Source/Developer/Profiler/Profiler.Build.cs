// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Profiler : ModuleRules
{
	public Profiler( TargetInfo Target )
	{
		PrivateIncludePaths.AddRange
		(
			new string[] 
			{
				"Developer/Profiler/Private",
				"Developer/Profiler/Private/Widgets",
			}
		);

		PublicDependencyModuleNames.AddRange
		(
			 new string[]
			{
				"Core",
                "InputCore",
				"RHI",
				"RenderCore",
				"Slate",
                "EditorStyle",
				"Messaging",
				"SessionServices",
				"ProfilerClient",
				"DesktopPlatform",
			}
		);

		PublicIncludePaths.AddRange
		(
			new string[]
			{
				"Developer/Profiler/Public",
				"Developer/Profiler/Public/Interfaces",
			}
		);
	}
}
