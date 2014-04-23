// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Perforce : ModuleRules
{
	public Perforce(TargetInfo Target)
	{
		Type = ModuleType.External;


		string LibFolder = "lib/";
		string LibPrefix = "";
		string LibPostfixAndExt = (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT) ? "d." : ".";

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string P4APIPath;
			if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
			{
				P4APIPath = UEBuildConfiguration.UEThirdPartyDirectory + "Perforce/p4api-2014.2/";
			} 
			else
			{
				P4APIPath = UEBuildConfiguration.UEThirdPartyDirectory + "Perforce/p4api-2013.1-BETA/";
			}
			PublicIncludePaths.Add(P4APIPath + "include");

			LibFolder += "win64";
			LibPostfixAndExt += "lib";
			PublicLibraryPaths.Add(P4APIPath + LibFolder);
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32)
		{
			string P4APIPath;
			if (WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2013)
			{
				P4APIPath = UEBuildConfiguration.UEThirdPartyDirectory + "Perforce/p4api-2014.2/";
			}
			else
			{
				P4APIPath = UEBuildConfiguration.UEThirdPartyDirectory + "Perforce/p4api-2013.1-BETA/";
			}
			PublicIncludePaths.Add(P4APIPath + "include");

			LibFolder += "win32";
			LibPostfixAndExt += "lib";
			PublicLibraryPaths.Add(P4APIPath + LibFolder);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			string P4APIPath = UEBuildConfiguration.UEThirdPartyDirectory + "Perforce/p4api-2012.1/";
			PublicIncludePaths.Add(P4APIPath + "include");

			LibFolder += "mac";
			LibPrefix = P4APIPath + LibFolder + "/";
			LibPostfixAndExt = ".a";
		}

		PublicAdditionalLibraries.Add(LibPrefix + "libclient" + LibPostfixAndExt);

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(LibPrefix + "libp4sslstub" + LibPostfixAndExt);
		}
		else if ((Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32) && WindowsPlatform.Compiler == WindowsCompiler.VisualStudio2012)
		{
			PublicAdditionalLibraries.Add(LibPrefix + "libp4sslstub" + LibPostfixAndExt);
		}
		else
		{
			PublicAdditionalLibraries.Add(LibPrefix + "libeay32" + LibPostfixAndExt);
			PublicAdditionalLibraries.Add(LibPrefix + "ssleay32" + LibPostfixAndExt);
		}

		PublicAdditionalLibraries.Add(LibPrefix + "librpc" + LibPostfixAndExt);
		PublicAdditionalLibraries.Add(LibPrefix + "libsupp" + LibPostfixAndExt);
	}
}
