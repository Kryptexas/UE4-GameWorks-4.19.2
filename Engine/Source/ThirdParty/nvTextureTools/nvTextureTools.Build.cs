// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class nvTextureTools : ModuleRules
{
	public nvTextureTools(TargetInfo Target)
	{
		Type = ModuleType.External;

		string nvttPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "nvTextureTools/nvTextureTools-2.0.8/";

		string nvttLibPath = nvttPath + "lib";

		PublicIncludePaths.Add(nvttPath + "src/src");

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			nvttLibPath += ("/Win64/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName());
			PublicLibraryPaths.Add(nvttLibPath);

			PublicAdditionalLibraries.Add("nvtt_64.lib");

			PublicDelayLoadDLLs.Add("nvtt_64.dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			nvttLibPath += ("/Win32/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName());
			PublicLibraryPaths.Add(nvttLibPath);

			PublicAdditionalLibraries.Add("nvtt.lib");

			PublicDelayLoadDLLs.Add("nvtt.dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(nvttPath + "lib/Mac/libnvcore.dylib");
			PublicAdditionalLibraries.Add(nvttPath + "lib/Mac/libnvimage.dylib");
			PublicAdditionalLibraries.Add(nvttPath + "lib/Mac/libnvmath.dylib");
			PublicAdditionalLibraries.Add(nvttPath + "lib/Mac/libnvtt.dylib");
			PublicAdditionalLibraries.Add(nvttPath + "lib/Mac/libsquish.a");
		}
	}
}

