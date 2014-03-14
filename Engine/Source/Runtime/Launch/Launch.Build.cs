// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Launch : ModuleRules
{
	public Launch(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Runtime/Launch/Private");

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AutomationController",
				"OnlineSubsystem",
				"TaskGraph",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
                "MoviePlayer",
				"Networking",
				"PakFile",
				"Projects",
				"RenderCore",
				"RHI",
				"SandboxFile",
				"ShaderCore",
				"Slate",
				"Sockets",
			}
		);
        
        if( !UEBuildConfiguration.bBuildDedicatedServer)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[] {
			        "SlateRHIRenderer",
		        }
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
			        "SlateRHIRenderer",
		        }
            );
        }

		// UFS clients are not available in shipping builds
		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateDependencyModuleNames.AddRange(
					new string[] {
					"NetworkFile",
					"StreamingFile",
    				"AutomationWorker",
				}
			);
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"Renderer",
			}
		);

		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
			PublicDependencyModuleNames.Add("Messaging");
			PublicDependencyModuleNames.Add("SessionServices");
			PrivateIncludePaths.Add("Developer/DerivedDataCache/Public");

			// LaunchEngineLoop.cpp does a LoadModule() on OnlineSubsystem and OnlineSubsystemUtils when compiled WITH_ENGINE, so they must be marked as dependencies so that they get compiled and cleaned
			DynamicallyLoadedModuleNames.Add("OnlineSubsystem");
			DynamicallyLoadedModuleNames.Add("OnlineSubsystemUtils");
		}

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PublicIncludePathModuleNames.Add("ProfilerService");
			DynamicallyLoadedModuleNames.AddRange(new string[] { "TaskGraph", "RealtimeProfiler", "ProfilerService" });
		}

		if (UEBuildConfiguration.bBuildEditor == true)
		{
			PublicIncludePathModuleNames.Add("ProfilerClient");

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"SourceControl",
					"UnrealEd",
				}
			);


			// ExtraModules that are loaded when WITH_EDITOR=1 is true
			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"AutomationController",
					"AutomationWorker",
					"AutomationWindow",
					"ProfilerClient",
					"Toolbox",
					"GammaUI",
					"ModuleUI",
					"OutputLog",
					"TextureCompressor",
					"MeshUtilities"
				}
			);

			if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				PrivateDependencyModuleNames.Add("MainFrame");
			}
			else
			{
				DynamicallyLoadedModuleNames.Add("MainFrame");
			}
		}

		if ((Target.Platform == UnrealTargetPlatform.Win32) ||
						(Target.Platform == UnrealTargetPlatform.Win64))
		{
			DynamicallyLoadedModuleNames.Add("D3D11RHI");
			DynamicallyLoadedModuleNames.Add("XAudio2");

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				DynamicallyLoadedModuleNames.Add("VSAccessor");
			}
		}

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			DynamicallyLoadedModuleNames.Add("CoreAudio");
		}

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PrivateDependencyModuleNames.Add("OpenGLDrv");
			DynamicallyLoadedModuleNames.Add("IOSAudio");
			DynamicallyLoadedModuleNames.Add("IOSRuntimeSettings");
			PublicFrameworks.Add("OpenGLES");
			PublicFrameworks.Add("QuartzCore");

			PrivateDependencyModuleNames.Add("LaunchDaemonMessages");
		}

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("OpenGLDrv"); 
			PrivateDependencyModuleNames.Add("AndroidAudio");
			DynamicallyLoadedModuleNames.Add("AndroidRuntimeSettings");
		}

		if ((Target.Platform == UnrealTargetPlatform.Win32) ||
			(Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Mac))
		{
			DynamicallyLoadedModuleNames.Add("OpenGLDrv");
		}

        if (Target.Platform == UnrealTargetPlatform.HTML5 )
        {
            PrivateDependencyModuleNames.Add("HTML5Audio");
			if (Target.Architecture == "-win32")
			{
                PrivateDependencyModuleNames.Add("HTML5Win32");
                PublicIncludePathModuleNames.Add("HTML5Win32");
				AddThirdPartyPrivateStaticDependencies(Target, "SDL");
			}
        }


		// @todo ps4 clang bug: this works around a PS4/clang compiler bug (optimizations)
		if (Target.Platform == UnrealTargetPlatform.PS4)
		{
			bFasterWithoutUnity = true;
		}
	}
}
