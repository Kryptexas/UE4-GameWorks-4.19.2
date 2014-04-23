// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DeviceManager : ModuleRules
{
	public DeviceManager(TargetInfo Target)
	{
		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"TargetDeviceServices",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"EditorStyle",
                "InputCore",
				"Slate",
				"TargetPlatform",
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Developer/DeviceManager/Private",
				"Developer/DeviceManager/Private/Models",
				"Developer/DeviceManager/Private/Widgets",
				"Developer/DeviceManager/Private/Widgets/Apps",
				"Developer/DeviceManager/Private/Widgets/Browser",
				"Developer/DeviceManager/Private/Widgets/Details",
				"Developer/DeviceManager/Private/Widgets/Processes",
				"Developer/DeviceManager/Private/Widgets/Shared",
				"Developer/DeviceManager/Private/Widgets/Toolbar",
			}
		);
	}
}
