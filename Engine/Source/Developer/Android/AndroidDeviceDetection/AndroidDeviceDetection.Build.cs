// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AndroidDeviceDetection : ModuleRules
{
	public AndroidDeviceDetection( TargetInfo Target )
	{
		BinariesSubFolder = "Android";

        PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateIncludePathModuleNames.AddRange(
			new string[]
			{
				"TcpMessaging",
			}
		);

		PublicIncludePaths.AddRange(
			new string[]
			{
				"Runtime/Core/Public/Android"
			}
		);

		if (UEBuildConfiguration.bCompileAgainstEngine)
		{
			PrivateDependencyModuleNames.Add("Engine");
		}

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
                "TcpMessaging"
            }
        );
    }
}
