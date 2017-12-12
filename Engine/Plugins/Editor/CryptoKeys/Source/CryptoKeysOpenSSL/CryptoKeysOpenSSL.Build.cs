// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CryptoKeysOpenSSL : ModuleRules
{
	public CryptoKeysOpenSSL(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.Add("CryptoKeys/Classes");

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
	}
}
