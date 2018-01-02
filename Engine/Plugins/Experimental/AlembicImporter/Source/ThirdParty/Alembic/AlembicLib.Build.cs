// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Diagnostics;
using System.Collections.Generic;

public class AlembicLib : ModuleRules
{
    public AlembicLib(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;
        if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32 ||
            Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux)
        {
            bool bDebug = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT);

            string LibDir = ModuleDirectory + "/AlembicDeploy/";
            string Platform;
            bool bAllowDynamicLibs = true;
            switch (Target.Platform)
            {
                case UnrealTargetPlatform.Win64:
                    Platform = "x64";
                    LibDir += "VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
                    break;
                case UnrealTargetPlatform.Mac:
                    Platform = "Mac";
                    bAllowDynamicLibs = false;
                    break;
                case UnrealTargetPlatform.Linux:
                    Platform = "Linux";
                    bAllowDynamicLibs = false;
                    break;
                default:
                    return;
            }
            LibDir = LibDir + "/" + Platform + "/lib/";
            PublicLibraryPaths.Add(LibDir);

            string LibPostFix = bDebug && bAllowDynamicLibs ? "d" : "";
            string LibExtension = (Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux) ? ".a" : ".lib";

            if (Target.Platform == UnrealTargetPlatform.Win64)
            {
                List<string> ReqLibraryNames = new List<string>();
                ReqLibraryNames.AddRange
                (
                    new string[] {
                        "Half",
                        "Iex",
                        "IlmThread",
                        "Imath",
                        "Alembic"
                });

                foreach (string LibraryName in ReqLibraryNames)
                {
                    PublicAdditionalLibraries.Add(LibraryName + LibPostFix + LibExtension);
                }

                if (Target.bDebugBuildsActuallyUseDebugCRT && bDebug)
                {
                    RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Plugins/Experimental/AlembicImporter/Binaries/ThirdParty/zlib/zlibd1.dll"));
                }
            }
            else if (Target.Platform == UnrealTargetPlatform.Mac)
            {
                List<string> ReqLibraryNames = new List<string>();
                ReqLibraryNames.AddRange
                (
                    new string[] {
                    "libHalf",
                    "libIex",
                    "libIlmThread",
                    "libImath",
                    (bDebug && bAllowDynamicLibs) ? "hdf5_" : "hdf5",
                    "libAlembic"
                  });

                foreach (string LibraryName in ReqLibraryNames)
                {
                    PublicAdditionalLibraries.Add(LibDir + LibraryName + LibPostFix + LibExtension);
				}
			}
            else if (Target.Platform == UnrealTargetPlatform.Linux)
            {
                List<string> ReqLibraryNames = new List<string>();
                ReqLibraryNames.AddRange
                (
                    new string[] {
                    "libHalf",
                    "libIex",
                    "libIlmThread",
                    "libImath",
                    "hdf5",
                    "libAlembic"
                  });

                foreach (string LibraryName in ReqLibraryNames)
                {
                    PublicAdditionalLibraries.Add(LibDir + Target.Architecture + "/" + LibraryName + LibExtension);
                }
            }

            PublicIncludePaths.Add(ModuleDirectory + "/AlembicDeploy/include/");
            PublicIncludePaths.Add(ModuleDirectory + "/AlembicDeploy/include/OpenEXR/");
        }
    }
}
