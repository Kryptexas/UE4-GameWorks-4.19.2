// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class TargetPlatform : ModuleRules
{
	public TargetPlatform(TargetInfo Target)
	{
		PrivateDependencyModuleNames.Add("Core");

		if (!UEBuildConfiguration.bBuildRequiresCookedData)
		{
            // these are needed by multiple platform specific target platforms, so we make sure they are built with the base editor
            DynamicallyLoadedModuleNames.Add("ShaderPreprocessor");
            DynamicallyLoadedModuleNames.Add("ShaderFormatOpenGL");
            DynamicallyLoadedModuleNames.Add("ImageWrapper");

            if (Target.Platform == UnrealTargetPlatform.Win32 ||
                Target.Platform == UnrealTargetPlatform.Win64 ||
				(Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32"))
			{

                // these are needed by multiple platform specific target platforms, so we make sure they are built with the base editor
                DynamicallyLoadedModuleNames.Add("ShaderFormatD3D");

                if (UEBuildConfiguration.bCompileLeanAndMeanUE == false)
				{
                    DynamicallyLoadedModuleNames.Add("TextureFormatDXT");
                    DynamicallyLoadedModuleNames.Add("TextureFormatPVR");
				}

				DynamicallyLoadedModuleNames.Add("TextureFormatUncompressed");

				if (UEBuildConfiguration.bCompileAgainstEngine)
				{
					DynamicallyLoadedModuleNames.Add("AudioFormatADPCM"); // For IOS cooking
					DynamicallyLoadedModuleNames.Add("AudioFormatOgg");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				if (UEBuildConfiguration.bCompileLeanAndMeanUE == false)
				{
					DynamicallyLoadedModuleNames.Add("TextureFormatDXT");
                    DynamicallyLoadedModuleNames.Add("TextureFormatPVR");
				}

				DynamicallyLoadedModuleNames.Add("TextureFormatUncompressed");

				if (UEBuildConfiguration.bCompileAgainstEngine)
				{
					DynamicallyLoadedModuleNames.Add("AudioFormatOgg");
				}
			}

			if (UEBuildConfiguration.bCompileAgainstEngine && UEBuildConfiguration.bCompilePhysX)
			{
				DynamicallyLoadedModuleNames.Add("PhysXFormats");
			}
		}
	}
}
