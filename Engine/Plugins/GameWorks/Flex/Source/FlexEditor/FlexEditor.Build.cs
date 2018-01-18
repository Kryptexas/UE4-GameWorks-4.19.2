// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class FlexEditor : ModuleRules
	{
		public FlexEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					"FlexEditor/Public",
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"FlexEditor/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Flex",
				    	"Core",
					"CoreUObject",
					"AssetTools",
					"Engine",
					"UnrealEd",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "Engine",
                    "AdvancedPreviewScene",
                    "SlateCore",
                    "Slate",
                    "AssetRegistry",
                    "UnrealEd",
                    "ComponentVisualizers"
					// ... add private dependencies that you statically link with here ...
				}
                );

			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"Flex",
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);
		}
	}
}
