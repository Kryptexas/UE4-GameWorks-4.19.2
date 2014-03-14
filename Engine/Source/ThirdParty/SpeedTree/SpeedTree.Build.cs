// Copyright 1998-2011 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System.IO;

public class SpeedTree : ModuleRules
{
	public SpeedTree(TargetInfo Target)
	{
		Type = ModuleType.External;

		// @todo: SpeedTree does support OSX and Linux
		var bPlatformAllowed = ((Target.Platform == UnrealTargetPlatform.Win32) ||
								(Target.Platform == UnrealTargetPlatform.Win64));

		if (bPlatformAllowed &&
			UEBuildConfiguration.bCompileSpeedTree)
		{
			Definitions.Add("WITH_SPEEDTREE=1");
			Definitions.Add("EPIC_INTERNAL_SPEEDTREE_KEY=B62562037E6717DA");

			string SpeedTreePath = UEBuildConfiguration.UEThirdPartyDirectory + "NoRedist/SpeedTree/SpeedTree-v6.3/";
			PublicIncludePaths.Add(SpeedTreePath + "Include");

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
				{
					PublicLibraryPaths.Add(SpeedTreePath + "Lib/Windows/VC12.x64");

					if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC12_MT64_Static_d.lib");
					}
					else
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC12_MT64_Static.lib");
					}
				}
				else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
				{
					PublicLibraryPaths.Add(SpeedTreePath + "Lib/Windows/VC11.x64");

					if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC11_MT64_Static_d.lib");
					}
					else
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC11_MT64_Static.lib");
					}
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Win32)
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
				{
					PublicLibraryPaths.Add(SpeedTreePath + "Lib/Windows/VC12");

					if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC12_MT_Static_d.lib");
					}
					else
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC12_MT_Static.lib");
					}
				}
				else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
				{
					PublicLibraryPaths.Add(SpeedTreePath + "Lib/Windows/VC11");

					if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC11_MT_Static_d.lib");
					}
					else
					{
						PublicAdditionalLibraries.Add("SpeedTreeCore_Windows_v6.3_VC11_MT_Static.lib");
					}
				}
			}
		}
	}
}

