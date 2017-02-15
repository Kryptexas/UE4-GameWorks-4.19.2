// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class NullNetworkReplayStreaming : ModuleRules
	{
		public NullNetworkReplayStreaming( TargetInfo Target )
		{
			PrivateIncludePaths.Add( "Runtime/NetworkReplayStreaming/NullNetworkReplayStreaming/Private" );

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Engine",
					"NetworkReplayStreaming",
                    "Json",
				}
			);
		}
	}
}