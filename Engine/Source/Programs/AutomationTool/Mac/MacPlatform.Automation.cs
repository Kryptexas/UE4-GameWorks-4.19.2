// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
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

    public override string GetEditorCookPlatform()
    {
        return "Mac";
    }

	private void StageAppBundle(DeploymentContext SC, string InPath, string NewName)
	{
		SC.StageFiles(StagedFileType.NonUFS, InPath, "*", true, null, NewName, false, true, null);
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
		List<string> Exes = GetExecutableNames(SC);
		foreach (var Exe in Exes)
		{
			if (Exe.StartsWith(CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir)))
			{
				StageAppBundle(SC, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app"), CombinePaths(SC.ShortProjectName, "Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app"));
			}
			else if (Exe.StartsWith(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir)))
			{
				StageAppBundle(SC, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app"), CombinePaths("Engine/Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app"));
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

	public override void ProcessArchivedProject(ProjectParams Params, DeploymentContext SC)
	{
		string BundlePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName + ".app");
		if (!Directory.Exists(BundlePath))
		{
			string SourceBundlePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName, "Binaries", "Mac", SC.ShortProjectName + ".app");
			if (!Directory.Exists(SourceBundlePath))
			{
				SourceBundlePath = CombinePaths(SC.ArchiveDirectory, "Engine", "Binaries", "Mac", "UE4Game.app");
			}
			Directory.Move(SourceBundlePath, BundlePath);
		}

		string TargetPath = CombinePaths(BundlePath, "Contents", "UE4");
		if (DirectoryExists(TargetPath))
		{
			Directory.Delete(TargetPath, true);
		}

		// First, move all files and folders inside he app bundle
		string[] StagedFiles = Directory.GetFiles(SC.ArchiveDirectory, "*", SearchOption.TopDirectoryOnly);
		foreach (string FilePath in StagedFiles)
		{
			string TargetFilePath = CombinePaths(TargetPath, Path.GetFileName(FilePath));
			Directory.CreateDirectory(Path.GetDirectoryName(TargetFilePath));
			File.Move(FilePath, TargetFilePath);
		}

		string[] StagedDirectories = Directory.GetDirectories(SC.ArchiveDirectory, "*", SearchOption.TopDirectoryOnly);
		foreach (string DirPath in StagedDirectories)
		{
			string DirName = Path.GetFileName(DirPath);
			if (!DirName.EndsWith(".app"))
			{
				string TargetDirPath = CombinePaths(TargetPath, DirName);
				Directory.CreateDirectory(Path.GetDirectoryName(TargetDirPath));
				Directory.Move(DirPath, TargetDirPath);
			}
		}

		// Update executable name and entry in Info.plist
		string ExeName = SC.StageExecutables[0];
		string UE4GamePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName + ".app", "Contents", "MacOS", ExeName);
		if (ExeName != SC.ShortProjectName && File.Exists(UE4GamePath))
		{
			string GameExePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName + ".app", "Contents", "MacOS", SC.ShortProjectName);
			File.Delete(GameExePath);
			File.Move(UE4GamePath, GameExePath);

			string InfoPlistPath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName + ".app", "Contents", "Info.plist");
			string InfoPlistContents = File.ReadAllText(InfoPlistPath);
			InfoPlistContents = InfoPlistContents.Replace(ExeName, SC.ShortProjectName);
			File.Delete(InfoPlistPath);
			File.WriteAllText(InfoPlistPath, InfoPlistContents);
		}

		Directory.CreateDirectory(CombinePaths(TargetPath, ExeName.StartsWith("UE4Game") ? "Engine" : SC.ShortProjectName, "Binaries", "Mac"));
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
    public override List<string> GetDebugFileExtentions()
    {
        return new List<string> {".dsym"};
    }
    public override bool CanHostPlatform(UnrealTargetPlatform Platform)
    {
        if (Platform == UnrealTargetPlatform.IOS || Platform == UnrealTargetPlatform.Mac)
        {
            return true;
        }
        return false;
    }
}
