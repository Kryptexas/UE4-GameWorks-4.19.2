// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemAmazon : ModuleRules
{
	public OnlineSubsystemAmazon(TargetInfo Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
				"HTTP",
                "OnlineSubsystem", 
			}
			);
	}
}
