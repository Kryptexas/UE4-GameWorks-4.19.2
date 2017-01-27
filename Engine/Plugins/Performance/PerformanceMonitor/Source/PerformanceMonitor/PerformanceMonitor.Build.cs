// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class PerformanceMonitor : ModuleRules
    {
        public PerformanceMonitor(TargetInfo Target)
        {
            PrivateIncludePaths.AddRange
            (
                new string[] {
				"PerformanceMonitor/Private",
			}
            );

            PublicDependencyModuleNames.AddRange
            (
                new string[] {
				"Core",
                "Engine",
                "InputCore",
				"RHI",
				"RenderCore",
			}
            );

            if (UEBuildConfiguration.bBuildEditor)
            {
                PrivateDependencyModuleNames.AddRange(
                    new string[] {
					
				}
                );
            }

        }
    }
}