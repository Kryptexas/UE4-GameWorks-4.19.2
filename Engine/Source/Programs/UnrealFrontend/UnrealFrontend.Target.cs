// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealFrontendTarget : TargetRules
{
	public UnrealFrontendTarget( TargetInfo Target ) : base(Target)
	{
		Type = TargetType.Program;
		LinkType = TargetLinkType.Modular;
		AdditionalPlugins.Add("UdpMessaging");
		LaunchModuleName = "UnrealFrontend";

		bBuildEditor = false;
		bCompileAgainstEngine = false;
		bCompileAgainstCoreUObject = true;
		bForceBuildTargetPlatforms = true;
		bCompileWithStatsWithoutEngine = true;
		bCompileWithPluginSupport = true;

		bHasExports = false;
	}
}
