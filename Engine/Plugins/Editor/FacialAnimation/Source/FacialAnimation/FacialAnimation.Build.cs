// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class FacialAnimation : ModuleRules
	{
		public FacialAnimation(ReadOnlyTargetRules Target) : base(Target)
		{			
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"InputCore",
					"Engine",
                }
			);
		}
	}
}
