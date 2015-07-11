// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class DX12 : ModuleRules
{
	public DX12(TargetInfo Target)
	{
		Type = ModuleType.External;

		string DirectXSDKDir = UEBuildConfiguration.UEThirdPartySourceDirectory + "Windows/DirectX";
		PublicSystemIncludePaths.Add(DirectXSDKDir + "/include");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicLibraryPaths.Add(DirectXSDKDir + "/Lib/x64");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			PublicLibraryPaths.Add(DirectXSDKDir + "/Lib/x86");
		}

		// If we're targeting Windows XP, then always delay-load D3D12 as it won't exist on that architecture
		if (WindowsPlatform.IsWindowsXPSupported())
		{
			PublicDelayLoadDLLs.AddRange( new string[] {
				"d3d12.dll"
			} );
		}

		PublicAdditionalLibraries.AddRange(
			new string[] {
                "d3d12.lib"
			}
			);
	}
}

