// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

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

		bool bHaveVulkan = false;
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// Note: header is the same for all architectures so just use arch-arm
			string NDKPath = Environment.GetEnvironmentVariable("NDKROOT");
			string NDKVulkanIncludePath = NDKPath + "/android-24/arch-arm/usr/include/vulkan";

			// Use NDK Vulkan header if discovered, or VulkanSDK if available
			if (File.Exists(NDKVulkanIncludePath + "/vulkan.h"))
			{
				bHaveVulkan = true;
				PrivateIncludePaths.Add(NDKVulkanIncludePath);
			}
			else
			if (!String.IsNullOrEmpty(VulkanSDKPath))
			{
				bHaveVulkan = true;
				PrivateIncludePaths.Add(VulkanSDKPath + "/Include/vulkan");
			}
		}
		else
		if (!String.IsNullOrEmpty(VulkanSDKPath))
		{
			bHaveVulkan = true;
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

		if (bHaveVulkan)
		{
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
