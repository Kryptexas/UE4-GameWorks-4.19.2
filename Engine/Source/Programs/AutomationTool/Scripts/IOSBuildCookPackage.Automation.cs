// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using AutomationTool;
using UnrealBuildTool;

[Help("Builds the current package for IOS")]
class IOSBuildCookPackage : BuildCookRun
{
	public override void ExecuteBuild()
	{
		var Params = SetupParams();
		Params.ClientTargetPlatforms.Add(UnrealTargetPlatform.IOS);

		// if the commandline isn't already set, then set it now
		if (String.IsNullOrEmpty(Params.StageCommandline))
		{
            Params.StageCommandline = String.Format("{0} {1}", Params.IsCodeBasedProject ?
				Params.ClientTargetPlatformInstances[0].LocalPathToTargetPath(Params.RawProjectPath, CmdEnv.LocalRoot) : "", Params.MapToRun).Trim();
		}

		DoBuildCookRun(Params);
	}
}
