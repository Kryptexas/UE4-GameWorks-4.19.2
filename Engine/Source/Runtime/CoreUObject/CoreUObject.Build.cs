// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CoreUObject : ModuleRules
{
	public CoreUObject(TargetInfo Target)
	{
		SharedPCHHeaderFile = "Runtime/CoreUObject/Public/CoreUObject.h";

		// Cannot use shared pchs due to Core including CoreUObject with different _API macros
		PCHUsage = PCHUsageMode.NoSharedPCHs;

		PrivateIncludePaths.Add("Runtime/CoreUObject/Private");

        PrivateIncludePathModuleNames.AddRange(
                new string[] 
			    {
				    "TargetPlatform",
			    }
            );

		PublicDependencyModuleNames.Add("Core");

		PrivateDependencyModuleNames.Add("Projects");

	}

}
