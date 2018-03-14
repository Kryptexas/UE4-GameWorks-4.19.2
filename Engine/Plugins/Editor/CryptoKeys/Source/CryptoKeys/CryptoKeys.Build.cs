// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CryptoKeys : ModuleRules
{
	public CryptoKeys(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.Add("CryptoKeys/Classes");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"ApplicationCore",
				"Engine",
				"UnrealEd",
				"CryptoKeysOpenSSL",
				"Slate",
				"SlateCore",
				"GameProjectGeneration"
		});
	}
}
