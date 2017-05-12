// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class libPhonon : ModuleRules
{
    public libPhonon(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string LibraryPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "libPhonon/phonon_api/";
        string BinaryPath = "$(EngineDir)/Binaries/ThirdParty/Phonon/";

        PublicIncludePaths.Add(LibraryPath + "include");

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicLibraryPaths.Add(LibraryPath + "/lib/Win64");
            PublicAdditionalLibraries.Add("phonon.lib");

            string DllName = "phonon.dll";

            PublicDelayLoadDLLs.Add(DllName);

            BinaryPath += "Win64/";

            RuntimeDependencies.Add(new RuntimeDependency(BinaryPath + DllName));
        }
    }
}

