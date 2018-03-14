// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class NetcodeUnitTest : ModuleRules
	{
		public NetcodeUnitTest(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateIncludePaths.Add("NetcodeUnitTest/Private");

			PublicDependencyModuleNames.AddRange
			(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"Sockets"
				}
			);

			PrivateDependencyModuleNames.AddRange
			(
				new string[]
				{
					"ApplicationCore",
					"EngineSettings",
					"InputCore",
					"PacketHandler",
					"Slate",
					"SlateCore"
				}
			);

			// @todo #JohnBLowpri: Currently don't support standalone commandlet, with static builds (can't get past linker error in Win32)
			if (Target.LinkType != TargetLinkType.Monolithic)
			{
				PrivateDependencyModuleNames.AddRange
				(
					new string[]
					{
						"StandaloneRenderer"
					}
				);
			}
		}
	}
}

