// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SubversionSourceControl : ModuleRules
{
	public SubversionSourceControl(TargetInfo Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"Core",
				"Slate",
                "EditorStyle",
				"SourceControl",
				"XmlParser",
		    }
            );
	}
}
