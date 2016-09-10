// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class HTML5Networking  : ModuleRules
	{
		public HTML5Networking(TargetInfo Target)
		{
			Definitions.Add("ONLINESUBSYSTEMUTILS_PACKAGE=1");

			PrivateIncludePaths.Add("OnlineSubsystemUtils/Private");

            PrivateDependencyModuleNames.AddRange(
                new string[] { 
                    "Core", 
                    "CoreUObject",
                    "Engine",
					"EngineSettings",
                    "ImageCore",
                    "Sockets",
					"PacketHandler",
                    "libWebSockets",
                    "zlib"
                }
            );

	        PublicDependencyModuleNames.Add("OnlineSubsystem");
		}
	}
}
