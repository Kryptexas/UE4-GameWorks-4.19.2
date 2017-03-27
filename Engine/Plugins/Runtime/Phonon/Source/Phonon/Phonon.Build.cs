// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Phonon : ModuleRules
	{
        public Phonon(ReadOnlyTargetRules Target) : base(Target)
        {

            OptimizeCode = CodeOptimization.Never;

            PrivateIncludePaths.AddRange(
                new string[] {
                    "Phonon/Private",
                }
            );

            PublicDependencyModuleNames.AddRange(
				new string[] {
                    "Core",
					"CoreUObject",
					"Engine",
					"AudioMixer",
                    "InputCore",
                    "RenderCore",
                    "ShaderCore",
                    "RHI"
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
                    "XAudio2",
                }
            );

            if (UEBuildConfiguration.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
                PrivateDependencyModuleNames.Add("Landscape");
            }

            AddEngineThirdPartyPrivateStaticDependencies(Target, "libPhonon");
            AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11Audio");
        }
    }
}