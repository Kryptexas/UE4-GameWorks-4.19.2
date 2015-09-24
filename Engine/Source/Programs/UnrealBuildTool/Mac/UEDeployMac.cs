// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.IO;
using Ionic.Zip;

namespace UnrealBuildTool.IOS
{
	class UEDeployMac : UEBuildDeploy
	{
		/// <summary>
		/// Register the platform with the UEBuildDeploy class
		/// </summary>
		public override void RegisterBuildDeploy()
		{
			// TODO: print debug info and handle any cases that would keep this from registering
			UEBuildDeploy.RegisterBuildDeploy(UnrealTargetPlatform.Mac, this);
		}

		public override bool PrepTargetForDeployment(UEBuildTarget InTarget)
		{
			Log.TraceInformation("Deploying now!");
			return true;
		}
	}
}
