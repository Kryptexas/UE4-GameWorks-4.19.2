// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ExternalImagePicker : ModuleRules
{
	public ExternalImagePicker(TargetInfo Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
				"Core",
                "Slate",
				"SlateCore",
				"DesktopPlatform",
				"ImageWrapper",
				"EditorStyle",
				"InputCore",
				"PropertyEditor",	// for 'reset to default'
            }
        );
	}
}
