// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class StreamingFile : ModuleRules
{
	public StreamingFile(TargetInfo Target)
	{
        PublicIncludePathModuleNames.Add("NetworkFile");

        PrivateDependencyModuleNames.AddRange(new string[] { "Core", "Sockets", "NetworkFile" });
	}
}