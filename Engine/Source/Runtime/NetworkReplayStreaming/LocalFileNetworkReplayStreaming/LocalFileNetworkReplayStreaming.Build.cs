// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class LocalFileNetworkReplayStreaming : ModuleRules
    {
        public LocalFileNetworkReplayStreaming(ReadOnlyTargetRules Target) : base(Target)
        {
			PrivateDependencyModuleNames.AddRange(
                new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
					"NetworkReplayStreaming",
					"Json"
				} );
        }
    }
}
