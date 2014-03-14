// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Engine : ModuleRules
{
	public Engine(TargetInfo Target)
	{
		SharedPCHHeaderFile = "Runtime/Engine/Public/Engine.h";

		PublicIncludePathModuleNames.AddRange( new string[] { "Renderer" } );

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/DerivedDataCache/Public",
				"Runtime/Online/OnlineSubsystem/Public",
                "Developer/SynthBenchmark/Public",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"CrashTracker",
				"OnlineSubsystem", 
				"TargetPlatform",
				"DerivedDataCache",
				"ImageWrapper",
				"HeadMountedDisplay",
			}
		);

		if (Target.Configuration != UnrealTargetConfiguration.Shipping)
		{
			PrivateIncludePathModuleNames.AddRange(new string[] { "TaskGraph" });
		}

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core",
				"CoreUObject",
				"Slate",
				"InputCore",
				"Messaging",
				"RenderCore", 
				"RHI",
				"ShaderCore",
				"AssetRegistry", // Here until FAssetData is moved to engine
				"EngineMessages",
				"EngineSettings",
				"SynthBenchmark",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Networking",
				"Sockets",
				"Slate",
				"VectorVM",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"MovieSceneCore",
				"MovieSceneCoreTypes",
				"HeadMountedDisplay",
			}
		);

		if (UEBuildConfiguration.bBuildDeveloperTools)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"ImageCore",
					"RawMesh"
				});

			// Add "BlankModule" so that it gets compiled as an example and will be maintained and tested.  This can be removed
			// at any time if needed.  The module isn't actually loaded by the engine so there is no runtime cost.
			DynamicallyLoadedModuleNames.Add("BlankModule");

			PrivateIncludePathModuleNames.Add("MeshUtilities");
			DynamicallyLoadedModuleNames.Add("MeshUtilities");

			if (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)
			{
				PrivateDependencyModuleNames.Add("CollisionAnalyzer");
				CircularlyReferencedDependentModules.Add("CollisionAnalyzer");

				PrivateDependencyModuleNames.Add("LogVisualizer");
				CircularlyReferencedDependentModules.Add("LogVisualizer");
			}

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				DynamicallyLoadedModuleNames.AddRange(
					new string[] {
						"WindowsTargetPlatform",
						"WindowsNoEditorTargetPlatform",
						"WindowsServerTargetPlatform",
						"WindowsClientTargetPlatform",
					}
				);
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				DynamicallyLoadedModuleNames.AddRange(
					new string[] {
						"MacTargetPlatform",
						"MacNoEditorTargetPlatform"
					}
				);
			}
		}

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"Analytics",
				"AnalyticsET",
				"OnlineSubsystem", 
			}
		);

		if (!UEBuildConfiguration.bBuildDedicatedServer)
        {
		    DynamicallyLoadedModuleNames.AddRange(
			    new string[] {
				    "CrashTracker",
				    "ImageWrapper",
			    }
		    );
        }

		if (!UEBuildConfiguration.bBuildRequiresCookedData && UEBuildConfiguration.bCompileAgainstEngine)
		{
			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"DerivedDataCache", 
					"TargetPlatform"
				}
			);
		}

		if (UEBuildConfiguration.bBuildEditor == true)
		{
			PublicDependencyModuleNames.Add("UnrealEd");	// @todo api: Only public because of WITH_EDITOR and UNREALED_API
			CircularlyReferencedDependentModules.Add("UnrealEd");

			//PrivateDependencyModuleNames.Add("BlueprintGraph");
			//CircularlyReferencedDependentModules.Add("BlueprintGraph");

			PrivateIncludePathModuleNames.Add("TextureCompressor");
			PrivateIncludePaths.Add("Developer/TextureCompressor/Public");
		}

		SetupModulePhysXAPEXSupport(Target);

		if (UEBuildConfiguration.bCompileNetworkProfiler)
		{
			Definitions.Add("USE_NETWORK_PROFILER=1");
		}

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			AddThirdPartyPrivateStaticDependencies(Target,
				"UEOgg",
				"Vorbis",
				"VorbisFile"
				);

			if (UEBuildConfiguration.bCompileLeanAndMeanUE == false)
			{
				AddThirdPartyPrivateStaticDependencies(Target, "DirectShow");
			}

            // Head Mounted Display support
//            PrivateIncludePathModuleNames.AddRange(new string[] { "HeadMountedDisplay" });
//            DynamicallyLoadedModuleNames.AddRange(new string[] { "HeadMountedDisplay" });
		}

		if (Target.Platform == UnrealTargetPlatform.HTML5 && Target.Architecture == "-win32")
        {
			AddThirdPartyPrivateStaticDependencies(Target, 
                    "UEOgg",
                    "Vorbis",
                    "VorbisFile"
                    );
        }

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			AddThirdPartyPrivateStaticDependencies(Target, 
				"UEOgg",
				"Vorbis"
				);
		}

		if (Target.Platform == UnrealTargetPlatform.Android)
        {
			AddThirdPartyPrivateStaticDependencies(Target,
				"UEOgg",
				"Vorbis",
				"VorbisFile"
				);
        }

		if (UEBuildConfiguration.bCompileRecast)
		{
			AddThirdPartyPrivateStaticDependencies(Target, "Recast");
			Definitions.Add("WITH_RECAST=1");
		}
		else
		{
			// Because we test WITH_RECAST in public Engine header files, we need to make sure that modules
			// that import us also have this definition set appropriately.  Recast is a private dependency
			// module, so it's definitions won't propagate to modules that import Engine.
			Definitions.Add("WITH_RECAST=0");
		}

		if ((UEBuildConfiguration.bCompileSpeedTree == true) &&
			(Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "SpeedTree");

			// Because we test WITH_SPEEDTREE in public UnrealEd header files, we need to make sure that modules
			// that import us also have this definition set appropriately.  SpeedTree is a private dependency
			// module, so it's definitions won't propagate to modules that import UnrealEd.
			Definitions.Add("WITH_SPEEDTREE=1");
		}
		else
		{
			Definitions.Add("WITH_SPEEDTREE=0");
		}
	}
}
