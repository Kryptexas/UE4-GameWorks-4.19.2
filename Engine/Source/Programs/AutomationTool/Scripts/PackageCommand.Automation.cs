// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using AutomationTool;
using UnrealBuildTool;

public partial class Project : CommandUtils
{
	#region Package Command

	public static void Package(ProjectParams Params, int WorkingCL=-1)
	{
		Params.ValidateAndLog();
		if (!Params.Package)
		{
			return;
		}
		if (!Params.NoClient)
		{
			var DeployContextList = CreateDeploymentContext(Params, false, false);
			foreach ( var SC in DeployContextList )
			{
				if (SC.StageTargetPlatform.PackageViaUFE)
				{
					string ClientCmdLine = "-run=Package ";
					ClientCmdLine += "-Targetplatform=" + SC.StageTargetPlatform.PlatformType.ToString() + " ";
					ClientCmdLine += "-SourceDir=" + CombinePaths(Params.BaseStageDirectory, SC.StageTargetPlatform.PlatformType.ToString()) + " ";
					string ClientApp = CombinePaths(CmdEnv.LocalRoot, "Engine/Binaries/Win64/UnrealFrontend.exe");

					Log("Packaging via UFE:");
					Log("\tClientCmdLine: " + ClientCmdLine + "");

					//@todo UAT: Consolidate running of external applications like UFE (See 'RunProjectCommand' for other instances)
					PushDir(Path.GetDirectoryName(ClientApp));
					// Always start client process and don't wait for exit.
					ProcessResult ClientProcess = Run(ClientApp, ClientCmdLine, null, ERunOptions.NoWaitForExit);
					PopDir();
					if (ClientProcess != null)
					{
						do
						{
							Thread.Sleep(100);
						}
						while (ClientProcess.HasExited == false);
					}
				}
				else
				{
					SC.StageTargetPlatform.Package(Params, SC, WorkingCL);
				}
			}
		}
		if (Params.DedicatedServer)
		{
			var DeployContextList = CreateDeploymentContext(Params, true, false);
			foreach (var SC in DeployContextList)
			{
				SC.StageTargetPlatform.Package(Params, SC, WorkingCL);
			}
		}
	}

	#endregion
}
