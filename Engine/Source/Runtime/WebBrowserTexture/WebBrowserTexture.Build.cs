// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class WebBrowserTexture : ModuleRules
{
	public WebBrowserTexture(ReadOnlyTargetRules Target) : base(Target)
	{
        if (Target.Platform == UnrealTargetPlatform.Android)
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
