// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using AutomationTool;
using UnrealBuildTool;

[Help("Updates your local versions based on your P4 sync")]
[RequireP4]
public class UpdateLocalVersion : BuildCommand
{
	public override void ExecuteBuild()
	{
		var UE4Build = new UE4Build(this);
		UE4Build.UpdateVersionFiles();
	}
}
