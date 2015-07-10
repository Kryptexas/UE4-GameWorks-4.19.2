// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
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
		const string NoEditorCookPlatform = "MacNoEditor";
		const string ServerCookPlatform = "MacServer";
		const string ClientCookPlatform = "MacClient";

		if (bDedicatedServer)
		{
			return ServerCookPlatform;
		}
		else if (bIsClientOnly)
		{
			return ClientCookPlatform;
		}
		else
		{
			return NoEditorCookPlatform;
		}
	}

	public override string GetEditorCookPlatform()
	{
		return "Mac";
	}

	private void StageAppBundle(DeploymentContext SC, StagedFileType InStageFileType, string InPath, string NewName)
	{
		SC.StageFiles(InStageFileType, InPath, "*", true, null, NewName, false, true, null);
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
	
		if (Params.StageNonMonolithic)
		{
			if (SC.DedicatedServer)
			{
				if (SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Development))
				{
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UE4Server.app"));
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), "UE4Server-" + SC.ShortProjectName + ".dylib");
				}
				if (SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Test))
				{
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UE4Server-Mac-Test.app"));
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), "UE4Server-" + SC.ShortProjectName + "-Mac-Test.dylib");
				}
				if (SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Shipping))
				{
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UE4Server-Mac-Shipping.app"));
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), "UE4Server-" + SC.ShortProjectName + "-Mac-Shipping.dylib");
				}

				SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Plugins"), "UE4Server-*.dylib", true);
				SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Plugins"), "UE4Server-*.dylib", true);
			}
			else
			{
				if (SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Development))
				{
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UE4.app"));
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), "UE4-" + SC.ShortProjectName + ".dylib");
				}
				if (SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Test))
				{
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UE4-Mac-Test.app"));
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), "UE4-" + SC.ShortProjectName + "-Mac-Test.dylib");
				}
				if (SC.StageTargetConfigurations.Contains(UnrealTargetConfiguration.Shipping))
				{
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UE4-Mac-Shipping.app"));
					SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir), "UE4-" + SC.ShortProjectName + "-Mac-Shipping.dylib");
				}

				SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Plugins"), "UE4-*.dylib", true);
				SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Plugins"), "UE4-*.dylib", true, null, null, true);
			}
		}
		else
		{
			// the first app is the "main" one, the rest are marked as debug files for exclusion from chunking/distribution
			StagedFileType WorkingFileType = StagedFileType.NonUFS;

			List<string> Exes = GetExecutableNames(SC);
			foreach (var Exe in Exes)
			{
				string AppBundlePath = "";
				if (Exe.StartsWith(CombinePaths(SC.RuntimeProjectRootDir, "Binaries", SC.PlatformDir)))
				{
					AppBundlePath = CombinePaths(SC.ShortProjectName, "Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app");
					StageAppBundle(SC, WorkingFileType, CombinePaths(SC.ProjectRoot, "Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app"), AppBundlePath);
				}
				else if (Exe.StartsWith(CombinePaths(SC.RuntimeRootDir, "Engine/Binaries", SC.PlatformDir)))
				{
					AppBundlePath = CombinePaths("Engine/Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app");

					string AbsoluteBundlePath = CombinePaths (SC.LocalRoot, AppBundlePath);
					// ensure the ue4game binary exists, if applicable
					if (!SC.IsCodeBasedProject && !Directory.Exists(AbsoluteBundlePath) && !SC.bIsCombiningMultiplePlatforms)
					{
						Log("Failed to find app bundle " + AbsoluteBundlePath);
						throw new AutomationException(ErrorCodes.Error_MissingExecutable, "Could not find app bundle {0}. You may need to build the UE4 project with your target configuration and platform.", AbsoluteBundlePath);
					}

					StageAppBundle(SC, WorkingFileType, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, Path.GetFileNameWithoutExtension(Exe) + ".app"), AppBundlePath);
				}

				if (!string.IsNullOrEmpty(AppBundlePath))
				{
					SC.StageFiles(WorkingFileType, CombinePaths(SC.ProjectRoot, "Build/Mac"), "Application.icns", false, null, CombinePaths(AppBundlePath, "Contents/Resources"), true);

					if (Params.bUsesSteam)
					{
						SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Source/ThirdParty/Steamworks/Steamv132/sdk/redistributable_bin/osx32"), "libsteam_api.dylib", false, null, CombinePaths(AppBundlePath, "Contents/MacOS"), true);
					}
				}

				// the first app is the "main" one, the rest are marked as debug files for exclusion from chunking/distribution
				WorkingFileType = StagedFileType.DebugNonUFS;
			}
		}

		if (SC.bStageCrashReporter)
		{
			string CrashReportClientPath = CombinePaths("Engine/Binaries", SC.PlatformDir, "CrashReportClient.app");
			StageAppBundle(SC, StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "CrashReportClient.app"), CrashReportClientPath);
		}

		// Copy the splash screen, Mac specific
		SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.ProjectRoot, "Content/Splash"), "Splash.bmp", false, null, null, true);

		// CEF3 files
		if(Params.bUsesCEF3)
		{
			SC.StageFiles(StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries/ThirdParty/CEF3/Mac/"), "*", true, null, null, true);
			string UnrealCEFSubProcessPath = CombinePaths("Engine/Binaries", SC.PlatformDir, "UnrealCEFSubProcess.app");
			StageAppBundle(SC, StagedFileType.NonUFS, CombinePaths(SC.LocalRoot, "Engine/Binaries", SC.PlatformDir, "UnrealCEFSubProcess.app"), UnrealCEFSubProcessPath);
		}

		// Stage the bootstrap executable
		if(!Params.NoBootstrapExe)
		{
			foreach(BuildReceipt Receipt in SC.StageTargetReceipts)
			{
				BuildProduct Executable = Receipt.BuildProducts.FirstOrDefault(x => x.Type == BuildProductType.Executable);
				if(Executable != null)
				{
					// only create bootstraps for executables
					if (SC.NonUFSStagingFiles.ContainsKey(Executable.Path) && Executable.Path.Replace("\\", "/").Contains("/" + TargetPlatformType.ToString() + "/"))
					{
						string BootstrapArguments = "";
						if (!SC.IsCodeBasedProject && !ShouldStageCommandLine(Params, SC))
						{
							BootstrapArguments = String.Format("../../../{0}/{0}.uproject", SC.ShortProjectName);
						}

						string BootstrapExeName;
						if(SC.StageTargetConfigurations.Count > 1)
						{
							BootstrapExeName = Path.GetFileName(Executable.Path) + ".app";
						}
						else if(Params.IsCodeBasedProject)
						{
							BootstrapExeName = Receipt.GetProperty("TargetName", SC.ShortProjectName) + ".app";
						}
						else
						{
							BootstrapExeName = SC.ShortProjectName + ".app";
						}

						string AppPath = Executable.Path.Substring(0, Executable.Path.LastIndexOf(".app/") + 4);
						string AppRelativePath = SC.NonUFSStagingFiles[Executable.Path].Substring(0, SC.NonUFSStagingFiles[Executable.Path].LastIndexOf(".app/") + 4);
						StageBootstrapExecutable(SC, BootstrapExeName, AppPath, AppRelativePath, BootstrapArguments);
					}
				}
			}
		}
	}

	string GetValueFromInfoPlist(string InfoPlist, string Key, string DefaultValue = "")
	{
		string Value = DefaultValue;
		string KeyString = "<key>" + Key + "</key>";
		int KeyIndex = InfoPlist.IndexOf(KeyString);
		if (KeyIndex > 0)
		{
			int ValueStartIndex = InfoPlist.IndexOf("<string>", KeyIndex + KeyString.Length) + "<string>".Length;
			int ValueEndIndex = InfoPlist.IndexOf("</string>", ValueStartIndex);
			if (ValueStartIndex > 0 && ValueEndIndex > ValueStartIndex)
			{
				Value = InfoPlist.Substring(ValueStartIndex, ValueEndIndex - ValueStartIndex);
			}
		}
		return Value;
	}

	void StageBootstrapExecutable(DeploymentContext SC, string ExeName, string TargetFile, string StagedRelativeTargetPath, string StagedArguments)
	{
		string InputApp = CombinePaths(SC.LocalRoot, "Engine", "Binaries", SC.PlatformDir, "BootstrapPackagedGame.app");
		if (InternalUtils.SafeDirectoryExists(InputApp))
		{
			// Create the new bootstrap program
			string IntermediateDir = CombinePaths(SC.ProjectRoot, "Intermediate", "Staging");
			InternalUtils.SafeCreateDirectory(IntermediateDir);

			string IntermediateApp = CombinePaths(IntermediateDir, ExeName);
			if (Directory.Exists(IntermediateApp))
			{
				Directory.Delete(IntermediateApp, true);
			}
			CloneDirectory(InputApp, IntermediateApp);

			// Rename the executable
			string GameName = Path.GetFileNameWithoutExtension(ExeName);
			File.Move(CombinePaths(IntermediateApp, "Contents", "MacOS", "BootstrapPackagedGame"), CombinePaths(IntermediateApp, "Contents", "MacOS", GameName));

			// Copy the icon
			string SrcInfoPlistPath = CombinePaths(TargetFile, "Contents", "Info.plist");
			string SrcInfoPlist = File.ReadAllText(SrcInfoPlistPath);

			string IconName = GetValueFromInfoPlist(SrcInfoPlist, "CFBundleIconFile");
			if (!string.IsNullOrEmpty(IconName))
			{
				string IconPath = CombinePaths(TargetFile, "Contents", "Resources", IconName + ".icns");
				InternalUtils.SafeCreateDirectory(CombinePaths(IntermediateApp, "Contents", "Resources"));
				File.Copy(IconPath, CombinePaths(IntermediateApp, "Contents", "Resources", IconName + ".icns"));
			}

			// Update Info.plist contents
			string DestInfoPlistPath = CombinePaths(IntermediateApp, "Contents", "Info.plist");
			string DestInfoPlist = File.ReadAllText(DestInfoPlistPath);

			string AppIdentifier = GetValueFromInfoPlist(SrcInfoPlist, "CFBundleIdentifier");
			if (AppIdentifier == "com.epicgames.UE4Game")
			{
				AppIdentifier = "";
			}

			string Copyright = GetValueFromInfoPlist(SrcInfoPlist, "NSHumanReadableCopyright");
			string BundleVersion = GetValueFromInfoPlist(SrcInfoPlist, "CFBundleVersion", "1");
			string ShortVersion = GetValueFromInfoPlist(SrcInfoPlist, "CFBundleShortVersionString", "1.0");

			DestInfoPlist = DestInfoPlist.Replace("com.epicgames.BootstrapPackagedGame", string.IsNullOrEmpty(AppIdentifier) ? "com.epicgames." + GameName + "_bootstrap" : AppIdentifier + "_bootstrap");
			DestInfoPlist = DestInfoPlist.Replace("BootstrapPackagedGame", GameName);
			DestInfoPlist = DestInfoPlist.Replace("__UE4_ICON_FILE__", IconName);
			DestInfoPlist = DestInfoPlist.Replace("__UE4_APP_TO_LAUNCH__", StagedRelativeTargetPath);
			DestInfoPlist = DestInfoPlist.Replace("__UE4_COMMANDLINE__", StagedArguments);
			DestInfoPlist = DestInfoPlist.Replace("__UE4_COPYRIGHT__", Copyright);
			DestInfoPlist = DestInfoPlist.Replace("__UE4_BUNDLE_VERSION__", BundleVersion);
			DestInfoPlist = DestInfoPlist.Replace("__UE4_SHORT_VERSION__", ShortVersion);

			File.WriteAllText(DestInfoPlistPath, DestInfoPlist);

			StageAppBundle(SC, StagedFileType.NonUFS, IntermediateApp, ExeName);
		}
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		// package up the program, potentially with an installer for Mac
		PrintRunTime();
	}

	public override void ProcessArchivedProject(ProjectParams Params, DeploymentContext SC)
	{
		string ExeName = SC.StageExecutables[0];
		string BundlePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName + ".app");

		if (SC.bIsCombiningMultiplePlatforms)
		{
			// when combining multiple platforms, don't merge the content into the .app, use the one in the Binaries directory
			BundlePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName, "Binaries", "Mac", ExeName + ".app");
			if (!Directory.Exists(BundlePath))
			{
				// if the .app wasn't there, just skip out (we don't require executables when combining)
				return;
			}
		}

		string TargetPath = CombinePaths(BundlePath, "Contents", "UE4");
		if (!SC.bIsCombiningMultiplePlatforms)
		{
			if (Directory.Exists(BundlePath))
			{
				Directory.Delete(BundlePath, true);
			}

			string SourceBundlePath = CombinePaths(SC.ArchiveDirectory, SC.ShortProjectName, "Binaries", "Mac", ExeName + ".app");
			if (!Directory.Exists(SourceBundlePath))
			{
				SourceBundlePath = CombinePaths(SC.ArchiveDirectory, "Engine", "Binaries", "Mac", ExeName + ".app");
			}
			Directory.Move(SourceBundlePath, BundlePath);

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
		}

		// Update executable name, icon and entry in Info.plist
		string UE4GamePath = CombinePaths(BundlePath, "Contents", "MacOS", ExeName);
		if (ExeName != SC.ShortProjectName && File.Exists(UE4GamePath))
		{
			string GameExePath = CombinePaths(BundlePath, "Contents", "MacOS", SC.ShortProjectName);
			File.Delete(GameExePath);
			File.Move(UE4GamePath, GameExePath);

			string DefaultIconPath = CombinePaths(BundlePath, "Contents", "Resources", "UE4.icns");
			string CustomIconSrcPath = CombinePaths(BundlePath, "Contents", "Resources", "Application.icns");
			string CustomIconDestPath = CombinePaths(BundlePath, "Contents", "Resources", SC.ShortProjectName + ".icns");
			if (File.Exists(CustomIconSrcPath))
			{
				File.Delete(DefaultIconPath);
				File.Move(CustomIconSrcPath, CustomIconDestPath);
			}
			else
			{
				File.Move(DefaultIconPath, CustomIconDestPath);
			}

			string InfoPlistPath = CombinePaths(BundlePath, "Contents", "Info.plist");
			string InfoPlistContents = File.ReadAllText(InfoPlistPath);
			InfoPlistContents = InfoPlistContents.Replace(ExeName, SC.ShortProjectName);
			InfoPlistContents = InfoPlistContents.Replace("<string>UE4</string>", "<string>" + SC.ShortProjectName + "</string>");
			File.Delete(InfoPlistPath);
			File.WriteAllText(InfoPlistPath, InfoPlistContents);
		}

		if (!SC.bIsCombiningMultiplePlatforms)
		{
			// creating these directories when the content isn't moved into the application causes it 
			// to fail to load, and isn't needed
			Directory.CreateDirectory(CombinePaths(TargetPath, "Engine", "Binaries", "Mac"));
			Directory.CreateDirectory(CombinePaths(TargetPath, SC.ShortProjectName, "Binaries", "Mac"));
		}
	}

	public override ProcessResult RunClient(ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
		if (!File.Exists(ClientApp))
		{
			if (Directory.Exists(ClientApp + ".app"))
			{
				ClientApp += ".app/Contents/MacOS/" + Path.GetFileName(ClientApp);
			}
			else
			{
				Int32 BaseDirLen = Params.BaseStageDirectory.Length;
				string StageSubDir = ClientApp.Substring(BaseDirLen, ClientApp.IndexOf("/", BaseDirLen + 1) - BaseDirLen);
				ClientApp = CombinePaths(Params.BaseStageDirectory, StageSubDir, Params.ShortProjectName + ".app/Contents/MacOS/" + Params.ShortProjectName);
			}
		}

		PushDir(Path.GetDirectoryName(ClientApp));
		// Always start client process and don't wait for exit.
		ProcessResult ClientProcess = Run(ClientApp, ClientCmdLine, null, ClientRunFlags | ERunOptions.NoWaitForExit);
		PopDir();

		return ClientProcess;
	}

	public override bool IsSupported { get { return true; } }

	public override bool ShouldUseManifestForUBTBuilds(string AddArgs)
	{
		// don't use the manifest to set up build products if we are compiling Mac under Windows and we aren't going to copy anything back to the PC
		bool bIsBuildingRemotely = UnrealBuildTool.BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Mac;
		bool bUseManifest = !bIsBuildingRemotely || AddArgs.IndexOf("-CopyAppBundleBackToDevice", StringComparison.InvariantCultureIgnoreCase) > 0;
		return bUseManifest;
	}
	public override List<string> GetDebugFileExtentions()
	{
		return new List<string> { ".dsym" };
	}
	public override bool CanHostPlatform(UnrealTargetPlatform Platform)
	{
		if (Platform == UnrealTargetPlatform.IOS || Platform == UnrealTargetPlatform.Mac)
		{
			return true;
		}
		return false;
	}

	public override bool ShouldStageCommandLine(ProjectParams Params, DeploymentContext SC)
	{
		return false; // !String.IsNullOrEmpty(Params.StageCommandline) || !String.IsNullOrEmpty(Params.RunCommandline) || (!Params.IsCodeBasedProject && Params.NoBootstrapExe);
	}

	public override bool SignExecutables(DeploymentContext SC, ProjectParams Params)
	{
		// Sign everything we built
		List<string> FilesToSign = GetExecutableNames(SC);
		foreach (var File in FilesToSign)
		{
			CodeSign.SignMacFileOrFolder(File);
		}

		return true;
	}
}
