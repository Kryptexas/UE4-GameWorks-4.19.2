// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SourceControl : ModuleRules
{
	public SourceControl(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateIncludePaths.Add("Developer/SourceControl/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"InputCore",
			}
		);

		if (Target.Platform == UnrealTargetPlatform.Linux && Target.Type == TargetType.Program)
		{
			PublicDefinitions.Add("SOURCE_CONTROL_WITH_SLATE=0");
		}
		else
		{
			PublicDefinitions.Add("SOURCE_CONTROL_WITH_SLATE=1");

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Slate",
					"SlateCore",
                    "EditorStyle"
				}
			);
		}

        if (Target.bBuildEditor)
        {
			PrivateDependencyModuleNames.AddRange(
                new string[] {
					"Engine",
					"UnrealEd",
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[] {
					"AssetTools"
				}
			);
	        
			CircularlyReferencedDependentModules.Add("UnrealEd");
        }

		if (Target.bBuildDeveloperTools)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"MessageLog",
				}
			);
		}
	}
}
