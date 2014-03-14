// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class VorbisFile : ModuleRules
{
	public VorbisFile(TargetInfo Target)
	{
		Type = ModuleType.External;

		string VorbisPath = UEBuildConfiguration.UEThirdPartyDirectory + "Vorbis/libvorbis-1.3.2/";
		PublicIncludePaths.Add(VorbisPath + "include");
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string VorbisLibPath = VorbisPath + "Lib/win64/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
			PublicLibraryPaths.Add(VorbisLibPath);
			PublicAdditionalLibraries.Add("libvorbisfile_64.lib");
			PublicDelayLoadDLLs.Add("libvorbisfile_64.dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.Win32 )
		{
			string VorbisLibPath = VorbisPath + "Lib/win32/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName() + "/";
			PublicLibraryPaths.Add(VorbisLibPath);
			PublicAdditionalLibraries.Add("libvorbisfile.lib");
			PublicDelayLoadDLLs.Add("libvorbisfile.dll");
		}
		else if (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32")
        {
            string VorbisLibPath = VorbisPath + "Lib/HTML5Win32/";
            PublicLibraryPaths.Add(VorbisLibPath);
            PublicAdditionalLibraries.Add("libvorbisfile.lib");
        }
        else if (Target.Platform == UnrealTargetPlatform.HTML5)
        {
            string VorbisLibPath = VorbisPath + "Lib/HTML5";
            PublicLibraryPaths.Add(VorbisLibPath);
            PublicAdditionalLibraries.Add("libvorbisfile.bc");
        }
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			if (Target.Architecture == "-armv7")
			{
				PublicLibraryPaths.Add(VorbisPath + "Lib/Android/ARMv7");
			}
			else
			{
				PublicLibraryPaths.Add(VorbisPath + "Lib/Android/x86");
			}
			PublicAdditionalLibraries.Add("vorbisfile");
		}
    }
}

