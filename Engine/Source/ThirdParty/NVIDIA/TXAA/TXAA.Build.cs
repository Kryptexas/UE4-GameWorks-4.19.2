// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System;

public class TXAA : ModuleRules
{
    public TXAA(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        const bool debugTXAA = false;
        PublicDefinitions.Add("WITH_TXAA=1");
        if (debugTXAA)
            PublicDefinitions.Add("DEBUG_TXAA=1");
        else
            PublicDefinitions.Add("DEBUG_TXAA=0");

        string nvTxaaPath = Target.UEThirdPartySourceDirectory + "NVIDIA/TXAA/";

        string nvTxaaIncludePath = nvTxaaPath /*+ "inc/"*/;
        PublicSystemIncludePaths.Add(nvTxaaIncludePath);

        string nvTxaaLibPath = nvTxaaPath + "lib/";
        PublicLibraryPaths.Add(nvTxaaLibPath);
        string libdllname = "GFSDK_Txaa.";

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            libdllname += "win64.";
//             PublicAdditionalLibraries.Add("GFSDK_Txaa.win64.lib");
//             PublicDelayLoadDLLs.Add("GFSDK_Txaa.win64.dll");
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            libdllname += "win32.";
//             PublicAdditionalLibraries.Add("GFSDK_Txaa.win32.lib");
//             PublicDelayLoadDLLs.Add("GFSDK_Txaa.win32.dll");
        }
        if (debugTXAA)
            libdllname += "D.";
        PublicAdditionalLibraries.Add(libdllname + "lib");
        PublicDelayLoadDLLs.Add(libdllname + "dll");
    }
}

