// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Stomp : ModuleRules
{
    public Stomp(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDefinitions.Add("STOMP_PACKAGE=1");

		bool bShouldUseModule = 
			Target.Platform == UnrealTargetPlatform.Win32 ||
			Target.Platform == UnrealTargetPlatform.Win64 ||
			Target.Platform == UnrealTargetPlatform.Mac ||
			Target.Platform == UnrealTargetPlatform.Linux ||
			Target.Platform == UnrealTargetPlatform.XboxOne ||
			Target.Platform == UnrealTargetPlatform.PS4;

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core"
			}
		);

		if (bShouldUseModule)
		{
			PublicDefinitions.Add("WITH_STOMP=1");

			PrivateIncludePaths.AddRange(
				new string[]
				{
				"Runtime/Online/Stomp/Private",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
				"WebSockets"
				}
			);
		}
		else
		{
			PublicDefinitions.Add("WITH_STOMP=0");
		}
	}
}
