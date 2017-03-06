// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;

public class FLEX : ModuleRules
{
	public FLEX(TargetInfo Target)
	{
		Type = ModuleType.External;

        if (UEBuildConfiguration.bCompileFLEX_DX == false && UEBuildConfiguration.bCompileFLEX_CUDA == false)
        {
            Definitions.Add("WITH_FLEX=0");
            return;
        }

        Definitions.Add("WITH_FLEX=1");

        if(UEBuildConfiguration.bCompileFLEX_DX)
        {
            Definitions.Add("WITH_FLEX_DX=1");
        }

        if (UEBuildConfiguration.bCompileFLEX_CUDA)
        {
            Definitions.Add("WITH_FLEX_CUDA=1");
        }

        string FLEXDir = UEBuildConfiguration.UEThirdPartySourceDirectory + "PhysX/FLEX-1.1.0/";
		string FLEXLibDir = FLEXDir + "lib";

        PublicIncludePaths.Add(FLEXDir + "include");
        PublicSystemIncludePaths.Add(FLEXDir + "include");

        // Libraries and DLLs for windows platform
        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            PublicLibraryPaths.Add(FLEXLibDir + "/win64");

            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("NvFlexDebugCUDA_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDebugCUDA_x64.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtDebugCUDA_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtDebugCUDA_x64.dll");
                    PublicAdditionalLibraries.Add("NvFlexDeviceDebug_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDeviceDebug_x64.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("NvFlexReleaseCUDA_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexReleaseCUDA_x64.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtReleaseCUDA_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtReleaseCUDA_x64.dll");
                    PublicAdditionalLibraries.Add("NvFlexDeviceRelease_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDeviceRelease_x64.dll");
                }
            }

            if (UEBuildConfiguration.bCompileFLEX_DX)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("NvFlexDebugD3D_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDebugD3D_x64.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtDebugD3D_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtDebugD3D_x64.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("NvFlexReleaseD3D_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexReleaseD3D_x64.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtReleaseD3D_x64.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtReleaseD3D_x64.dll");
                }
            }

            PublicLibraryPaths.Add(FLEXDir + "/Win64");

            string FlexBinariesDir = String.Format("$(EngineDir)/Binaries/ThirdParty/PhysX/FLEX-1.1.0/Win64/");


            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {
                string[] RuntimeDependenciesX64 =
                {
                    "cudart64_80.dll",
                    "NvFlexDebugCUDA_x64.dll",
                    "NvFlexReleaseCUDA_x64.dll",
                    "NvFlexExtDebugCUDA_x64.dll",
                    "NvFlexExtReleaseCUDA_x64.dll",
                    "NvFlexDeviceRelease_x64.dll",
                };

                foreach (string RuntimeDependency in RuntimeDependenciesX64)
                {
                    RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
                }
            }

            if (UEBuildConfiguration.bCompileFLEX_DX)
            {
                string[] RuntimeDependenciesX64 =
                {
                    "amd_ags_x64.dll",
                    "NvFlexDebugD3D_x64.dll",
                    "NvFlexReleaseD3D_x64.dll",
                    "NvFlexExtDebugD3D_x64.dll",
                    "NvFlexExtReleaseD3D_x64.dll",
                };

                foreach (string RuntimeDependency in RuntimeDependenciesX64)
                {
                    RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
                }
            }
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			PublicLibraryPaths.Add(FLEXLibDir + "/win32");

            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("NvFlexDebugCUDA_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDebugCUDA_x86.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtDebugCUDA_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtDebugCUDA_x86.dll");
                    PublicAdditionalLibraries.Add("NvFlexDeviceDebug_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDeviceDebug_x86.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("NvFlexReleaseCUDA_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexReleaseCUDA_x86.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtReleaseCUDA_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtReleaseCUDA_x86.dll");
                    PublicAdditionalLibraries.Add("NvFlexDeviceRelease_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDeviceRelease_x86.dll");
                }
            }

            if (UEBuildConfiguration.bCompileFLEX_DX)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("NvFlexDebugD3D_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexDebugD3D_x86.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtDebugD3D_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtDebugD3D_x86.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("NvFlexReleaseD3D_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexReleaseD3D_x86.dll");
                    PublicAdditionalLibraries.Add("NvFlexExtReleaseD3D_x86.lib");
                    PublicDelayLoadDLLs.Add("NvFlexExtReleaseD3D_x86.dll");
                }
            }

            PublicLibraryPaths.Add(FLEXDir + "/Win32");

            string FlexBinariesDir = String.Format("$(EngineDir)/Binaries/ThirdParty/PhysX/FLEX-1.1.0/Win32/");

            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {

                string[] RuntimeDependenciesX86 =
                {
                    "cudart32_80.dll",
                    "NvFlexDebugCUDA_x86.dll",
                    "NvFlexReleaseCUDA_x86.dll",
                    "NvFlexExtDebugCUDA_x86.dll",
                    "NvFlexExtReleaseCUDA_x86.dll",
                    "NvFlexDeviceRelease_x86.dll",
                };

                foreach (string RuntimeDependency in RuntimeDependenciesX86)
                {
                    RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
                }
            }


            if (UEBuildConfiguration.bCompileFLEX_DX)
            {

                string[] RuntimeDependenciesX86 =
                {
                    "amd_ags_x86.dll",
                    "NvFlexDebugD3D_x86.dll",
                    "NvFlexReleaseD3D_x86.dll",
                    "NvFlexExtDebugD3D_x86.dll",
                    "NvFlexExtReleaseD3D_x86.dll",
                };

                foreach (string RuntimeDependency in RuntimeDependenciesX86)
                {
                    RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
                }
            }
        }
	}
}
