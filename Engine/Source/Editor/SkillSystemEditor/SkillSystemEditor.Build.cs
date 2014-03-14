// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SkillSystemEditor : ModuleRules
	{
        public SkillSystemEditor(TargetInfo Target)
		{
            PrivateIncludePaths.AddRange(
                new string[] {
					"Editor/SkillSystemTagsEditor/Private",
				}
                );


            PrivateDependencyModuleNames.AddRange(
                new string[]
				{
					// ... add private dependencies that you statically link with here ...
					"Core",
					"CoreUObject",
					"Engine",
					"AssetTools",
					"SkillSystem",
                    "InputCore",
					"Slate",
                    "EditorStyle",
					"BlueprintGraph",
					"KismetCompiler",
					"GraphEditor",
					"MainFrame",
					"UnrealEd",
				}
                );
		}
	}
}