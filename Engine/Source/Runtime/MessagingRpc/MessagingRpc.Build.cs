// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class MessagingRpc : ModuleRules
	{
		public MessagingRpc(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
				}
			);

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Messaging",
                }
            );

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/MessagingRpc/Private",
				}
			);
		}
	}
}
