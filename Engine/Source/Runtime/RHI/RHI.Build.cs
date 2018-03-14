// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;

public class RHI : ModuleRules
{
	public RHI(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("ApplicationCore");

		if (Target.bCompileAgainstEngine)
		{
            DynamicallyLoadedModuleNames.Add("NullDrv");

			if (Target.Type != TargetRules.TargetType.Server)   // Dedicated servers should skip loading everything but NullDrv
			{
				// UEBuildAndroid.cs adds VulkanRHI for Android builds if it is enabled
				if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
				{
					DynamicallyLoadedModuleNames.Add("D3D11RHI");

					//#todo-rco: D3D12 requires different SDK headers not compatible with WinXP
					DynamicallyLoadedModuleNames.Add("D3D12RHI");
				}

				if ((Target.Platform == UnrealTargetPlatform.Win64) ||
					(Target.Platform == UnrealTargetPlatform.Win32) ||
					(Target.Platform == UnrealTargetPlatform.Linux && Target.Architecture.StartsWith("x86_64")))	// temporary, not all archs can support Vulkan atm
				{
					DynamicallyLoadedModuleNames.Add("VulkanRHI");
				}

				if ((Target.Platform == UnrealTargetPlatform.Win32) ||
					(Target.Platform == UnrealTargetPlatform.Win64) ||
					(Target.Platform == UnrealTargetPlatform.Linux && Target.Type != TargetRules.TargetType.Server) ||  // @todo should servers on all platforms skip this?
					(Target.Platform == UnrealTargetPlatform.HTML5))
				{
					DynamicallyLoadedModuleNames.Add("OpenGLDrv");
				}
			}
        }

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateIncludePathModuleNames.AddRange(new string[] { "TaskGraph" });
		}

		PrivateIncludePaths.Add("Runtime/RHI/Private");
    }
}
