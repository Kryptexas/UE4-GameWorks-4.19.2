// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DesktopPlatform : ModuleRules
{
	public DesktopPlatform(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Developer/DesktopPlatform/Private");

		PrivateDependencyModuleNames.Add("Core");

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
            AddThirdPartyPrivateStaticDependencies(Target, "LinuxNativeDialogs");
            AddThirdPartyPrivateStaticDependencies(Target, "SDL2");
		}
	}
}
