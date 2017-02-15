// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StreamingFile : ModuleRules
{
	public StreamingFile(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "NetworkFile",
                "Sockets",
            }
        );
    }
}