// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HTTP : ModuleRules
{
	public HTTP(TargetInfo Target)
	{
		Definitions.Add("HTTP_PACKAGE=1");

		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/Online/HTTP/Private",
			}
			);

		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		if (Target.Platform == UnrealTargetPlatform.Win32 ||
			Target.Platform == UnrealTargetPlatform.Win64)
		{
			AddThirdPartyPrivateStaticDependencies(Target, "WinInet");
		}

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			AddThirdPartyPrivateStaticDependencies(Target, "libcurl");
		}
	}
}
