// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class InputBindingEditor : ModuleRules
{
	public InputBindingEditor(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
                "EditorStyle"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject", // @todo Mac: for some reason it's needed to link in debug on Mac
				"InputCore",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[]
			{
				"Editor/InputBindingEditor/Private",
				"Editor/InputBindingEditor/Private/Widgets",
			}
		);
	}
}
