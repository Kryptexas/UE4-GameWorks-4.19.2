// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Net.NetworkInformation;
using System.Threading;
using AutomationTool;
using UnrealBuildTool;
using Ionic.Zip;

public class DesktopPlatform : Platform
{
	public DesktopPlatform()
		: base(UnrealTargetPlatform.Desktop)
	{
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{

		PrintRunTime();
	}

	public override void GetFilesToArchive(ProjectParams Params, DeploymentContext SC)
	{
	}

	public override void GetConnectedDevices(ProjectParams Params, out List<string> Devices)
	{
		Devices = new List<string>();
	}

	public override void Deploy(ProjectParams Params, DeploymentContext SC)
	{
	}

	public override ProcessResult RunClient(ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
		return null;
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
	}

	/// <summary>
	/// Gets cook platform name for this platform.
	/// </summary>
	/// <param name="CookFlavor">Additional parameter used to indicate special sub-target platform.</param>
	/// <returns>Cook platform string.</returns>
	public override string GetCookPlatform(bool bDedicatedServer, bool bIsClientOnly, string CookFlavor)
	{
		return "Desktop";
	}

	public override bool DeployPakInternalLowerCaseFilenames()
	{
		return false;
	}

	public override bool DeployLowerCaseFilenames(bool bUFSFile)
	{
		return false;
	}

	public override bool IsSupported { get { return true; } }

	public override string Remap(string Dest)
	{
		return Dest;
	}

	public override PakType RequiresPak(ProjectParams Params)
	{
		return PakType.DontCare;
	}
    
	public override List<string> GetDebugFileExtentions()
	{
		return new List<string> { };
	}
}
