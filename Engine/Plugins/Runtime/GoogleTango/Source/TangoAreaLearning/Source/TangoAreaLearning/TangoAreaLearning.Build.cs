// Copyright 2017 Google Inc.

using UnrealBuildTool;

public class TangoAreaLearning : ModuleRules
{
	public TangoAreaLearning(ReadOnlyTargetRules Target) : base(Target)
	{
       PublicIncludePaths.AddRange(
			new string[] {
				"TangoAreaLearning/Public"
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[] {
				"TangoAreaLearning/Private",
				// ... add other private include paths required here ...
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
                "Engine",
                "Tango",
                "EcefTools",
                "TangoSDK"
				// ... add other public dependencies that you statically link with here ...
			}
		);

		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
