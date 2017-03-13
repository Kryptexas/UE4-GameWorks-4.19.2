// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NiagaraEditor : ModuleRules
{
	public NiagaraEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(new string[] {
			"Editor/NiagaraEditor/Private",
			"Editor/NiagaraEditor/Private/Toolkits",
			"Editor/NiagaraEditor/Private/Widgets",
			"Editor/NiagaraEditor/Private/Sequencer",
			"Editor/NiagaraEditor/Private/ViewModels",
			"Editor/NiagaraEditor/Private/TypeEditorUtilities"
		});

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Engine",
                "RHI",
                "Core", 
				"CoreUObject", 
                "InputCore",
				"RenderCore",
				"Slate", 
				"SlateCore",
				"Kismet",
                "EditorStyle",
				"UnrealEd", 
				"VectorVM",
                "Niagara",
                "MovieScene",
				"Sequencer",
				"PropertyEditor",
				"GraphEditor",
                "ShaderFormatVectorVM",
				"AppFramework",
				"MovieSceneTools",
                "MovieSceneTracks",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"Engine",
				"Messaging",
				"LevelEditor",
				"AssetTools",
				"ContentBrowser",
			}
		);

		PublicDependencyModuleNames.AddRange(
            new string[] {
				"Engine",
                "Niagara",
                "UnrealEd",
            }
        );

        PublicIncludePathModuleNames.AddRange(
            new string[] {
				"Engine",
				"Niagara"
			}
		);

        DynamicallyLoadedModuleNames.AddRange(
            new string[] {
                "WorkspaceMenuStructure",
                }
            );
	}
}
