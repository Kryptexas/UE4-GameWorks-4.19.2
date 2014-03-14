// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Slate : ModuleRules
{
	public Slate(TargetInfo Target)
	{
//		SharedPCHHeaderFile = "Runtime/Slate/Public/Slate.h";

		PrivateIncludePathModuleNames.Add("VSAccessor");

		PublicDependencyModuleNames.AddRange(
			new string[] { 
				"Core",
				"CoreUObject",
				"InputCore",
			}
			);

        if (!UEBuildConfiguration.bBuildDedicatedServer)
        {
			AddThirdPartyPrivateStaticDependencies(Target, "FreeType2");
        }

        PrivateDependencyModuleNames.Add( "Sockets" );

		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32))
		{
			AddThirdPartyPrivateStaticDependencies(Target, "XInput");
			if (UEBuildConfiguration.bBuildEditor == true)
			{
				DynamicallyLoadedModuleNames.Add("VSAccessor");
			}
		}
	}
}
