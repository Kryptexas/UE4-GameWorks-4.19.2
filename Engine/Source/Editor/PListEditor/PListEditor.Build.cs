// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PListEditor : ModuleRules
{
	public PListEditor( TargetInfo Target )
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{ 
				"Core", 
				"CoreUObject", 
				"Engine", 
                "InputCore",
				"Slate", 
                "EditorStyle",
				"UnrealEd", 
				"DesktopPlatform",
				"XmlParser"
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"MainFrame",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"MainFrame",
			}
		);
	}
}
