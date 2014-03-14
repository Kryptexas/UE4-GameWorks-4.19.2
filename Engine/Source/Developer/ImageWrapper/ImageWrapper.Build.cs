// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ImageWrapper : ModuleRules
{
	public ImageWrapper(TargetInfo Target)
	{
		PrivateIncludePaths.Add("Developer/ImageWrapper/Private");

		Definitions.Add("WITH_UNREALPNG=1");
		Definitions.Add("WITH_UNREALJPEG=1");

		PrivateDependencyModuleNames.Add("Core");

		AddThirdPartyPrivateStaticDependencies(Target, 
			"zlib",
			"UElibPNG",
			"UElibJPG"
			);
	}
}
