// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DeviceProfileServices : ModuleRules
{

	public DeviceProfileServices(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Editor/DeviceProfileServices/Private");

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject", // @todo Mac: for some reason it's needed to link in debug on Mac
				"Engine",
				"UnrealEd",
				"TargetDeviceServices",
			}
		);

	}
}

