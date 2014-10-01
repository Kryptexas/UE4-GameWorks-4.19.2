// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Serialization : ModuleRules
	{
		public Serialization(TargetInfo Target)
		{	
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Json",
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/Serialization/Private",
				}
			);
		}
	}
}
