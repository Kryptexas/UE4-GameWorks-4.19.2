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
            PublicLibraryPaths.Add(FLEXLibDir + "/x64");

            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("flex_cuda_debug_x64.lib");
                    PublicDelayLoadDLLs.Add("flex_cuda_debug_x64.dll");
                    PublicAdditionalLibraries.Add("flexExt_cuda_debug_x64.lib");
                    PublicDelayLoadDLLs.Add("flexExt_cuda_debug_x64.dll");
                    PublicAdditionalLibraries.Add("flexDevice_debug_x64.lib");
                    PublicDelayLoadDLLs.Add("flexDevice_debug_x64.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("flex_cuda_release_x64.lib");
                    PublicDelayLoadDLLs.Add("flex_cuda_release_x64.dll");
                    PublicAdditionalLibraries.Add("flexExt_cuda_release_x64.lib");
                    PublicDelayLoadDLLs.Add("flexExt_cuda_release_x64.dll");
                    PublicAdditionalLibraries.Add("flexDevice_release_x64.lib");
                    PublicDelayLoadDLLs.Add("flexDevice_release_x64.dll");
                }
            }

            if (UEBuildConfiguration.bCompileFLEX_DX)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("flex_d3d11_debug_x64.lib");
                    PublicDelayLoadDLLs.Add("flex_d3d11_debug_x64.dll");
                    PublicAdditionalLibraries.Add("flexExt_d3d11_debug_x64.lib");
                    PublicDelayLoadDLLs.Add("flexExt_d3d11_debug_x64.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("flex_d3d11_release_x64.lib");
                    PublicDelayLoadDLLs.Add("flex_d3d11_release_x64.dll");
                    PublicAdditionalLibraries.Add("flexExt_d3d11_release_x64.lib");
                    PublicDelayLoadDLLs.Add("flexExt_d3d11_release_x64.dll");
                }
            }

            PublicLibraryPaths.Add(FLEXDir + "/Win64");

            string FlexBinariesDir = String.Format("$(EngineDir)/Binaries/ThirdParty/PhysX/FLEX-1.1.0/Win64/");


            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {
                string[] RuntimeDependenciesX64 =
                {
                    "cudart64_80.dll",
                    "flex_cuda_debug_x64.dll",
                    "flex_cuda_release_x64.dll",
                    "flexExt_cuda_debug_x64.dll",
                    "flexExt_cuda_release_x64.dll",
                    "flexDevice_release_x64.dll",
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
                    "flex_d3d11_debug_x64.dll",
                    "flex_d3d11_release_x64.dll",
                    "flexExt_d3d11_debug_x64.dll",
                    "flexExt_d3d11_release_x64.dll",
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
                    PublicAdditionalLibraries.Add("flex_cuda_debug_x86.lib");
                    PublicDelayLoadDLLs.Add("flex_cuda_debug_x86.dll");
                    PublicAdditionalLibraries.Add("flexExt_cuda_debug_x86.lib");
                    PublicDelayLoadDLLs.Add("flexExt_cuda_debug_x86.dll");
                    PublicAdditionalLibraries.Add("flexDevice_debug_x86.lib");
                    PublicDelayLoadDLLs.Add("flexDevice_debug_x86.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("flex_cuda_release_x86.lib");
                    PublicDelayLoadDLLs.Add("flex_cuda_release_x86.dll");
                    PublicAdditionalLibraries.Add("flexExt_cuda_release_x86.lib");
                    PublicDelayLoadDLLs.Add("flexExt_cuda_release_x86.dll");
                    PublicAdditionalLibraries.Add("flexDevice_release_x86.lib");
                    PublicDelayLoadDLLs.Add("flexDevice_release_x86.dll");
                }
            }

            if (UEBuildConfiguration.bCompileFLEX_DX)
            {
                if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
                {
                    PublicAdditionalLibraries.Add("flex_d3d11_debug_x86.lib");
                    PublicDelayLoadDLLs.Add("flex_d3d11_debug_x86.dll");
                    PublicAdditionalLibraries.Add("flexExt_d3d11_debug_x86.lib");
                    PublicDelayLoadDLLs.Add("flexExt_d3d11_debug_x86.dll");
                }
                else
                {
                    PublicAdditionalLibraries.Add("flex_d3d11_release_x86.lib");
                    PublicDelayLoadDLLs.Add("flex_d3d11_release_x86.dll");
                    PublicAdditionalLibraries.Add("flexExt_d3d11_release_x86.lib");
                    PublicDelayLoadDLLs.Add("flexExt_d3d11_release_x86.dll");
                }
            }

            PublicLibraryPaths.Add(FLEXDir + "/Win32");

            string FlexBinariesDir = String.Format("$(EngineDir)/Binaries/ThirdParty/PhysX/FLEX-1.1.0/Win32/");

            if (UEBuildConfiguration.bCompileFLEX_CUDA)
            {

                string[] RuntimeDependenciesX86 =
                {
                    "cudart32_80.dll",
                    "flex_cuda_debug_x86.dll",
                    "flexExt_cuda_debug_x86.dll",
                    "flexExt_cuda_release_x86.dll",
                    "flex_cuda_release_x86.dll",
                    "flexDevice_release_x86.dll",
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
                    "flex_d3d11_debug_x86.dll",
                    "flexExt_d3d11_debug_x86.dll",
                    "flexExt_d3d11_release_x86.dll",
                    "flex_d3d11_release_x86.dll",
                };

                foreach (string RuntimeDependency in RuntimeDependenciesX86)
                {
                    RuntimeDependencies.Add(new RuntimeDependency(FlexBinariesDir + RuntimeDependency));
                }
            }
        }
	}
}
