// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class MediaAssets : ModuleRules
	{
		public MediaAssets(TargetInfo Target)
		{
            PublicDependencyModuleNames.AddRange(
                new string[] {
					"Core",
					"CoreUObject",
					"Engine",                
                }
            );

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Media",
                    "RenderCore",
                    "RHI",
				}
			); 

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/MediaAssets/Private",
					"Runtime/MediaAssets/Private/Assets",
				}
			);
		}
	}
}
