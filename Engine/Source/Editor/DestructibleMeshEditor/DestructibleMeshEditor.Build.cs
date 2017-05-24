// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DestructibleMeshEditor : ModuleRules
{
	public DestructibleMeshEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.Add("Engine");

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"LevelEditor",
			}
		);

		PrivateIncludePaths.Add("Editor/UnrealEd/Private");	//compatibility for FBX importer
		PrivateIncludePathModuleNames.Add("UnrealEd");	//compatibility for FBX importer

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
				"RenderCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
				"UnrealEd",
				"DesktopPlatform"
			}
		);

		DynamicallyLoadedModuleNames.Add("PropertyEditor");

		AddEngineThirdPartyPrivateStaticDependencies(Target, "FBX");

		SetupModulePhysXAPEXSupport(Target);
	}
}
