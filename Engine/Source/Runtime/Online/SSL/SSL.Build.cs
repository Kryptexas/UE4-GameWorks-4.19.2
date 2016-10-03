// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SSL : ModuleRules
{
    public SSL(TargetInfo Target)
    {
        Definitions.Add("SSL_PACKAGE=1");

		bool bShouldUseModule = false;
		if (Target.Platform == UnrealTargetPlatform.Mac ||
			Target.Platform == UnrealTargetPlatform.Win32 ||
			Target.Platform == UnrealTargetPlatform.Win64)
		{
			bShouldUseModule = true;
		}

		PrivateDependencyModuleNames.AddRange(
			new string[] {
					"Core",
			}
			);

		if (bShouldUseModule)
		{
			Definitions.Add("WITH_SSL=1");

			PrivateIncludePaths.AddRange(
				new string[] {
					"Runtime/Online/SSL/Private",
				}
				);

			if (Target.Platform == UnrealTargetPlatform.Mac ||
				Target.Platform == UnrealTargetPlatform.Win32 ||
				Target.Platform == UnrealTargetPlatform.Win64)
			{
				AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
			}
		}
    }
}
