// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class Internationalization : ModuleRules
	{
		public Internationalization(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
			); 

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Json",
				}
			); 

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/Internationalization/Private",
				}
			);
		}
	}
}
