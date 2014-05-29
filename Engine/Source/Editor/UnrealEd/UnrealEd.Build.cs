// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class UnrealEd : ModuleRules
{
	public UnrealEd(TargetInfo Target)
	{
		SharedPCHHeaderFile = "Editor/UnrealEd/Public/UnrealEd.h";

		PrivateIncludePaths.AddRange(
			new string[] 
			{
				"Editor/UnrealEd/Private",
				"Editor/UnrealEd/Private/Settings",
				"Editor/PackagesDialog/Public",
				"Developer/DerivedDataCache/Public",
				"Developer/TargetPlatform/Public",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] 
			{
				"Analytics",
				"AssetRegistry",
				"AssetTools",
                "BehaviorTreeEditor",
				"ClassViewer",
				"ContentBrowser",
				"CrashTracker",
				"DerivedDataCache",
				"DesktopPlatform",
                "EnvironmentQueryEditor",
				"GameProjectGeneration",
				"ImageWrapper",
				"MainFrame",
				"MaterialEditor",
				"MeshUtilities",
				"Messaging",
				"NiagaraEditor",
				"PlacementMode",
				"Settings",
				"SoundClassEditor",
				"ViewportSnapping",
				"SourceCodeAccess",
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[] 
			{
				"BspMode",
				"Core",
				"CoreUObject",
				"Documentation",
				"Engine",
				"Projects",
				"SandboxFile",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"SourceControl",
				"UnrealEdMessages",
                "AIModule",
				"BlueprintGraph",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{ 
				"AnimGraph",
				"BlueprintGraph",
				"OnlineSubsystem",
				"OnlineBlueprintSupport",
				"DirectoryWatcher",
				"EditorSettingsViewer",
				"EditorStyle",
				"EngineSettings",
				"InputCore",
				"InputBindingEditor",
				"LauncherAutomatedService",
				"LauncherServices",
				"MaterialEditor",
				"MessageLog",
				"NetworkFileSystem",
				"ProjectSettingsViewer",
				"PropertyEditor",
				"Projects",
				"RawMesh",
				"RenderCore", 
				"RHI", 
				"ShaderCore", 
				"Sockets",
				"SoundClassEditor",
				"SoundCueEditor",
				"SourceControlWindows", 
				"StatsViewer",
				"SwarmInterface",
				"TargetPlatform",
				"TargetDeviceServices",
				"VectorVM",
                "EditorWidgets",
				"GraphEditor",
				"Kismet",
                "InternationalizationSettings"
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] 
			{
				"CrashTracker",
				"MaterialEditor",
				"FontEditor",
				"StaticMeshEditor",
				"TextureEditor",
				"Cascade",
                "UMGEditor",
				"Matinee",
				"AssetRegistry",
				"AssetTools",
				"ClassViewer",
				"CollectionManager",
				"ContentBrowser",
				"CurveTableEditor",
				"DataTableEditor",
				"DestructibleMeshEditor",
				"LandscapeEditor",
				"KismetCompiler",
				"DetailCustomizations",
				"ComponentVisualizers",
				"MainFrame",
				"LevelEditor",
				"InputBindingEditor",
				"PackagesDialog",
				"Persona",
				"PhAT",
				"DeviceManager",
				"SettingsEditor",
				"SessionFrontend",
				"SessionLauncher",
				"TaskBrowser",
				"Sequencer",
				"SoundClassEditor",
				"GeometryMode",
				"TextureAlignMode",
				"FoliageEdit",
				"PackageDependencyInfo",
				"ImageWrapper",
				"Blutility",
				"DesktopPlatform",
				"WorkspaceMenuStructure",
				"BspMode",
				"MeshPaint",
				"PlacementMode",
				"NiagaraEditor",
				"MeshUtilities",
				"GameProjectGeneration",
				"PListEditor",
                "Documentation",
                "BehaviorTreeEditor",
                "EnvironmentQueryEditor",
				"ViewportSnapping",
				"UserFeedback",
				"GameplayTagsEditor",
				"UndoHistory",
				"SourceCodeAccess"
			}
		);

		CircularlyReferencedDependentModules.AddRange(
			new string[] 
			{
                "GraphEditor",
				"Kismet",
            }
		); 


		// Add include directory for Lightmass
		PublicIncludePaths.Add("Programs/UnrealLightmass/Public");

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"UserFeedback",
             	"CollectionManager",
				"BlueprintGraph",
			}
			);

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			PublicDependencyModuleNames.Add("XAudio2");

			AddThirdPartyPrivateStaticDependencies(Target, 
				"UEOgg",
				"Vorbis",
				"VorbisFile",
				"DX11Audio"
				);

			
		}

        if (Target.Platform == UnrealTargetPlatform.HTML5)
        {
            PublicDependencyModuleNames.Add("HTML5Audio");
        }

		AddThirdPartyPrivateStaticDependencies(Target, 
			"HACD",
			"FBX",
			"FreeType2"
		);

		SetupModulePhysXAPEXSupport(Target);

		if ((UEBuildConfiguration.bCompileSimplygon == true) &&
			(Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees") == true) &&
			(Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees/Simplygon") == true))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "Simplygon");
		}
		else
		{
			Definitions.Add("WITH_SIMPLYGON=0");
		}

		if (UEBuildConfiguration.bCompileRecast)
		{
            PrivateDependencyModuleNames.Add("Navmesh");
			Definitions.Add( "WITH_RECAST=1" );
		}
		else
		{
			Definitions.Add( "WITH_RECAST=0" );
		}
	}
}
