// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ProfilerClient : ModuleRules
	{
		public ProfilerClient(TargetInfo Target)
		{
			PublicIncludePathModuleNames.Add("ProfilerService");

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"Messaging",
					"SessionServices",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"Networking",
					"ProfilerMessages",
				}
			);
		}
	}
}
