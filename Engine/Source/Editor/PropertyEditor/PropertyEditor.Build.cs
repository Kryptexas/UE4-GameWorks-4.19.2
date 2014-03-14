// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PropertyEditor : ModuleRules
{
	public PropertyEditor(TargetInfo Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"UnrealEd",		
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Editor/PropertyEditor/Private",
				"Editor/PropertyEditor/Private/Presentation",
				"Editor/PropertyEditor/Private/Presentation/PropertyTable",
				"Editor/PropertyEditor/Private/Presentation/PropertyEditor",
				"Editor/PropertyEditor/Private/Presentation/PropertyDetails",
				"Editor/PropertyEditor/Private/UserInterface",
				"Editor/PropertyEditor/Private/UserInterface/PropertyTable",
				"Editor/PropertyEditor/Private/UserInterface/PropertyEditor",
				"Editor/PropertyEditor/Private/UserInterface/PropertyTree",
				"Editor/PropertyEditor/Private/UserInterface/PropertyDetails",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"MainFrame",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"EditorStyle",
				"Engine",
				"InputCore",
				"Slate",
				"EditorWidgets",
				"Documentation",
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"ContentBrowser",
				"Documentation",
				"MainFrame",
			}
		);
	}
}
