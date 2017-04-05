// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class PhononEditor : ModuleRules
	{
        public PhononEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.AddRange(
                new string[] {
                    "PhononEditor/Private",
                    "Phonon/Private",
                    "Phonon/Public"
                }
            );

            PublicIncludePaths.AddRange(
                new string[] {
                    "Phonon/Public"
                }
            );


            PublicDependencyModuleNames.AddRange(
				new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "UnrealEd",
                    "LevelEditor",
                    "EditorStyle",
                    "RenderCore",
                    "ShaderCore",
                    "RHI",
                    "AudioEditor",
                    "AudioMixer",
                    "Phonon"
				}
			);

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                   "AssetTools",
                   "Landscape"
            });

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Slate",
                    "SlateCore",
                    "UnrealEd",
                    "AudioEditor",
                    "LevelEditor",
                    "Landscape",
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "PropertyEditor",
                    "Projects",
                    "EditorStyle",
                    "Phonon",
                    "ComponentVisualizers"
                 }
            );

            AddEngineThirdPartyPrivateStaticDependencies(Target, "libPhonon");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11Audio");
        }
    }
}