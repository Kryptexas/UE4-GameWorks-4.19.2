// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CoreUObject : ModuleRules
{
	public CoreUObject(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivatePCHHeaderFile = "Private/CoreUObjectPrivatePCH.h";

		SharedPCHHeaderFile = "Public/CoreUObjectSharedPCH.h";

		PrivateIncludePaths.Add("Runtime/CoreUObject/Private");

        PrivateIncludePathModuleNames.AddRange(
                new string[] 
			    {
				    "TargetPlatform",
			    }
            );

		PublicDependencyModuleNames.Add("Core");

		PrivateDependencyModuleNames.Add("Projects");
		PrivateDependencyModuleNames.Add("ApplicationCore");
        PrivateDependencyModuleNames.Add("Json");

	}

}
