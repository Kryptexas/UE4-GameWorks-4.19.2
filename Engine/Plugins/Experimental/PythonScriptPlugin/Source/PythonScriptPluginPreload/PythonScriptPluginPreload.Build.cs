// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class PythonScriptPluginPreload : ModuleRules
	{
		public PythonScriptPluginPreload(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Python",
				}
			);
		}
	}
}
