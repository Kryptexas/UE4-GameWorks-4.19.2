// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RHI : ModuleRules
{
	public RHI(TargetInfo Target)
	{
		PrivateDependencyModuleNames.Add("Core");

		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
            DynamicallyLoadedModuleNames.Add("NullDrv");

			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
			{
				DynamicallyLoadedModuleNames.Add("D3D11RHI");
			}

			if ((Target.Platform == UnrealTargetPlatform.Win32) ||
				(Target.Platform == UnrealTargetPlatform.Win64) ||
				(Target.Platform == UnrealTargetPlatform.Mac)   || 
                (Target.Platform == UnrealTargetPlatform.HTML5))
			{
				DynamicallyLoadedModuleNames.Add("OpenGLDrv");
			}
		}

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateIncludePathModuleNames.AddRange(new string[] { "TaskGraph" });
		}
	}
}
