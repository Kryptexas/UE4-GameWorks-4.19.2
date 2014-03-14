// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnlineSubsystemUtils : ModuleRules
{
	public OnlineSubsystemUtils(TargetInfo Target)
	{
		Definitions.Add("ONLINESUBSYSTEMUTILS_PACKAGE=1");

        PrivateIncludePaths.Add("Runtime/Online/OnlineSubsystemUtils/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject",
				"Engine", 
				"EngineSettings",
                "ImageCore",
				"Sockets",
				"OnlineSubsystem",
				"Voice"
			}
		);
	}
}
