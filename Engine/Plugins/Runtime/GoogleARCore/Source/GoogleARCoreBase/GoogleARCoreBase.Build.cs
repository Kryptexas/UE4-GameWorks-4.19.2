// Copyright 2017 Google Inc.


using System.IO;


namespace UnrealBuildTool.Rules
{
	public class GoogleARCoreBase : ModuleRules
	{
		public GoogleARCoreBase(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.AddRange(
				new string[]
				{
					"GoogleARCoreBase/Private",
				}
			);
			PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"HeadMountedDisplay",
						"AugmentedReality",
					}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"EngineSettings",
					"Slate",
					"SlateCore",
					"RHI",
					"RenderCore",
					"ShaderCore",
					"AndroidPermission",
					"GoogleARCoreRendering",
					"GoogleARCoreSDK",
					"OpenGL"
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[]
				{
					"Settings" // For editor settings panel.
				}
			);

			if (Target.Platform == UnrealTargetPlatform.Android)
			{
				// Additional dependencies on android...
				PrivateDependencyModuleNames.Add("Launch");

				// Register Plugin Language
				AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "GoogleARCoreBase_APL.xml"));
			}

			bFasterWithoutUnity = false;
		}
	}
}
