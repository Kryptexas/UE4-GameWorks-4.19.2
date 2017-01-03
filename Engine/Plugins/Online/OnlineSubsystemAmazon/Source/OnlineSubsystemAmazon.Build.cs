// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class OnlineSubsystemAmazon : ModuleRules
{
	public OnlineSubsystemAmazon(TargetInfo Target)
	{
		Definitions.Add("ONLINESUBSYSTEMAMAZON_PACKAGE=1");	
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core", 
                "CoreUObject",
				"HTTP",
				"Json",
                "OnlineSubsystem", 
			}
		);
	}
}
