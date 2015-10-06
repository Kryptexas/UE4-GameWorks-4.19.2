// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SimplygonUtilities : ModuleRules
	{
		public SimplygonUtilities(TargetInfo Target)
		{
            BinariesSubFolder = "NotForLicensees";

			PublicIncludePaths.AddRange(
				new string[] {
					ModuleDirectory + "/Public/",
				}
				);

            PublicIncludePaths.AddRange(
				new string[] {
					ModuleDirectory + "/Private/",
					ModuleDirectory + "/Private/Layouts/",
				}
				);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"PropertyEditor",
					"MeshUtilities",
					"Landscape",
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"InputCore",
					"Slate",
					"SlateCore",
					"EditorStyle",
					"RHI",
				}
				);


			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"RenderCore",
					"UnrealEd",
					"InputCore",
					"LevelEditor",
					"EditorStyle", //Fonts etc.
					"ContentBrowser", //For asset syncing
					"ShaderCore", //For material rendering
					"Landscape",
					"RawMesh",
                    "MaterialUtilities"
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					"PropertyEditor",
					"MeshUtilities"
				}
				);

			if (UEBuildConfiguration.bCompileSimplygon == true)
			{
                AddThirdPartyPrivateStaticDependencies(Target, "Simplygon");
			}
		}
	}
}