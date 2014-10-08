// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WebBrowser : ModuleRules
{
	public WebBrowser(TargetInfo Target)
	{
		PublicIncludePaths.Add("Developer/WebBrowser/Public");
		PrivateIncludePaths.Add("Developer/WebBrowser/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"InputCore",
				"SlateCore",
				"CEF3Utils",
			}
		);

		/*if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			AddThirdPartyPrivateStaticDependencies(Target,
				"CEF3"
				);
		}*/
	}
}
