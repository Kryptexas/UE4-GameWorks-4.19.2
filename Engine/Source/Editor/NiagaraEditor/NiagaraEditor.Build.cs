// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NiagaraEditor : ModuleRules
{
	public NiagaraEditor(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
                "InputCore",
				"RenderCore",
				"Slate", 
                "EditorStyle",
				"UnrealEd", 
				"GraphEditor", 
				"VectorVM"
			}
			);

		PublicIncludePathModuleNames.Add("LevelEditor");

		DynamicallyLoadedModuleNames.Add("WorkspaceMenuStructure");

	}
}
