// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Recast : ModuleRules
{
	public Recast(TargetInfo Target)
	{
		Type = ModuleType.External;

		Definitions.Add("WITH_RECAST=1");
		Definitions.Add("WITH_FIXED_AREA_ENTERING_COST=1");

		string RecastPath = UEBuildConfiguration.UEThirdPartyDirectory + "Recast/";
		PublicIncludePaths.Add(RecastPath + "Recast/Include");
		PublicIncludePaths.Add(RecastPath + "Detour/Include");
		PublicIncludePaths.Add(RecastPath + "DetourTileCache/Include");
		// @todo remove when rcChunkyTriMesh gets reimplemented
		PublicIncludePaths.Add(RecastPath + "RecastDemo/Include");

		string RecastLibName = "recast";
		if (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
		{
			RecastLibName += "d";
		}
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			RecastLibName += "_64";
		}
		
		if (Target.Platform == UnrealTargetPlatform.Win32 ||
			Target.Platform == UnrealTargetPlatform.Win64 ||
			(Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32"))
		{
			RecastLibName += ".lib";

			string RecastLibPath = RecastPath + "Epic/lib/";
			RecastLibPath += (Target.Platform == UnrealTargetPlatform.Win64 ? "Win64/" : "Win32/");
			RecastLibPath += "VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();
		
			PublicLibraryPaths.Add(RecastLibPath);
			PublicAdditionalLibraries.Add(RecastLibName);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicAdditionalLibraries.Add(RecastPath + "Epic/lib/Mac/lib" + RecastLibName + ".a");
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			if (Target.Architecture == "-armv7")
		{
			PublicLibraryPaths.Add(RecastPath + "Epic/lib/Android/ARMv7");
		}
			else
		{
			PublicLibraryPaths.Add(RecastPath + "Epic/lib/Android/x86");
			}
			PublicAdditionalLibraries.Add("recast");
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			string ShadowRecastLibName = "lib" + RecastLibName + ".a";
			if (Target.Architecture == "-simulator")
			{
				PublicLibraryPaths.Add(RecastPath + "Epic/lib/IOS/Simulator");
				PublicAdditionalShadowFiles.Add(RecastPath + "Epic/lib/IOS/Simulator/" + ShadowRecastLibName);
			}
			else
			{
				PublicLibraryPaths.Add(RecastPath + "Epic/lib/IOS/Device");
				PublicAdditionalShadowFiles.Add(RecastPath + "Epic/lib/IOS/Device/" + ShadowRecastLibName);
			}

			PublicAdditionalLibraries.Add(RecastLibName);
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			PublicLibraryPaths.Add(RecastPath + "Epic/lib/Linux");
			PublicAdditionalLibraries.Add("recast");
		}
        else if (Target.Platform == UnrealTargetPlatform.HTML5)
        {
            PublicAdditionalLibraries.Add(RecastPath + "Epic/lib/HTML5/librecast.bc");
        }
	}
}

