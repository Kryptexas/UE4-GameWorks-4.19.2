// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LibOVRAudio : ModuleRules
{
	public LibOVRAudio(ReadOnlyTargetRules Target) : base(Target)
	{
		/** Mark the current version of the Oculus SDK */
		Type = ModuleType.External;

		string OculusThirdPartyDirectory = Target.UEThirdPartySourceDirectory + "Oculus/LibOVRAudio/LibOVRAudio";

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicIncludePaths.Add(OculusThirdPartyDirectory + "/include");

			string LibraryPath = OculusThirdPartyDirectory + "/lib/win64";
			string LibraryName = "ovraudio64";

			PublicLibraryPaths.Add(LibraryPath);
			PublicAdditionalLibraries.Add(LibraryName + ".lib");

            PublicDelayLoadDLLs.Add("ovraudio64.dll");
            RuntimeDependencies.Add("$(EngineDir)/Binaries/ThirdParty/Oculus/Audio/Win64/ovraudio64.dll");
        }
	}
}
