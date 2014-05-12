// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class Box2D : ModuleRules
{
	public Box2D(TargetInfo Target)
	{
		Type = ModuleType.External;

		// Determine the root directory of Box2D
		string ModuleCSFilename = UnrealBuildTool.RulesCompiler.GetModuleFilename(this.GetType().Name);
		string ModuleBaseDirectory = Path.GetDirectoryName(ModuleCSFilename);
		string Box2DBaseDir = Path.Combine(ModuleBaseDirectory, "Box2D_v2.2.1");

		// Add the libraries for the current platform
		bool bSupported = false;
		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{
			bSupported = true;

			string WindowsVersion = "vs" + WindowsPlatform.GetVisualStudioCompilerVersionName();
			string Box2DLibDir = Path.Combine(Box2DBaseDir, "build", WindowsVersion, "lib");
			PublicLibraryPaths.Add(Box2DLibDir);

			string PlatformString = (Target.Platform == UnrealTargetPlatform.Win64) ? "x64" : "x86";
			string LibName = "Box2D_";
			if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
			{
				LibName += "DEBUG_";
			}
			LibName += PlatformString + ".lib";
			PublicAdditionalLibraries.Add(LibName);
		}

		// Box2D included define
		Definitions.Add(string.Format("WITH_BOX2D={0}", bSupported ? 1 : 0));

		if (bSupported)
		{
			// Include path
			PublicIncludePaths.Add(Box2DBaseDir);
		}
	}
}
