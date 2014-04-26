// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Slate : ModuleRules
{
	public Slate(TargetInfo Target)
	{
//		SharedPCHHeaderFile = "Runtime/Slate/Public/Slate.h";

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core",
				"CoreUObject",
				"InputCore",
				"SlateCore",
			}
		);

        if (!UEBuildConfiguration.bBuildDedicatedServer)
        {
			AddThirdPartyPrivateStaticDependencies(Target, "FreeType2");
        }

		PrivateDependencyModuleNames.Add("Sockets");

		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/Slate/Private",
				"Runtime/Slate/Private/Docking",
				"Runtime/Slate/Private/DockingFramework",
				"Runtime/Slate/Private/Framework",
				"Runtime/Slate/Private/Framework/Animation",
				"Runtime/Slate/Private/Framework/Application",
				"Runtime/Slate/Private/Framework/Commands",
				"Runtime/Slate/Private/Framework/Styling",
				"Runtime/Slate/Private/Framework/Widgets",
				"Runtime/Slate/Private/IOS",
				"Runtime/Slate/Private/MultiBox",
				"Runtime/Slate/Private/TableView",
				"Runtime/Slate/Private/Testing",
				"Runtime/Slate/Private/Text",
			}
		);

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "XInput");
		}
	}
}
