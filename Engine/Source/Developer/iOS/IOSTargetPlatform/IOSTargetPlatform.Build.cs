// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IOSTargetPlatform : ModuleRules
{
	public IOSTargetPlatform(TargetInfo Target)
	{
		BinariesSubFolder = "IOS";
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"TargetPlatform",
				"Messaging",
				"TargetDeviceServices",
				"LaunchDaemonMessages",
				"Networking"
			}
		);
        PlatformSpecificDynamicallyLoadedModuleNames.Add("LaunchDaemonMessages");

		//This is somehow necessary for getting iOS to build on, at least, windows. It seems like the target platform is included for cooking, and thus it requirtes a bunch of other info.
		PublicIncludePaths.AddRange(
			new string[]
			{
				"Runtime/Core/Public/Apple",
				"Runtime/Core/Public/IOS",
			}
		);

		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
			PrivateDependencyModuleNames.Add("Engine");
		}

		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
            PublicAdditionalLibraries.Add("/System/Library/PrivateFrameworks/MobileDevice.Framework/Versions/Current/MobileDevice");
		}
	}
}
