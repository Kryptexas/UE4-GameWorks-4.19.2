// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DeviceProfileServices : ModuleRules
{

	public DeviceProfileServices(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/DeviceProfileServices/Private");

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				
				"CoreUObject",
				"TargetPlatform",
				"UnrealEd",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"TargetDeviceServices",
			}
		);
	}
}

