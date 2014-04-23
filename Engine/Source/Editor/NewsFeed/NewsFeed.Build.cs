// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NewsFeed : ModuleRules
{
	public NewsFeed(TargetInfo Target)
	{
		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Editor/NewsFeed/Private",
				"Editor/NewsFeed/Private/Implementation",
				"Editor/NewsFeed/Private/Models",
				"Editor/NewsFeed/Private/Widgets",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Analytics",
				"Core",
				"CoreUObject",
				"DesktopPlatform",
				"EditorStyle",
				"Engine",
				"HTTP",
				"ImageWrapper",
                "InputCore",
				"OnlineSubsystem",
				"Settings",
				"Slate",
			}
		);
	}
}
