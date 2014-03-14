// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WorldBrowser : ModuleRules
{
    public WorldBrowser(TargetInfo Target)
    {
        PrivateIncludePaths.Add("Editor/WorldBrowser/Private");	// For PCH includes (because they don't work with relative paths, yet)

        PrivateIncludePathModuleNames.AddRange(
        new string[] {
				"AssetTools",
                "MeshUtilities",	
                "ContentBrowser"	
			}
        );
     
        PrivateDependencyModuleNames.AddRange(
            new string[] {    
                "Core", 
                "CoreUObject", 
                "RenderCore", 
                "InputCore",
                "Engine", 
                "Slate", 
                "EditorStyle",
                "UnrealEd",
                "GraphEditor", 
                "LevelEditor", 
                "PropertyEditor", 
                "DesktopPlatform",
                "MainFrame",
                "SourceControl",
				"SourceControlWindows",
                "RawMesh",
                "MeshUtilities"
            }
            );

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
				"AssetTools",
				"SceneOutliner",
                "MeshUtilities",
                "ContentBrowser"
			}
            );
    }
}