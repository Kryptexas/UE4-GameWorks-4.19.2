// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class WmfMedia : ModuleRules
	{
		public WmfMedia(ReadOnlyTargetRules Target) : base(Target)
		{
            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                    "Media",
				}
            );

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"RenderCore",
					"WmfMediaFactory",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
                    "Media",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"WmfMedia/Private",
                    "WmfMedia/Private/Player",
					"WmfMedia/Private/Wmf",
				}
			);

			if (Target.bCompileAgainstEngine)
			{
				PrivateDependencyModuleNames.Add("HeadMountedDisplay");
			}

			if ((Target.Platform == UnrealTargetPlatform.Win64) ||
				(Target.Platform == UnrealTargetPlatform.Win32))
            {
                PublicDelayLoadDLLs.Add("mf.dll");
                PublicDelayLoadDLLs.Add("mfplat.dll");
                PublicDelayLoadDLLs.Add("mfplay.dll");
                PublicDelayLoadDLLs.Add("mfuuid.dll");
                PublicDelayLoadDLLs.Add("shlwapi.dll");
            }
		}
	}
}
