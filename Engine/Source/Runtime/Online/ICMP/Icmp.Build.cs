// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Icmp : ModuleRules
{
	public Icmp(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDefinitions.Add("ICMP_PACKAGE=1");

		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/Online/Icmp/Private",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core", 
				"CoreUObject",
                "Sockets",
			}
		);
	}
}
