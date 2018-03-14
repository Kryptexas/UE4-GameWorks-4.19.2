// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class MTLPP : ModuleRules
{
	public MTLPP(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string MTLPPPath = Target.UEThirdPartySourceDirectory + "mtlpp/mtlpp-master-7efad47/";

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicSystemIncludePaths.Add(MTLPPPath + "src");
			PublicSystemIncludePaths.Add(MTLPPPath + "interpose");
			if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
			{
				PublicAdditionalLibraries.Add(MTLPPPath + "lib/Mac/libmtlppd.a");
			}
			else
			{
				PublicAdditionalLibraries.Add(MTLPPPath + "lib/Mac/libmtlpp.a");
			}
		}
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicSystemIncludePaths.Add(MTLPPPath + "src");
			PublicSystemIncludePaths.Add(MTLPPPath + "interpose");
            if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
            {
                PublicAdditionalLibraries.Add(MTLPPPath + "lib/IOS/libmtlppd.a");
            }
            else
            {
                PublicAdditionalLibraries.Add(MTLPPPath + "lib/IOS/libmtlpp.a");
            }
        }
        else if (Target.Platform == UnrealTargetPlatform.TVOS)
        {
            PublicSystemIncludePaths.Add(MTLPPPath + "src");
			PublicSystemIncludePaths.Add(MTLPPPath + "interpose");
            if (Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT)
            {
                PublicAdditionalLibraries.Add(MTLPPPath + "lib/TVOS/libmtlppd.a");
            }
            else
            {
                PublicAdditionalLibraries.Add(MTLPPPath + "lib/TVOS/libmtlpp.a");
            }
        }
    }
}
