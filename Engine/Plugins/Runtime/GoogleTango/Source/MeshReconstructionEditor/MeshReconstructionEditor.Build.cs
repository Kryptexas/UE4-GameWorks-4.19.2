// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{

public class MeshReconstructionEditor : ModuleRules
{
	public MeshReconstructionEditor(ReadOnlyTargetRules Target): base(Target)
	{
		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"WorkspaceMenuStructure",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"MeshReconstructionEditor/Private",
				"MeshReconstructionEditor/Private/AssetTools",
				"MeshReconstructionEditor/Private/Factories",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"ContentBrowser",
				"Core",
				"CoreUObject",
				"EditorStyle",
				"Engine",
				"InputCore",
				"MediaAssets",
				"PropertyEditor",
				"RenderCore",
				"RHI",
				"ShaderCore",
				"Slate",
				"SlateCore",
				"TextureEditor",
				"UnrealEd",
				"MRMesh",
				"Tango",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"UnrealEd",
                                "MeshReconstructionEditor",
				"WorkspaceMenuStructure",
			}
		);
	}
}
}