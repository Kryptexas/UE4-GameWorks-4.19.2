// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ShaderCore : ModuleRules
{
	public ShaderCore(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateIncludePaths.AddRange( new string[] { "Developer/DerivedDataCache/Public" });

        PublicDependencyModuleNames.AddRange(new string[] { "RHI", "RenderCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Core" });

		PrivateIncludePathModuleNames.AddRange(new string[] { "DerivedDataCache", "TargetPlatform" });
	}
}
