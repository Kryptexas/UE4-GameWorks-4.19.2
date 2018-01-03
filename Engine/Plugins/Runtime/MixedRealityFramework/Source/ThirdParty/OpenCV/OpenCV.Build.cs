// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using System;
using System.IO;
using UnrealBuildTool;

public class OpenCV : ModuleRules
{
	public OpenCV(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.Win64 ||
			Target.Platform == UnrealTargetPlatform.Win32)
		{
			string PlatformDir = Target.Platform.ToString();
			string IncPath = Path.Combine(ModuleDirectory, "include");
			PublicSystemIncludePaths.Add(IncPath);

			string LibPath = Path.Combine(ModuleDirectory, "lib", PlatformDir);
			PublicLibraryPaths.Add(LibPath);

            string BinaryPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../Binaries/ThirdParty", PlatformDir));

            string LibName = "opencv_world331";

			if (Target.Configuration == UnrealTargetConfiguration.Debug &&
				Target.bDebugBuildsActuallyUseDebugCRT)
			{
					LibName += "d";
			}

			PublicAdditionalLibraries.Add(LibName + ".lib");
			string DLLName = LibName + ".dll";
			PublicDelayLoadDLLs.Add(DLLName);
            RuntimeDependencies.Add(Path.Combine(BinaryPath, DLLName));
			PublicDefinitions.Add("WITH_OPENCV=1");
		}
		else // unsupported platform
		{
            PublicDefinitions.Add("WITH_OPENCV=0");
		}
	}
}
