// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ReferenceViewer : ModuleRules
	{
        public ReferenceViewer(TargetInfo Target)
		{
			PrivateDependencyModuleNames.AddRange(
                new string[] {
				    "Core",
				    "CoreUObject",
				    "Engine",
                    "InputCore",
				    "Slate",
                    "EditorStyle",
				    "RenderCore",
                    "GraphEditor",
				    "UnrealEd"
			    }
			);

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
				    "AssetRegistry",
					"EditorWidgets",
					"CollectionManager",
			    }
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
				    "AssetRegistry",
					"EditorWidgets"
			    }
            );
		}
	}
}