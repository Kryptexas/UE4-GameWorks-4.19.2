//
// Copyright (C) Google Inc. 2017. All rights reserved.
//

using System.IO;
using UnrealBuildTool;

public class ResonanceAudioApi : ModuleRules
{
    public ResonanceAudioApi(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        string LibraryPath = Target.UEThirdPartySourceDirectory + "ResonanceAudioApi/";

        PublicIncludePaths.Add(LibraryPath + "include");

        if (Target.Platform == UnrealTargetPlatform.Android)
        {
			// Register Plugin Language
			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "ResonanceAudio_APL.xml"));

            PublicLibraryPaths.Add(LibraryPath + "lib/android/armv7/");
            PublicLibraryPaths.Add(LibraryPath + "lib/android/arm64/");

            PublicAdditionalLibraries.Add("vraudio");
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS) {
            string IOSLibraryPath = LibraryPath + "lib/ios/";
            PublicLibraryPaths.Add(IOSLibraryPath);
            PublicAdditionalLibraries.Add(IOSLibraryPath + "libvraudio.a");
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            LibraryPath += "lib/linux/";
            PublicLibraryPaths.Add(LibraryPath);

            string SharedObjectName = "libvraudio.so";
            PublicDelayLoadDLLs.Add(SharedObjectName);
            RuntimeDependencies.Add(LibraryPath + SharedObjectName);
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            LibraryPath += "lib/darwin/";
            PublicLibraryPaths.Add(LibraryPath);

            string DylibName = "libvraudio.dylib";
            PublicDelayLoadDLLs.Add(LibraryPath + DylibName);
            RuntimeDependencies.Add(LibraryPath + DylibName);
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            LibraryPath += "lib/win_x86/";
            PublicLibraryPaths.Add(LibraryPath);

            string DllName = "vraudio.dll";
            PublicDelayLoadDLLs.Add(DllName);
            RuntimeDependencies.Add(LibraryPath + DllName);
        }
        else if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            LibraryPath += "lib/win_x64/";
            PublicLibraryPaths.Add(LibraryPath);

            string DllName = "vraudio.dll";
            PublicDelayLoadDLLs.Add(DllName);
            RuntimeDependencies.Add(LibraryPath + DllName);
        }
        else
        {
            throw new System.Exception(System.String.Format("Unsupported platform {0}", Target.Platform.ToString()));
        }
    }
}