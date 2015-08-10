// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HierarchicalLODOutliner : ModuleRules
{
    public HierarchicalLODOutliner(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange
        (
            new string[] {
				"Core",
                "InputCore",
                "CoreUObject",
				"RHI",
				"RenderCore",
				"Slate",
                "EditorStyle",
                "Engine",
                "UnrealEd",
                "PropertyEditor"
			}
        );


        PrivateDependencyModuleNames.AddRange(
             new string[] {
					"Engine",
                    "UnrealEd"
				}
         );
        
        PrivateDependencyModuleNames.AddRange(
            new string[] {
				"SlateCore",
			}
        );

        PrivateIncludePathModuleNames.AddRange(
        new string[] {
				"Messaging",
				"SessionServices",
			}
    );
	}
}
