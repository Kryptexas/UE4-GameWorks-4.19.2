// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class USDImporter : ModuleRules
	{
		public USDImporter(ReadOnlyTargetRules Target) : base(Target)
        {

			PublicIncludePaths.AddRange(
				new string[] {
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"Editor/USDImporter/Private",
					ModuleDirectory + "/../UnrealUSDWrapper/Source/Public",
				}
				);

		
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealEd",
					"InputCore",
					"SlateCore",
                    "PropertyEditor",
					"Slate",
					"RawMesh",
                    "GeometryCache",
					"MeshUtilities",
                    "RenderCore",
                    "RHI",
                    "MessageLog",
                }
				);

			string BaseLibDir = ModuleDirectory + "/../UnrealUSDWrapper/Lib/";

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				string LibraryPath = BaseLibDir + "x64/Release/";
				PublicAdditionalLibraries.Add(LibraryPath+"/UnrealUSDWrapper.lib");

                foreach (string FilePath in Directory.EnumerateFiles(Path.Combine(ModuleDirectory, "../../Binaries/Win64/"), "*.dll", SearchOption.AllDirectories))
                {
                    RuntimeDependencies.Add(new RuntimeDependency(FilePath));
                }
            }
		}
	}
}