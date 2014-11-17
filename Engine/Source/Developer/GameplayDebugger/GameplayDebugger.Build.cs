// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class GameplayDebugger : ModuleRules
    {
        public GameplayDebugger(TargetInfo Target)
        {
            PublicIncludePaths.AddRange(
                new string[] {
				    "Developer/GameplayDebugger/Public",
				    // ... add public include paths required here ...
			    }
            );

            PrivateIncludePaths.AddRange(
                new string[] {
					"Developer/GameplayDebugger/Private",
                    "Runtime/Engine/Private",
					// ... add other private include paths required here ...
				    }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
				{
					"Core",
					"CoreUObject",
                    "InputCore",
					"Engine",    
                    "RenderCore",
                    "RHI",
                    "AIModule",  // it have to be here for now. It'll be changed to remove any dependency to AIModule in future
				}
                );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
				    {
					    // ... add any modules that your module loads dynamically here ...
				    }
                );

            if (UEBuildConfiguration.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
            }

            if (UEBuildConfiguration.bCompileRecast)
            {
                PrivateDependencyModuleNames.Add("Navmesh");
            }
        }
    }
}
