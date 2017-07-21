// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

public class ANGLE : ModuleRules
{
	public ANGLE(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		if (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32")
		{
			PublicIncludePaths.Add(Target.UEThirdPartySourceDirectory + "ANGLE");
			PublicAdditionalLibraries.Add(Target.UEThirdPartySourceDirectory + "ANGLE/lib/libGLESv2.lib");
			PublicAdditionalLibraries.Add(Target.UEThirdPartySourceDirectory + "ANGLE/lib/libEGL.lib");

            PublicDelayLoadDLLs.AddRange(
                       new string[] {
						"libGLESv2.dll", 
    					"libEGL.dll" }
                       );

		}
	}
}
