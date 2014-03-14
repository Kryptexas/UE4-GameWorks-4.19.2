// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Messaging : ModuleRules
	{
		public Messaging(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"Networking",
				}
			); 
			
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Sockets",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/Messaging/Private",
					"Runtime/Messaging/Private/Bus",
					"Runtime/Messaging/Private/Bridge",
					"Runtime/Messaging/Private/Serialization",
				}
			);
		}
	}
}
