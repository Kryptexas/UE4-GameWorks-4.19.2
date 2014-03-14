// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using AutomationTool;
using UnrealBuildTool;

public class MacPlatform : Platform
{
	public MacPlatform()
		: base(UnrealTargetPlatform.Mac)
	{ 
	}

    public override string GetCookPlatform(bool bDedicatedServer, bool bIsClientOnly, string CookFlavor)
    {
        const string ClientCookPlatform = "MacNoEditor";
        const string ServerCookPlatform = "MacServer";
        return bDedicatedServer ? ServerCookPlatform : ClientCookPlatform;
    }

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
		List<string> Exes = GetExecutableNames(SC);
		foreach (var Exe in Exes)
		{
			if (Exe.StartsWith(CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir)))
			{
				SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir, System.IO.Path.GetFileNameWithoutExtension(Exe) + ".app"));
			}
			else if (Exe.StartsWith(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir)))
			{
				SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, System.IO.Path.GetFileNameWithoutExtension(Exe) + ".app"));
			}
		}

		// Copy the splash screen, Mac specific
		SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Content/Splash"), "Splash.bmp", false, null, null, true);
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		// package up the program, potentially with an installer for Mac
		PrintRunTime();
	}

	public override bool IsSupported { get { return true; } }

	public override bool ShouldUseManifestForUBTBuilds()
	{
		// don't use the manifest to set up build products if we are compiling Mac under Windows and we aren't going to copy anything back to the PC
		bool bIsBuildingRemotely = UnrealBuildTool.ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac;
		bool bUseManifest = !bIsBuildingRemotely || UnrealBuildTool.Utils.GetEnvironmentVariable("ue.bCopyAppBundleBackToDevice", false);
		return bUseManifest;
	}
    public override string GUBP_GetPlatformFailureEMails(string Branch)
    {
        return "Michael.Trepka[epic]";
    }
}
