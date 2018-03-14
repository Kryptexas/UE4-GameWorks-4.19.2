// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
using System.IO;
using System.Diagnostics;
public class OpenAL : ModuleRules
{
	public OpenAL(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;
		string version = (Target.Platform == UnrealTargetPlatform.Linux) ? "1.18.1" : "1.15.1";

		string OpenALPath = Target.UEThirdPartySourceDirectory + "OpenAL/" + version + "/";
		PublicIncludePaths.Add(OpenALPath + "include");

		if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			// link against runtime path since this avoids hardcoing an RPATH
			string OpenALRuntimePath = Path.Combine(Target.UEThirdPartyBinariesDirectory , "OpenAL", Target.Platform.ToString(), Target.Architecture, "libopenal.so");
			PublicAdditionalLibraries.Add(OpenALRuntimePath);

			RuntimeDependencies.Add("$(EngineDir)/Binaries/ThirdParty/OpenAL/Linux/" + Target.Architecture + "/libopenal.so.1");
		}
	}
}
