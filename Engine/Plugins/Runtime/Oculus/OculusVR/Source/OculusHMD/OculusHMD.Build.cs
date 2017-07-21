// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class OculusHMD : ModuleRules
	{
		public OculusHMD(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(
				new string[] {
					// Relative to Engine\Plugins\Runtime\Oculus\OculusVR\Source
					"../../../../../Source/Runtime/Renderer/Private",
					"../../../../../Source/Runtime/OpenGLDrv/Private",
					"../../../../../Source/Runtime/VulkanRHI/Private",
					"../../../../../Source/Runtime/Engine/Classes/Components",
				});

			PublicIncludePathModuleNames.Add("Launch");

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"RHI",
					"RenderCore",
					"Renderer",
					"ShaderCore",
					"HeadMountedDisplay",
					"Slate",
					"SlateCore",
					"ImageWrapper",
					"MediaAssets",
					"Analytics",
					"UtilityShaders",
					"OpenGLDrv",
					"VulkanRHI",
					"OVRPlugin",
				});

			if (Target.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
			}

			AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenGL");

			if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
			{
				// D3D
				{
					PrivateDependencyModuleNames.AddRange(
						new string[]
						{
							"D3D11RHI",
							"D3D12RHI",
						});

					PrivateIncludePaths.AddRange(
						new string[]
						{
							"../../../../../Source/Runtime/Windows/D3D11RHI/Private",
							"../../../../../Source/Runtime/Windows/D3D11RHI/Private/Windows",
							"../../../../../Source/Runtime/D3D12RHI/Private",
							"../../../../../Source/Runtime/D3D12RHI/Private/Windows",
						});

					AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11");
					AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
					AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAPI");
					AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11Audio");
					AddEngineThirdPartyPrivateStaticDependencies(Target, "DirectSound");
				}

				// Vulkan
				{
					string VulkanSDKPath = Environment.GetEnvironmentVariable("VULKAN_SDK");
					if (!String.IsNullOrEmpty(VulkanSDKPath))
					{
						// If the user has an installed SDK, use that instead
						PrivateIncludePaths.Add(VulkanSDKPath + "/Include");
						// Older SDKs have an extra subfolder
						PrivateIncludePaths.Add(VulkanSDKPath + "/Include/vulkan");

						if (Target.Platform == UnrealTargetPlatform.Win32)
						{
							PublicLibraryPaths.Add(VulkanSDKPath + "/Source/lib32");
						}
						else
						{
							PublicLibraryPaths.Add(VulkanSDKPath + "/Source/lib");
						}

						PublicAdditionalLibraries.Add("vulkan-1.lib");
						PublicAdditionalLibraries.Add("vkstatic.1.lib");
					}
					else
					{
						AddEngineThirdPartyPrivateStaticDependencies(Target, "Vulkan");
					}
				}

				// OVRPlugin
				{
					PublicDelayLoadDLLs.Add("OVRPlugin.dll");
					RuntimeDependencies.Add(new RuntimeDependency("$(EngineDir)/Binaries/ThirdParty/Oculus/OVRPlugin/OVRPlugin/" + Target.Platform.ToString() + "/OVRPlugin.dll"));
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Android)
			{
				// Vulkan
				{
					string NDKPath = Environment.GetEnvironmentVariable("NDKROOT");
					// Note: header is the same for all architectures so just use arch-arm
					string NDKVulkanIncludePath = NDKPath + "/platforms/android-24/arch-arm/usr/include/vulkan";

					if (File.Exists(NDKVulkanIncludePath + "/vulkan.h"))
					{
						// Use NDK Vulkan header if discovered
						PrivateIncludePaths.Add(NDKVulkanIncludePath);
					}
					else 
					{
						string VulkanSDKPath = Environment.GetEnvironmentVariable("VULKAN_SDK");

						if (!String.IsNullOrEmpty(VulkanSDKPath))
						{
							// If the user has an installed SDK, use that instead
							PrivateIncludePaths.Add(VulkanSDKPath + "/Include/vulkan");
						}
						else
						{
							// Fall back to the Windows Vulkan SDK (the headers are the same)
							PrivateIncludePaths.Add(Target.UEThirdPartySourceDirectory + "Vulkan/Windows/Include/vulkan");
						}
					}
				}

				// AndroidPlugin
				{
					string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
					AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(PluginPath, "GearVR_APL.xml")));
				}
			}
		}
	}
}