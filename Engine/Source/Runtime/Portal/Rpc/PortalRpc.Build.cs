// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class PortalRpc : ModuleRules
	{
		public PortalRpc(TargetInfo Target)
		{
            DynamicallyLoadedModuleNames.AddRange(
                new string[] {
                    "Messaging",
                    "MessagingRpc",
                }
            );

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
				}
			);

            PrivateIncludePathModuleNames.AddRange(
                new string[] {
                    "Messaging",
                    "MessagingRpc",
                    "PortalServices",
                }
            );

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/Portal/Rpc/PortalRpc/Private",
				}
			);
		}
	}
}
