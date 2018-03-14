// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class ProxyLODMeshReduction : ModuleRules
	{
		public ProxyLODMeshReduction(ReadOnlyTargetRules Target) : base(Target)
		{


            // For boost:: and TBB:: code
            bEnableUndefinedIdentifierWarnings = false;
            bUseRTTI = true;

            PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Experimental/ProxyLODPlugin/Private",
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
                    "UVAtlas",
                    "DirectXMesh",
                    "OpenVDB",
                    "UEOpenExr",
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "UnrealEd",
                    "RawMesh",
                    "MeshUtilities",
                    "MaterialUtilities",
                    "PropertyEditor",
                    "SlateCore",
                    "Slate",
                    "EditorStyle",
                    "RenderCore",
                    "RHI",
                    "QuadricMeshReduction"
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "Core",
                    "CoreUObject",
                    "Engine",
                    "UnrealEd"
					// ... add private dependencies that you statically link with here ...
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
}
