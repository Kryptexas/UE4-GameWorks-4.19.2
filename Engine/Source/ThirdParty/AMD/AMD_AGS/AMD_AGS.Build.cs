// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class AMD_AGS : ModuleRules
{
	public AMD_AGS(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string AmdAgsPath = Target.UEThirdPartySourceDirectory + "AMD/AMD_AGS/";
		PublicSystemIncludePaths.Add(AmdAgsPath + "inc/");

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			string AmdApiLibPath = AmdAgsPath + "lib/";
			AmdApiLibPath = AmdApiLibPath + "VS" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName();
			PublicLibraryPaths.Add(AmdApiLibPath);

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				string LibraryName = "amd_ags_x64_" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "_MD.lib";
				PublicAdditionalLibraries.Add(LibraryName);
			}
			else if (Target.Platform == UnrealTargetPlatform.Win32)
			{
				string LibraryName = "amd_ags_x86_" + Target.WindowsPlatform.GetVisualStudioCompilerVersionName() + "_MD.lib";
				PublicAdditionalLibraries.Add(LibraryName);
			}
		}
	}
}

