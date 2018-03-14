// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
    public class LiveLink : ModuleRules
    {
        public LiveLink(ReadOnlyTargetRules Target) : base(Target)
        {
            PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Projects",

                "Messaging",
                "LiveLinkInterface",
                "LiveLinkMessageBusFramework",
				"HeadMountedDisplay"
            }
            );

            PrivateIncludePaths.Add("/LiveLink/Private");
        }
    }
}
