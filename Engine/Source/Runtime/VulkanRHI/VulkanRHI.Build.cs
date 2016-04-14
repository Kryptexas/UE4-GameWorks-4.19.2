// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;

public class VulkanRHI : ModuleRules
{
	public VulkanRHI(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Runtime/Vulkan/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"CoreUObject", 
				"Engine", 
				"RHI", 
				"RenderCore", 
				"ShaderCore",
				"UtilityShaders",
			}
			);

        if (Target.Platform != UnrealTargetPlatform.Android)
        {
            //PrivateDependencyModuleNames.Add("VulkanShaderFormat");
            //AddEngineThirdPartyPrivateStaticDependencies(Target, "HLSLCC");
        }

		// Newer SDKs use a different env var, so fallback if not found
		bool bUsingNewEnvVar = true;
		string VulkanSDKPath = Environment.GetEnvironmentVariable("VULKAN_SDK");
		if (String.IsNullOrEmpty(VulkanSDKPath))
		{
			VulkanSDKPath = Environment.GetEnvironmentVariable("VK_SDK_PATH");
			bUsingNewEnvVar = false;
		}

		if (!String.IsNullOrEmpty(VulkanSDKPath))
		{
			PrivateIncludePaths.Add(VulkanSDKPath + "/Include");

			//#todo-rco: Using /Source/lib instead of /bin as we want pdb's for now
			if (Target.Platform == UnrealTargetPlatform.Win32)
			{
				PublicLibraryPaths.Add(VulkanSDKPath + "/Source/lib32");
			}
			else
			{
				PublicLibraryPaths.Add(VulkanSDKPath + "/Source/lib");
			}

			if (Target.Platform == UnrealTargetPlatform.Android)
            {
                PublicAdditionalLibraries.Add(System.IO.Path.Combine(VulkanSDKPath, "Source/lib/libvulkan.so"));
            }
            else
            {
				if (bUsingNewEnvVar || VulkanSDKPath.Contains("1.0"))
                {
                    PublicAdditionalLibraries.Add("vulkan-1.lib");
					if (bUsingNewEnvVar)
					{
						PublicAdditionalLibraries.Add("vkstatic.1.lib");
					}
                }
                else
                {
                    PublicAdditionalLibraries.Add("vulkan-0.lib");
                }
            }

            if (Target.Configuration != UnrealTargetConfiguration.Shipping)
            {
                    PrivateIncludePathModuleNames.AddRange(
                            new string[]
                            {
                                    "TaskGraph",
                            }
                    );
            }
		}
		else
		{
			PrecompileForTargets = PrecompileTargetsType.None;
		}
    }
}
