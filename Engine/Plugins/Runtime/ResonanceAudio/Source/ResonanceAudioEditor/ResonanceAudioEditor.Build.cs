//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

namespace UnrealBuildTool.Rules
{
    public class ResonanceAudioEditor : ModuleRules
    {
        public ResonanceAudioEditor(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateIncludePaths.AddRange(
                new string[] {
                    "ResonanceAudioEditor/Private",
                    "ResonanceAudio/Private",
                }
            );

            PublicIncludePaths.AddRange(
                new string[] {
                    "ResonanceAudioEditor/Public",
                    "ResonanceAudio/Public"
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
                    "ResonanceAudio"
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
                    "ResonanceAudio"
                 }
            );

            AddEngineThirdPartyPrivateStaticDependencies(Target, "ResonanceAudioApi");
        }
    }
}
