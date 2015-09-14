// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class QoSReporter : ModuleRules
	{
		public QoSReporter(TargetInfo Target)
		{
			PublicDependencyModuleNames.Add("Core");

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Analytics",
					"HTTP",
					"Json",
				}
			);

			Definitions.Add("WITH_QOSREPORTER=1");
		}
	}
}
