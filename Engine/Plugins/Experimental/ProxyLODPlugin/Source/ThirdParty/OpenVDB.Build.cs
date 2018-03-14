// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

// djh using System.IO;
using UnrealBuildTool;
using System.Diagnostics;
using System.Collections.Generic;
using System.IO; // This is for the directory check.

public class OpenVDB : ModuleRules
{
    public OpenVDB(ReadOnlyTargetRules Target) : base(Target)
    {
        // We are just setting up paths for pre-compiled binaries.
        Type = ModuleType.External;

        // For boost:: and TBB:: code
        bEnableUndefinedIdentifierWarnings = false;
        bUseRTTI = true;


        PublicDefinitions.Add("OPENVDB_STATICLIB");
        PublicDefinitions.Add("OPENVDB_OPENEXR_STATICLIB");
        PublicDefinitions.Add("OPENVDB_2_ABI_COMPATIBLE");

        // For testing during developement 
        bool bDebugPaths = true;

        // Only building for Windows

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            string OpenVDBHeaderDir = ModuleDirectory + "/openvdb";

            
            if (bDebugPaths)
            {
                if (!Directory.Exists(OpenVDBHeaderDir))
                {
                    string Err = string.Format("OpenVDB SDK not found in {0}", OpenVDBHeaderDir);
                    System.Console.WriteLine(Err);
                    throw new BuildException(Err);
                }
            }

            PublicIncludePaths.Add(OpenVDBHeaderDir);

            // Construct the OpenVDB directory name
            string LibDirName = ModuleDirectory + "/Deploy/";
            LibDirName += "VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/lib/x64/";

            bool bDebug = (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT);

            if (bDebug)
            {
                LibDirName += "Debug/";
            }
            else
            {
                LibDirName += "Release/";
            }

            if (bDebugPaths)
            {
                // Look for the file
                if (!File.Exists(LibDirName + "OpenVDB.lib"))
                {
                    string Err = string.Format("OpenVDB.lib not found in {0}", LibDirName);
                    System.Console.WriteLine(Err);
                    throw new BuildException(Err);
                }
            }
            PublicLibraryPaths.Add(LibDirName);
            PublicAdditionalLibraries.Add("OpenVDB.lib");

            // Add openexr
            PublicIncludePaths.Add(Target.UEThirdPartySourceDirectory);// + "openexr/Deploy/include");

        

            // Add TBB
            {
                // store the compiled tbb library in the same area as the rest of the third party code
                string TBBLibPath = ModuleDirectory + "/Deploy/VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "/lib/x64/IntelTBB-4.4u3/";
                PublicIncludePaths.Add(Target.UEThirdPartySourceDirectory + "IntelTBB/IntelTBB-4.4u3/include");
               // string TBBLibPath = Target.UEThirdPartySourceDirectory + "IntelTBB/IntelTBB-4.4u3/build/Windows_vc14/x64/";
                if (bDebug)
                {
                    TBBLibPath += "Debug-MT";
                    PublicAdditionalLibraries.Add("tbb_debug.lib");
                }
                else
                {
                    TBBLibPath += "Release-MT";
                    PublicAdditionalLibraries.Add("tbb.lib");
                }
                PublicLibraryPaths.Add(TBBLibPath);
            }
            // Add LibZ
            {
                string ZLibPath = Target.UEThirdPartySourceDirectory + "zlib/zlib-1.2.5/Lib/Win64";
                PublicLibraryPaths.Add(ZLibPath);
                if (bDebug)
                {
                    PublicAdditionalLibraries.Add("zlibd_64.lib");
                }
                else
                {
                    PublicAdditionalLibraries.Add("zlib_64.lib");
                }
            }
        }
        else
        {
            string Err = "Wrong build env!";
            System.Console.WriteLine(Err);
            throw new BuildException(Err);
        }
    }
}
