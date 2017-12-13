//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

namespace UnrealBuildTool.Rules
{
    public class ResonanceAudio : ModuleRules
    {
        public ResonanceAudio(ReadOnlyTargetRules Target) : base(Target)
        {

            OptimizeCode = CodeOptimization.Never;

            PublicIncludePaths.AddRange(
                new string[] {
                    "ResonanceAudio/Public"
                }
            );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "ResonanceAudio/Private",
                }
            );

            PublicDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "AudioMixer",
                    "ProceduralMeshComponent",
                }
            );

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                    "TargetPlatform",
                }
            );

            PrivateDependencyModuleNames.AddRange(
                new string[] {
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "Projects",
                    "AudioMixer",
                    "ProceduralMeshComponent",
                }
            );

            if (Target.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
                PrivateDependencyModuleNames.Add("Landscape");
                PrivateDependencyModuleNames.Add("ProceduralMeshComponent");
            }

            AddEngineThirdPartyPrivateStaticDependencies(Target, "ResonanceAudioApi");
        }
    }
}
