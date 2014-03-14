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
				"DesktopPlatform",
				"ImageWrapper",
				"EditorStyle",
				"InputCore"
            }
        );
	}
}
