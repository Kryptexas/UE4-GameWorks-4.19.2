// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class GameLiveStreaming : ModuleRules
	{
		public GameLiveStreaming( ReadOnlyTargetRules Target ) : base(Target)
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
					"CoreUObject",
					"Engine",
					"Slate",
					"SlateCore",
					"RenderCore",
					"ShaderCore",
					"RHI",
				}
			);
		}
	}
}
