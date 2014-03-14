// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IntelTBB : ModuleRules
{
	public IntelTBB(TargetInfo Target)
	{
		Type = ModuleType.External;

		if ((Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Win32))
		{
			string IntelTBBPath = UEBuildConfiguration.UEThirdPartyDirectory + "IntelTBB/IntelTBB-4.0/";
			PublicIncludePaths.Add(IntelTBBPath + "Include");

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
				{
					PublicLibraryPaths.Add(IntelTBBPath + "lib/Win64/vc12");
				}
				else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
				{
					PublicLibraryPaths.Add(IntelTBBPath + "lib/Win64/vc11");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Win32)
			{
				if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
				{
					PublicLibraryPaths.Add(IntelTBBPath + "lib/Win32/vc12");
				}
				else if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
				{
					PublicLibraryPaths.Add(IntelTBBPath + "lib/Win32/vc11");
				}
			}

			// Disable the #pragma comment(lib, ...) used by default in MallocTBB...
			// We want to explicitly include the library.
			Definitions.Add("__TBBMALLOC_BUILD=1");

			string LibName = "tbbmalloc";
			if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
			{
				LibName += "_debug";
			}
			LibName += ".lib";
			PublicAdditionalLibraries.Add(LibName);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					UEBuildConfiguration.UEThirdPartyDirectory + "IntelTBB/IntelTBB-4.0/Include",
				}
			);

			PublicAdditionalLibraries.AddRange(
				new string[] {
					UEBuildConfiguration.UEThirdPartyDirectory + "IntelTBB/IntelTBB-4.0/lib/Mac/libtbb.dylib",
					UEBuildConfiguration.UEThirdPartyDirectory + "IntelTBB/IntelTBB-4.0/lib/Mac/libtbbmalloc.dylib",
				}
			);
		}
	}
}
