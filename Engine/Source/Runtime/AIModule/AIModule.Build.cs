// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class AIModule : ModuleRules
	{
        public AIModule(TargetInfo Target)
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
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"RHI",
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					// ... add any modules that your module loads dynamically here ...
				}
				);

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}

            //@TODO: included always for now. Fortnite extends some classes from GDT and I have to deal with it first. (sebak)
            //if (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)
            {
                PublicDependencyModuleNames.Add("GameplayDebugger");
            }

            if (UEBuildConfiguration.bCompileRecast)
            {
                PrivateDependencyModuleNames.Add("Navmesh");
                Definitions.Add("WITH_RECAST=1");
            }
            else
            {
                // Because we test WITH_RECAST in public Engine header files, we need to make sure that modules
                // that import us also have this definition set appropriately.  Recast is a private dependency
                // module, so it's definitions won't propagate to modules that import Engine.
                Definitions.Add("WITH_RECAST=0");
            }
		}
	}
}
