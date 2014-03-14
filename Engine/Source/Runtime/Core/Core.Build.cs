// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;
using System.IO;

public class Core : ModuleRules
{
	public Core(TargetInfo Target)
	{
		SharedPCHHeaderFile = "Runtime/Core/Public/Core.h";

		PublicIncludePaths.AddRange(
			new string[] {
				"Runtime/Core/Public",
				"Runtime/Core/Public/Internationalization",
				"Runtime/Core/Public/Async",
				"Runtime/Core/Public/Concurrency",
				"Runtime/Core/Public/Containers",
				"Runtime/Core/Public/GenericPlatform",
				"Runtime/Core/Public/HAL",
				"Runtime/Core/Public/Math",
				"Runtime/Core/Public/Misc",
				"Runtime/Core/Public/Modules",
				"Runtime/Core/Public/Modules/Boilerplate",
				"Runtime/Core/Public/ProfilingDebugging",
				"Runtime/Core/Public/Serialization",
				"Runtime/Core/Public/Serialization/Json",
				"Runtime/Core/Public/Stats",
				"Runtime/Core/Public/Templates",
				"Runtime/Core/Public/UObject",
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/DerivedDataCache/Public",
				"Developer/SynthBenchmark/Public",
				"Runtime/Core/Private",
				"Runtime/Core/Private/Misc",
				"Runtime/Core/Private/Serialization/Json",
                "Runtime/Core/Private/Internationalization",
				"Runtime/Core/Private/Internationalization/Cultures",
			}
			);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"TargetPlatform",
				"DerivedDataCache",
				"InputDevice",
			}
			);

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			PublicIncludePaths.Add("Runtime/Core/Public/Windows");
			AddThirdPartyPrivateStaticDependencies(Target, 
				"zlib");

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				DynamicallyLoadedModuleNames.Add("VSAccessor");
			}

			AddThirdPartyPrivateStaticDependencies(Target, 
				"IntelTBB",
				"XInput"
				);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicIncludePaths.AddRange(new string[] {"Runtime/Core/Public/Apple", "Runtime/Core/Public/Mac",});
			AddThirdPartyPrivateStaticDependencies(Target, 
				"IntelTBB",
				"zlib",
				"OpenGL"
				);
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicIncludePaths.AddRange(new string[] {"Runtime/Core/Public/Apple", "Runtime/Core/Public/IOS"});
			AddThirdPartyPrivateStaticDependencies(Target, 
				"zlib"
				);
			PublicFrameworks.AddRange(new string[] { "UIKit", "Foundation", "AudioToolbox", "AVFoundation", "GameKit", "StoreKit", "CoreVideo", "CoreMedia"});

			bool bSupportAdvertising = true;

			Definitions.Add("UE_WITH_IAD=" + (bSupportAdvertising ? "1" : "0"));
			
			if (bSupportAdvertising)
			{
				PublicFrameworks.AddRange(new string[] { "iAD", "CoreGraphics" });
			}
		}
		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PublicIncludePaths.Add("Runtime/Core/Public/Android");
			AddThirdPartyPrivateStaticDependencies(Target, 
				"zlib"
				);
		}
        else if ((Target.Platform == UnrealTargetPlatform.Linux))
        {
            PublicIncludePaths.Add("Runtime/Core/Public/Linux");
			AddThirdPartyPrivateStaticDependencies(Target, 
				"zlib",
				"jemalloc",
				"elftoolchain"
                );
        }
		else if (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32")
		{
            PublicIncludePaths.Add("Runtime/Core/Public/HTML5");
			AddThirdPartyPrivateStaticDependencies(Target, "SDL");
			AddThirdPartyPrivateStaticDependencies(Target, "OpenAL");
		}


		if ((UEBuildConfiguration.bIncludeADO == true) || (UEBuildConfiguration.bCompileAgainstEngine == true))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "ADO");
		}

        if ( UEBuildConfiguration.bCompileICU == true ) 
        {
			AddThirdPartyPrivateStaticDependencies(Target, "ICU");
        }
        Definitions.Add("UE_ENABLE_ICU=" + (UEBuildConfiguration.bCompileICU ? "1" : "0")); // Enable/disable (=1/=0) ICU usage in the codebase. NOTE: This flag is for use while integrating ICU and will be removed afterward.

		// If we're compiling with the engine, then add Core's engine dependencies
		if (UEBuildConfiguration.bCompileAgainstEngine == true)
		{
			if (!UEBuildConfiguration.bBuildRequiresCookedData)
			{
				DynamicallyLoadedModuleNames.AddRange(new string[] { "DerivedDataCache" });
			}
		}

		
		// On Windows platform, VSPerfExternalProfiler.cpp needs access to "VSPerf.h".  This header is included with Visual Studio, but it's not in a standard include path.
		if( Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 )
		{
			var VisualStudioVersionNumber = "11.0";
			var SubFolderName = ( Target.Platform == UnrealTargetPlatform.Win64 ) ? "x64/PerfSDK" : "PerfSDK";

			string PerfIncludeDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), String.Format("Microsoft Visual Studio {0}/Team Tools/Performance Tools/{1}", VisualStudioVersionNumber, SubFolderName));

			if (File.Exists(Path.Combine(PerfIncludeDirectory, "VSPerf.h")))
			{
				PrivateIncludePaths.Add(PerfIncludeDirectory);
				Definitions.Add("WITH_VS_PERF_PROFILER=1");
			}
		}


        if ((Target.Platform == UnrealTargetPlatform.XboxOne) ||
            (Target.Platform == UnrealTargetPlatform.WinRT) ||
            (Target.Platform == UnrealTargetPlatform.WinRT_ARM))
        {
            Definitions.Add("WITH_DIRECTXMATH=1");
        }
        else if ((Target.Platform == UnrealTargetPlatform.Win64) ||
                (Target.Platform == UnrealTargetPlatform.Win32))
        {
			// To enable this requires Win8 SDK
            Definitions.Add("WITH_DIRECTXMATH=0");  // Enable to test on Win64/32.

            //PublicDependencyModuleNames.AddRange(  // Enable to test on Win64/32.
            //    new string[] { 
            //    "DirectXMath" 
            //});
        }
        else
        {
            Definitions.Add("WITH_DIRECTXMATH=0");
        }
	}
}
