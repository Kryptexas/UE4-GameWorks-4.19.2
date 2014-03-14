// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
using AutomationTool;
using UnrealBuildTool;

public class AndroidPlatform : Platform
{
	public AndroidPlatform()
		:base(UnrealTargetPlatform.Android)
	{
	}

	private static string GetArchitecture(ProjectParams Params)
	{
		// @todo android: Need the architecture passed through from the ProjectParams!
		return "-armv7";
	}

	private static string GetFinalSOName(ProjectParams Params, string DecoratedExeName)
	{
		return Path.Combine(Path.GetDirectoryName(Params.ProjectGameExeFilename), DecoratedExeName) + GetArchitecture(Params) + ".so";
	}

	private static string GetFinalApkName(ProjectParams Params, string DecoratedExeName, bool bRenameUE4Game)
	{
		string ProjectDir = Path.GetDirectoryName(Path.GetFullPath(Params.RawProjectPath));
		// Apk's go to project location, not necessarily where the .so is (content only packages need to output to their directory)
		string ApkName = Path.Combine(ProjectDir, "Binaries/Android", DecoratedExeName) + GetArchitecture(Params) + ".apk";
		
		// if the source binary was UE4Game, handle using it or switching to project name
		if (Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename) == "UE4Game")
		{
			if (bRenameUE4Game)
			{
				// replace UE4Game with project name (only replace in the filename part)
				ApkName = Path.Combine(Path.GetDirectoryName(ApkName), Path.GetFileName(ApkName).Replace("UE4Game", Params.ShortProjectName));
			}
			else
			{
				// if we want to use UE4 directly then use it from the engine directory not project directory
				ApkName = ApkName.Replace(ProjectDir, Path.Combine(CmdEnv.LocalRoot, "Engine"));
			}
		}

		Console.WriteLine("APKName = " + ApkName + " ::: SO name = " + GetFinalSOName(Params, DecoratedExeName));

		return ApkName;
	}

	private static string GetFinalObbName(string ApkName)
	{
		// calculate the name for the .obb file
		string PackageName = GetPackageInfo(ApkName, false);
		if (PackageName == null)
		{
			throw new AutomationException("Failed to get package name from " + ApkName);
		}

		string PackageVersion = GetPackageInfo(ApkName, true);
		if (PackageVersion == null || PackageVersion.Length == 0)
		{
			throw new AutomationException("Failed to get package version from " + ApkName);
		}

		if (PackageVersion.Length > 0)
		{
			int IntVersion = int.Parse(PackageVersion);
			PackageVersion = IntVersion.ToString("00000");
		}

		string ObbName = string.Format("main.{0}.{1}.obb", PackageVersion, PackageName);

		// plop the .obb right next to the executable
		ObbName = Path.Combine(Path.GetDirectoryName(ApkName), ObbName);

		return ObbName;
	}

	private static string GetDeviceObbName(string ApkName)
	{
		string ObbName = GetFinalObbName(ApkName);
		string PackageName = GetPackageInfo(ApkName, false);
		return "/mnt/sdcard/obb/" + PackageName + "/" + Path.GetFileName(ObbName);
	}

	private static string GetFinalBatchName(string ApkName, ProjectParams Params)
	{
		return Path.Combine(Path.GetDirectoryName(ApkName), "Install_" + Params.ShortProjectName + "_" + Params.ClientConfigsToBuild[0].ToString() + ".bat");
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		string SOName = GetFinalSOName(Params, SC.StageExecutables[0]);
		string ApkName = GetFinalApkName(Params, SC.StageExecutables[0], true);
		string BatchName = GetFinalBatchName(ApkName, Params);

		// packaging just takes a pak file and makes it the .obb
		UEBuildDeploy Deploy = UEBuildDeploy.GetBuildDeploy(UnrealTargetPlatform.Android);
		Deploy.PrepForUATPackageOrDeploy(Params.ShortProjectName, SC.ProjectRoot, SOName, SC.LocalRoot + "/Engine", Params.Distribution);
		
		// first, look for a .pak file in the staged directory
		string[] PakFiles = Directory.GetFiles(SC.StageDirectory, "*.pak", SearchOption.AllDirectories);

		// for now, we only support 1 pak/obb file
		if (PakFiles.Length != 1)
		{
			throw new AutomationException("Can't package for Android with 0 or more than 1 pak file (found {0} pak files in {1})", PakFiles.Length, SC.StageDirectory);
		}

		string LocalObbName = GetFinalObbName(ApkName);
		string DeviceObbName = GetDeviceObbName(ApkName);

		Log("Creating {0} from {1}", LocalObbName, PakFiles[0]);
		File.Delete(LocalObbName);
		File.Copy(PakFiles[0], LocalObbName);

		string PackageName = GetPackageInfo(ApkName, false);
		// make a batch file that can be used to install the .apk and .obb files
		string[] BatchLines = new string[] {
			"setlocal",
			"set ADB=%ANDROID_HOME%\\platform-tools\\adb.exe",
			"%ADB% uninstall " + PackageName,
			"%ADB% install " + Path.GetFileName(ApkName),
			"%ADB% shell rm -rf /mnt/sdcard/" + Params.ShortProjectName,
			SC.IsCodeBasedProject ? "" : "%ADB% shell rm -rf /mnt/sdcard/UE4Game/UE4CommandLine.txt", // we need to delete the commandline in UE4Game or it will mess up loading
			"%ADB% shell rm -rf /mnt/sdcard/obb/" + PackageName,
			"%ADB% push " + Path.GetFileName(LocalObbName) + " " + DeviceObbName,
		};
		File.WriteAllLines(BatchName, BatchLines);

 		PrintRunTime();
	}

	public override void GetFilesToArchive(ProjectParams Params, DeploymentContext SC)
	{
		if (SC.StageTargetConfigurations.Count != 1)
		{
			throw new AutomationException("Android is currently only able to package one target configuration at a time, but StageTargetConfigurations contained {0} configurations", SC.StageTargetConfigurations.Count);
		}
		var TargetConfiguration = SC.StageTargetConfigurations[0];

		string ApkName = GetFinalApkName(Params, SC.StageExecutables[0], true);
		string ObbName = GetFinalObbName(ApkName);
		string BatchName = GetFinalBatchName(ApkName, Params);

		// verify the files exist
		if (!FileExists(ApkName))
		{
			throw new AutomationException("ARCHIVE FAILED - {0} was not found", ApkName);
		}
		if (!FileExists(ObbName))
		{
			throw new AutomationException("ARCHIVE FAILED - {0} was not found", ObbName);
		}

		SC.ArchiveFiles(Path.GetDirectoryName(ApkName), Path.GetFileName(ApkName));
		SC.ArchiveFiles(Path.GetDirectoryName(ObbName), Path.GetFileName(ObbName));
		SC.ArchiveFiles(Path.GetDirectoryName(BatchName), Path.GetFileName(BatchName));
	}

	private string GetAdbCommand(ProjectParams Params)
	{
		string SerialNumber = Params.Device;
		if (SerialNumber.Contains("@"))
		{
			// get the bit after the @
			SerialNumber = SerialNumber.Split("@".ToCharArray())[1];
		}

		if (SerialNumber != "")
		{
			SerialNumber = " -s " + SerialNumber;
		}
		return Environment.ExpandEnvironmentVariables("/c %ANDROID_HOME%/platform-tools/adb.exe" + SerialNumber + " ");
	}

	public override void Deploy(ProjectParams Params, DeploymentContext SC)
	{
		string AdbCommand = GetAdbCommand(Params);
		string ApkName = GetFinalApkName(Params, SC.StageExecutables[0], false);
		string DeviceObbName = GetDeviceObbName(ApkName);
		string PackageName = GetPackageInfo(ApkName, false);

		// install the apk
		string UninstallCommandline = AdbCommand + "uninstall " + PackageName;
		RunAndLog(CmdEnv, CmdEnv.CmdExe, UninstallCommandline);

		string InstallCommandline = AdbCommand + "install \"" + ApkName + "\"";
		RunAndLog(CmdEnv, CmdEnv.CmdExe, InstallCommandline);

		// copy files to device if we were staging
		if (SC.Stage)
		{
			// cache some strings
			string BaseCommandline = AdbCommand + "push";
			string RemoteDir = "/mnt/sdcard/" + Params.ShortProjectName;
			string UE4GameRemoteDir = "/mnt/sdcard/" + Params.ShortProjectName;

			// make sure device is at a clean state
			Run(CmdEnv.CmdExe, AdbCommand + "shell rm -rf " + RemoteDir);
			Run(CmdEnv.CmdExe, AdbCommand + "shell rm -rf " + UE4GameRemoteDir);

			string[] Files = Directory.GetFiles(SC.StageDirectory, "*", SearchOption.AllDirectories);
			// copy each UFS file
			foreach (string Filename in Files)
			{
				// don't push the apk, we install it
				if (Path.GetExtension(Filename).Equals(".apk", StringComparison.InvariantCultureIgnoreCase))
				{
					continue;
				}

				string FinalRemoteDir = RemoteDir;
				// handle the special case of the UE4Commandline.txt when using content only game (UE4Game)
				if (!Params.IsCodeBasedProject &&
					Path.GetFileName(Filename).Equals("UE4CommandLine.txt", StringComparison.InvariantCultureIgnoreCase))
				{
					FinalRemoteDir = "/mnt/sdcard/UE4Game";
				}

				string RemoteFilename = Filename.Replace(SC.StageDirectory, FinalRemoteDir).Replace("\\", "/");
 				string Commandline = string.Format("{0} \"{1}\" \"{2}\"", BaseCommandline, Filename, RemoteFilename);
 				Run(CmdEnv.CmdExe, Commandline);
			}

			// delete the .obb file, since it will cause nothing we just deployed to be used
			Run(CmdEnv.CmdExe, AdbCommand + "shell rm -f " + DeviceObbName);
		}

	}

	/** Internal usage for GetPackageName */
	private static string PackageLine = null;

	/** Run an external exe (and capture the output), given the exe path and the commandline. */
	private static string GetPackageInfo(string ApkName, bool bRetrieveVersionCode)
	{
		// we expect there to be one, so use the first one
		string AaptPath = GetAaptPath();

		var ExeInfo = new ProcessStartInfo(AaptPath, "dump badging \"" + ApkName + "\"");
		ExeInfo.UseShellExecute = false;
		ExeInfo.RedirectStandardOutput = true;
		using (var GameProcess = Process.Start(ExeInfo))
		{
			PackageLine = null;
			GameProcess.BeginOutputReadLine();
			GameProcess.OutputDataReceived += ParsePackageName;
			GameProcess.WaitForExit();
		}
		string ReturnValue = null;
		if (PackageLine != null)
		{
			// the line should look like: package: name='com.epicgames.qagame' versionCode='1' versionName='1.0'
			string[] Tokens = PackageLine.Split("'".ToCharArray());
			int TokenIndex = bRetrieveVersionCode ? 3 : 1;
			if (Tokens.Length >= TokenIndex + 1)
			{
				ReturnValue = Tokens[TokenIndex];
			}
		}
		return ReturnValue;
	}

	/** Simple function to pipe output asynchronously */
	private static void ParsePackageName(object Sender, DataReceivedEventArgs Event)
	{
		// DataReceivedEventHandler is fired with a null string when the output stream is closed.  We don't want to
		// print anything for that event.
		if (!String.IsNullOrEmpty(Event.Data))
		{
			if (PackageLine == null)
			{
				string Line = Event.Data;
				if (Line.StartsWith("package:"))
				{
					PackageLine = Line;
				}
			}
		}
	}

	private static string GetAaptPath()
	{
		// there is a numbered directory in here, hunt it down
		string[] Subdirs = Directory.GetDirectories(Environment.ExpandEnvironmentVariables("%ANDROID_HOME%/build-tools/"));
		if (Subdirs.Length == 0)
		{
			throw new AutomationException("Failed to find %ANDROID_HOME%/build-tools subdirectory");
		}
		// we expect there to be one, so use the first one
		return Path.Combine(Subdirs[0], "aapt.exe");
	}

	public override ProcessResult RunClient(ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
		string ApkName = ClientApp + GetArchitecture(Params) + ".apk";
		if (!File.Exists(ApkName))
		{
			ApkName = GetFinalApkName(Params, Path.GetFileNameWithoutExtension(ClientApp), false);
		}

		Console.WriteLine("Apk='{0}', CLientApp='{1}', ExeName='{2}'", ApkName, ClientApp, Params.ProjectGameExeFilename);

		// run aapt to get the name of the intent
		string PackageName = GetPackageInfo(ApkName, false);
		if (PackageName == null)
		{
			throw new AutomationException("Failed to get package name from " + ClientApp);
		}

		string AdbCommand = GetAdbCommand(Params);
		string CommandLine = "shell am start -n " + PackageName + "/com.epicgames.ue4.GameActivity";

		// start the app on device!
		ProcessResult ClientProcess = Run(CmdEnv.CmdExe, AdbCommand + CommandLine, null, ClientRunFlags);
		 
		return ClientProcess;
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
// 		if (SC.StageExecutables.Count != 1 && Params.Package)
// 		{
// 			throw new AutomationException("Exactly one executable expected when staging Android. Had " + SC.StageExecutables.Count.ToString());
// 		}
// 
// 		// stage all built executables
// 		foreach (var Exe in SC.StageExecutables)
// 		{
// 			string ApkName = Exe + GetArchitecture(Params) + ".apk";
// 
// 			SC.StageFiles(StagedFileType.NonUFS, Params.ProjectBinariesFolder, ApkName);
// 		}
	}

	/// <summary>
	/// Gets cook platform name for this platform.
	/// </summary>
	/// <param name="CookFlavor">Additional parameter used to indicate special sub-target platform.</param>
	/// <returns>Cook platform string.</returns>
	public override string GetCookPlatform( bool bDedicatedServer, bool bIsClientOnly, string CookFlavor ) 
	{
		if( CookFlavor.Length > 0 )
		{
			return "Android_" + CookFlavor;
		}
		else 
		{
			return "Android";
		}
	}

	public override bool DeployPakInternalLowerCaseFilenames()
	{
		return false;
	}

	public override bool DeployLowerCaseFilenames(bool bUFSFile)
	{
		return false;
	}

	public override string LocalPathToTargetPath(string LocalPath, string LocalRoot)
	{
		return LocalPath.Replace("\\", "/").Replace(LocalRoot, "../../..");
	}

	public override bool IsSupported { get { return true; } }

	public override string Remap(string Dest)
	{
		return Dest;
	}

	public override bool RequiresPak(ProjectParams Params)
	{
		return Params.Package;
	}

	#region Hooks

	public override void PostBuildTarget(UE4Build Build, string ProjectName, string UProjectPath, string Config)
	{
		// Run UBT w/ the prep for deployment only option
		// This is required as UBT will 'fake' success when building via UAT and run
		// the deployment prep step before all the required parts are present.
		if (ProjectName.Length > 0)
		{
			string ProjectToBuild = ProjectName;
			if (ProjectToBuild != "UE4Game" && !string.IsNullOrEmpty(UProjectPath))
			{
				ProjectToBuild = UProjectPath;
			}
			string UBTCommand = string.Format("\"{0}\" Android {1} -prepfordeploy", ProjectToBuild, Config);
			CommandUtils.RunUBT(UE4Build.CmdEnv, Build.UBTExecutable, UBTCommand);
		}
	}
	#endregion

    public override string GUBP_GetPlatformFailureEMails(string Branch)
    {
        return "Josh.Adams[epic] JJ.Hoesing[epic]";
    }
}
