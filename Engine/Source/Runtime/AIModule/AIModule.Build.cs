// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class AIModule : ModuleRules
	{
        public AIModule(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
                    "Runtime/AIModule/Public",
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/AIModule/Private",
                    "Runtime/Engine/Private",
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Engine",    
               		"GameplayTags",
                    "GameplayTasks"
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"RHI",
                    "RenderCore",
                    "ShaderCore",
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					// ... add any modules that your module loads dynamically here ...
				}
				);

			if (Target.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");

                PrivateDependencyModuleNames.Add("AITestSuite");                
                CircularlyReferencedDependentModules.Add("AITestSuite");
			}

            if (Target.bCompileRecast)
            {
                PrivateDependencyModuleNames.Add("Navmesh");
                PublicDefinitions.Add("WITH_RECAST=1");
            }
            else
            {
                // Because we test WITH_RECAST in public Engine header files, we need to make sure that modules
                // that import us also have this definition set appropriately.  Recast is a private dependency
                // module, so it's definitions won't propagate to modules that import Engine.
                PublicDefinitions.Add("WITH_RECAST=0");
            }

			if (Target.bBuildDeveloperTools || (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test))
			{
				PrivateDependencyModuleNames.Add("GameplayDebugger");
				PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=1");
			}
			else
			{
				PublicDefinitions.Add("WITH_GAMEPLAY_DEBUGGER=0");
			}
		}
    }
}
