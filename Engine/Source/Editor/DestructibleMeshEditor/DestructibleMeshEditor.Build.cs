// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DestructibleMeshEditor : ModuleRules
{
	public DestructibleMeshEditor(TargetInfo Target)
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
                "EditorStyle",
				"UnrealEd",
				"DesktopPlatform"
			}
		);

		DynamicallyLoadedModuleNames.Add("PropertyEditor");

		AddThirdPartyPrivateStaticDependencies(Target, "FBX");

		SetupModulePhysXAPEXSupport(Target);
	}
}
