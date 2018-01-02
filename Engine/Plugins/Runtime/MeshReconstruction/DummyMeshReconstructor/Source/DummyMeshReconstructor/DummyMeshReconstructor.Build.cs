// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class DummyMeshReconstructor : ModuleRules
	{
		public DummyMeshReconstructor(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.Add("DummyMeshReconstructor/Private");

			PublicDependencyModuleNames.AddRange
			(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"MRMesh",
				}
			);
		}
	}
}
