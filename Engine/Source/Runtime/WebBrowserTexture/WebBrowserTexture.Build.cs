// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class WebBrowserTexture : ModuleRules
{
	public WebBrowserTexture(ReadOnlyTargetRules Target) : base(Target)
	{
        // WebBrowserTexture objects are needed only on Android, but we also need to be able to
        // cook the asset so we must include it in editor builds
        if (Target.Platform == UnrealTargetPlatform.Android || Target.bBuildEditor == true)
		{
			PublicIncludePaths.Add("Runtime/WebBrowserTexture/Public");
			PrivateIncludePaths.Add("Runtime/WebBrowserTexture/Private");
			
			// Needed for external texture support
			PublicIncludePaths.AddRange(
				new string[] {
				"Runtime/MediaUtils/Public",
				});

			PrivateIncludePaths.AddRange(
				new string[] {
				"Runtime/MediaUtils/Private",
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"ApplicationCore",
					"RHI",
					"InputCore",
					"Slate",
					"SlateCore",
					"Serialization",
					"MediaUtils",
					"RenderCore",
					"Engine",
					"ShaderCore",
					"UtilityShaders",
					"WebBrowser"
				}
			);
		}
		else
		{
			PrecompileForTargets = ModuleRules.PrecompileTargetsType.None;
		}
	}
}
