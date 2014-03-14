// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnlineSubsystem : ModuleRules
{
	public OnlineSubsystem(TargetInfo Target)
	{
        PublicIncludePaths.Add("Runtime/Online/OnlineSubsystem/Public/Interfaces");

		PrivateIncludePaths.Add("Runtime/Online/OnlineSubsystem/Private");		

		Definitions.Add("ONLINESUBSYSTEM_PACKAGE=1");

		PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject",
				"ImageCore",
				"Sockets"
			}
		);
	}
}
