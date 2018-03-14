// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class XGEController : ModuleRules
{
	public XGEController(ReadOnlyTargetRules TargetRules)
		: base(TargetRules)
	{
		PrivateIncludePaths.Add("Private");
		PublicIncludePaths.Add("Public");

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core"
		});
	}
}
