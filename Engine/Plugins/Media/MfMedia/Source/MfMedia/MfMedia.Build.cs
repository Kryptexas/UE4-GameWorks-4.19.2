// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class MfMedia : ModuleRules
	{
		public MfMedia(ReadOnlyTargetRules Target) : base(Target)
		{
			// this is for Xbox and Windows, so it's using public APIs, so we can distribute it in binary
			bOutputPubliclyDistributable = true;

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"Media",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"RenderCore",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Media",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"MfMedia/Private",
					"MfMedia/Private/Mf",
					"MfMedia/Private/Player",
				}
			);

			PublicAdditionalLibraries.Add("mfplat.lib");
			PublicAdditionalLibraries.Add("mfreadwrite.lib");
			PublicAdditionalLibraries.Add("mfuuid.lib");

			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
			{
				PublicAdditionalLibraries.Add("mf.lib");
				PublicAdditionalLibraries.Add("Propsys.lib");
				PublicAdditionalLibraries.Add("shlwapi.lib");
			}
		}
	}
}
