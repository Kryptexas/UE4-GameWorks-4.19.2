// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ImageWrapper : ModuleRules
{
	public ImageWrapper(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/ImageWrapper/Private",
				"Runtime/ImageWrapper/Private/Formats",
			});

		PublicDefinitions.Add("WITH_UNREALPNG=1");
		PublicDefinitions.Add("WITH_UNREALJPEG=1");

		PrivateDependencyModuleNames.Add("Core");

		AddEngineThirdPartyPrivateStaticDependencies(Target,
			"zlib",
			"UElibPNG",
			"UElibJPG"
		);

		// Add openEXR lib for windows builds.
		if ((Target.Platform == UnrealTargetPlatform.Win64) ||
			(Target.Platform == UnrealTargetPlatform.Win32) ||
			(Target.Platform == UnrealTargetPlatform.Mac) ||
			(Target.Platform == UnrealTargetPlatform.Linux && Target.Architecture.StartsWith("x86_64")))
		{
			PublicDefinitions.Add("WITH_UNREALEXR=1");
			AddEngineThirdPartyPrivateStaticDependencies(Target, "UEOpenExr");
		}
		else
		{
			PublicDefinitions.Add("WITH_UNREALEXR=0");
		}

		bEnableShadowVariableWarnings = false;

		// Enable exceptions to allow error handling
		bEnableExceptions = true;

		// Disable shared PCHs to handle warning C4652
		PCHUsage = ModuleRules.PCHUsageMode.NoSharedPCHs;
		PrivatePCHHeaderFile = "Private/ImageWrapperPrivate.h";
	}
}
