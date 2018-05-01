// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HTTPChunkInstaller : ModuleRules
{
	public HTTPChunkInstaller(ReadOnlyTargetRules Target)
		: base(Target)
	{
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "BuildPatchServices",
                "Core",
				"ApplicationCore",
                "Engine",
                "Http",
                "Json",
                "PakFile",
            }
            );
    }
}
