// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class LiveLinkEditor : ModuleRules
	{
		public LiveLinkEditor(ReadOnlyTargetRules Target) : base(Target)
        {
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "UnrealEd",
                    "Engine",
                    "Projects",
                    "DetailCustomizations",

                    "WorkspaceMenuStructure",
                    "EditorStyle",
                    "SlateCore",
                    "Slate",
                    "InputCore",

                    //"Messaging",
                    "LiveLinkInterface",
					"LiveLinkMessageBusFramework",
                    "BlueprintGraph",
					"LiveLink",
                    "AnimGraph",
                    "Persona"
                }
			); 

			PrivateIncludePaths.Add("/LiveLinkEditor/Private");
		}
	}
}
