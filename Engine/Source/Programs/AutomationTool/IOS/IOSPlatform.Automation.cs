// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using Ionic.Zip;
using Ionic.Zlib;

static class IOSEnvVarNames
{
	// Should we code sign when staging?  (defaults to 1 if not present)
	static public readonly string CodeSignWhenStaging = "uebp_CodeSignWhenStaging";
}

public class IOSPlatform : Platform
{
	public IOSPlatform()
		:base(UnrealTargetPlatform.IOS)
	{
	}

	protected string MakeIPAFileName( UnrealTargetConfiguration TargetConfiguration, string ProjectGameExeFilename )
	{
		var ProjectIPA = Path.ChangeExtension(ProjectGameExeFilename, null);
		if (TargetConfiguration != UnrealTargetConfiguration.Development)
		{
			ProjectIPA += "-" + PlatformType.ToString() + "-" + TargetConfiguration.ToString();
		}
		ProjectIPA += ".ipa";
		return ProjectIPA;
	}

	// Determine if we should code sign
	protected bool GetCodeSignDesirability(ProjectParams Params)
	{
		//@TODO: Would like to make this true, as it's the common case for everyone else
		bool bDefaultNeedsSign = false;

		bool bNeedsSign = false;
		string EnvVar = InternalUtils.GetEnvironmentVariable(IOSEnvVarNames.CodeSignWhenStaging, bDefaultNeedsSign ? "1" : "0", /*bQuiet=*/ false);
		if (!bool.TryParse(EnvVar, out bNeedsSign))
		{
			int BoolAsInt;
			if (int.TryParse(EnvVar, out BoolAsInt))
			{
				bNeedsSign = BoolAsInt != 0;
			}
			else
			{
				bNeedsSign = bDefaultNeedsSign;
			}
		}

		if (!String.IsNullOrEmpty(Params.BundleName))
		{
			// Have to sign when a bundle name is specified
			bNeedsSign = true;
		}

		return bNeedsSign;
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		Log("Package {0}", Params.RawProjectPath);

		//@TODO: We should be able to use this code on both platforms, when the following issues are sorted:
		//   - Raw executable is unsigned & unstripped (need to investigate adding stripping to IPP)
		//   - IPP needs to be able to codesign a raw directory
		//   - IPP needs to be able to take a .app directory instead of a Payload directory when doing RepackageFromStage (which would probably be renamed)
		//   - Some discrepancy in the loading screen pngs that are getting packaged, which needs to be investigated
		//   - Code here probably needs to be updated to write 0 byte files as 1 byte (difference with IPP, was required at one point when using Ionic.Zip to prevent issues on device, maybe not needed anymore?)
		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
		{
			// copy in all of the artwork and plist
			UEBuildDeploy DeployHandler = UEBuildDeploy.GetBuildDeploy(UnrealTargetPlatform.IOS);
			DeployHandler.PrepForUATPackageOrDeploy(Params.ShortProjectName,
				Path.GetDirectoryName(Params.RawProjectPath),
				CombinePaths(Path.GetDirectoryName(Params.ProjectGameExeFilename), SC.StageExecutables[0]),
				CombinePaths(SC.LocalRoot, "Engine"),
				Params.Distribution);

			// figure out where to pop in the staged files
			string AppDirectory = string.Format("{0}/Payload/{1}.app",
				Path.GetDirectoryName(Params.ProjectGameExeFilename),
				Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename));

			// delete the old cookeddata
			InternalUtils.SafeDeleteDirectory(AppDirectory + "/cookeddata", true);
			InternalUtils.SafeDeleteFile(AppDirectory + "/ue4commandline.txt", true);

			// copy the Staged files to the AppDirectory
			string[] StagedFiles = Directory.GetFiles(SC.StageDirectory, "*", SearchOption.AllDirectories);
			foreach (string Filename in StagedFiles)
			{
				string DestFilename = Filename.Replace(SC.StageDirectory, AppDirectory);
				Directory.CreateDirectory(Path.GetDirectoryName(DestFilename));
				InternalUtils.SafeCopyFile(Filename, DestFilename, true);
			}
		}
		

		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac)
		{
			if (SC.StageTargetConfigurations.Count != 1) 
			{
				throw new AutomationException ("iOS is currently only able to package one target configuration at a time, but StageTargetConfigurations contained {0} configurations", SC.StageTargetConfigurations.Count);
			}
			var TargetConfiguration = SC.StageTargetConfigurations[0];

			var ProjectStub = Params.ProjectGameExeFilename;
			var ProjectIPA = MakeIPAFileName(TargetConfiguration, Params.ProjectGameExeFilename);

			// package a .ipa from the now staged directory
			var IPPExe = CombinePaths(CmdEnv.LocalRoot, "Engine/Binaries/DotNET/IOS/IPhonePackager.exe");

			Log("ProjectName={0}", Params.ShortProjectName);
			Log("ProjectStub={0}", ProjectStub);
			Log("ProjectIPA={0}", ProjectIPA);
			Log("IPPExe={0}", IPPExe);

			bool cookonthefly = Params.CookOnTheFly || Params.SkipCookOnTheFly;

			string IPPArguments = "RepackageFromStage " + (Params.IsCodeBasedProject ? Params.RawProjectPath : "Engine");
			IPPArguments += " -config " + TargetConfiguration.ToString();

			// Determine if we should sign
			bool bNeedToSign = GetCodeSignDesirability(Params);

			if (!String.IsNullOrEmpty(Params.BundleName))
			{
				// Have to sign when a bundle name is specified
				bNeedToSign = true;
				IPPArguments += " -bundlename " + Params.BundleName;
			}

			if (bNeedToSign)
			{
				IPPArguments += " -sign";
			}

			IPPArguments += (cookonthefly ? " -cookonthefly" : "");
			IPPArguments += " -stagedir " + CombinePaths(Params.BaseStageDirectory, "IOS");

            // save a copy of the IPA
            var CopyName = ProjectIPA.Replace("UE4Game", "UE4Game-Backup");
            if (!Params.IsCodeBasedProject)
            {
                if (FileExists_NoExceptions(ProjectIPA))
                {
                    DeleteFile_NoExceptions(CopyName);
                    CopyFile(ProjectIPA, CopyName);
                }
                else
                {
                    CopyName = "";
                }
            }
            // delete the .ipa to make sure it was made
            DeleteFile(ProjectIPA);

			RunAndLog(CmdEnv, IPPExe, IPPArguments);

			// verify the .ipa exists
			if (!FileExists(ProjectIPA))
			{
				throw new AutomationException("PACKAGE FAILED - {0} was not created", ProjectIPA);
			}

			// rename the .ipa!
			if (!Params.IsCodeBasedProject)
			{
				var NewIPAName = ProjectIPA.Replace("UE4Game", Params.ShortProjectName);
				DeleteFile(NewIPAName);
				RenameFile(ProjectIPA, NewIPAName);
                if (CopyName != "")
                {
                    RenameFile(CopyName, ProjectIPA);
                }
			}

			if (WorkingCL > 0)
			{
				// Open files for add or edit
				var ExtraFilesToCheckin = new List<string>
				{
					ProjectIPA
				};

				// check in the .ipa along with everything else
				UE4Build.AddBuildProductsToChangelist(WorkingCL, ExtraFilesToCheckin);
			}

			//@TODO: This automatically deploys after packaging, useful for testing on PC when iterating on IPP
			//Deploy(Params, SC);
		}
		else
		{
			// code sign the app
			CodeSign(Path.GetDirectoryName(Params.ProjectGameExeFilename), Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename), Params.RawProjectPath, SC.StageTargetConfigurations[0], Params.Distribution);

			// now generate the ipa
			PackageIPA(Path.GetDirectoryName(Params.ProjectGameExeFilename), Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename), Params.ShortProjectName, Path.GetDirectoryName(Params.RawProjectPath), SC.StageTargetConfigurations[0], Params.Distribution);
		}

		PrintRunTime();
	}

	private string EnsureXcodeProjectExists(string RawProjectPath, out bool bWasGenerated)
	{
		// first check for ue4.xcodeproj
		bWasGenerated = false;
		string XcodeProj = RawProjectPath.Replace(".uproject", ".xcodeproj");
		Console.WriteLine ("Project: " + XcodeProj);
		if (!Directory.Exists (XcodeProj))
		{
			// project.xcodeproj doesn't exist, so check for ue4.xcodeproj
			XcodeProj = CombinePaths(CmdEnv.LocalRoot, "UE4.xcodeproj");
			if (!Directory.Exists (XcodeProj))
			{
				// ue4.xcodeproj doesn't exist, so generate temp project
				string Arguments = "-project=\"" + RawProjectPath + "\"";
				Arguments += " -game -nointellisense";
				string Script = CombinePaths(CmdEnv.LocalRoot, "Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh");
				if (GlobalCommandLine.Rocket)
				{
					Script = CombinePaths(CmdEnv.LocalRoot, "Engine/Build/BatchFiles/Mac/RocketGenerateProjectFiles.sh");
				}
				string CWD = Directory.GetCurrentDirectory ();
				Directory.SetCurrentDirectory (Path.GetDirectoryName (Script));
				Run (Script, Arguments, null, ERunOptions.Default);
				bWasGenerated = true;
				Directory.SetCurrentDirectory (CWD);

				if (!Directory.Exists (XcodeProj))
				{
					// something very bad happened
					throw new AutomationException("iOS couldn't find the appropriate Xcode Project");
				}
			}
		}

		return XcodeProj;
	}

	private void CodeSign(string BaseDirectory, string GameName, string RawProjectPath, UnrealTargetConfiguration TargetConfig, bool Distribution = false)
	{
		// check for the proper xcodeproject
		bool bWasGenerated = false;
		string XcodeProj = EnsureXcodeProjectExists (RawProjectPath, out bWasGenerated);
		string Arguments = "UBT_NO_POST_DEPLOY=true";
		Arguments += " /usr/bin/xcrun xcodebuild build -project \"" + XcodeProj + "\"";
		Arguments += " -scheme '";
		Arguments += GameName;
		Arguments += " - iOS (Run)'";
		Arguments += " -configuration " + TargetConfig.ToString();
		Arguments += " CODE_SIGN_IDENTITY=" + (Distribution ? "\"iPhone Distribution\"" : "\"iPhone Developer\"");
		Run ("/usr/bin/env", Arguments, null, ERunOptions.Default);
		if (bWasGenerated)
		{
			InternalUtils.SafeDeleteDirectory( XcodeProj, true);
		}
	}

	private void PackageIPA(string BaseDirectory, string GameName, string ProjectName, string ProjectDirectory, UnrealTargetConfiguration TargetConfig, bool Distribution = false)
	{
		// create the ipa
		string IPAName = CombinePaths(ProjectDirectory, "Binaries", "IOS", ProjectName + (TargetConfig != UnrealTargetConfiguration.Development ? ("-IOS-" + TargetConfig.ToString()) : "") + ".ipa");
		// delete the old one
		if (File.Exists(IPAName))
		{
			File.Delete(IPAName);
		}

		// make the subdirectory if needed
		string DestSubdir = Path.GetDirectoryName(IPAName);
		if (!Directory.Exists(DestSubdir))
		{
			Directory.CreateDirectory(DestSubdir);
		}

		// set up the directories
		string ZipWorkingDir = String.Format("Payload/{0}.app/", GameName);
		string ZipSourceDir = string.Format("{0}/Payload/{1}.app", BaseDirectory, GameName);

		// create the file
		using (ZipFile Zip = new ZipFile())
		{
			// set the compression level
			if (Distribution)
			{
				Zip.CompressionLevel = CompressionLevel.BestCompression;
			}

			// add the entire directory
			Zip.AddDirectory(ZipSourceDir, ZipWorkingDir);

			// Update permissions to be UNIX-style
			// Modify the file attributes of any added file to unix format
			foreach (ZipEntry E in Zip.Entries)
			{
				const byte FileAttributePlatform_NTFS = 0x0A;
				const byte FileAttributePlatform_UNIX = 0x03;
				const byte FileAttributePlatform_FAT = 0x00;

				const int UNIX_FILETYPE_NORMAL_FILE = 0x8000;
				//const int UNIX_FILETYPE_SOCKET = 0xC000;
				//const int UNIX_FILETYPE_SYMLINK = 0xA000;
				//const int UNIX_FILETYPE_BLOCKSPECIAL = 0x6000;
				const int UNIX_FILETYPE_DIRECTORY = 0x4000;
				//const int UNIX_FILETYPE_CHARSPECIAL = 0x2000;
				//const int UNIX_FILETYPE_FIFO = 0x1000;

				const int UNIX_EXEC = 1;
				const int UNIX_WRITE = 2;
				const int UNIX_READ = 4;


				int MyPermissions = UNIX_READ | UNIX_WRITE;
				int OtherPermissions = UNIX_READ;

				int PlatformEncodedBy = (E.VersionMadeBy >> 8) & 0xFF;
				int LowerBits = 0;

				// Try to preserve read-only if it was set
				bool bIsDirectory = E.IsDirectory;

				// Check to see if this 
				bool bIsExecutable = false;
				if (Path.GetFileNameWithoutExtension(E.FileName).Equals(GameName, StringComparison.InvariantCultureIgnoreCase))
				{
					bIsExecutable = true;
				}

				if (bIsExecutable)
				{
					// The executable will be encrypted in the final distribution IPA and will compress very poorly, so keeping it
					// uncompressed gives a better indicator of IPA size for our distro builds
					E.CompressionLevel = CompressionLevel.None;
				}

				if ((PlatformEncodedBy == FileAttributePlatform_NTFS) || (PlatformEncodedBy == FileAttributePlatform_FAT))
				{
					FileAttributes OldAttributes = E.Attributes;
					//LowerBits = ((int)E.Attributes) & 0xFFFF;

					if ((OldAttributes & FileAttributes.Directory) != 0)
					{
						bIsDirectory = true;
					}

					// Permissions
					if ((OldAttributes & FileAttributes.ReadOnly) != 0)
					{
						MyPermissions &= ~UNIX_WRITE;
						OtherPermissions &= ~UNIX_WRITE;
					}
				}

				if (bIsDirectory || bIsExecutable)
				{
					MyPermissions |= UNIX_EXEC;
					OtherPermissions |= UNIX_EXEC;
				}

				// Re-jigger the external file attributes to UNIX style if they're not already that way
				if (PlatformEncodedBy != FileAttributePlatform_UNIX)
				{
					int NewAttributes = bIsDirectory ? UNIX_FILETYPE_DIRECTORY : UNIX_FILETYPE_NORMAL_FILE;

					NewAttributes |= (MyPermissions << 6);
					NewAttributes |= (OtherPermissions << 3);
					NewAttributes |= (OtherPermissions << 0);

					// Now modify the properties
					E.AdjustExternalFileAttributes(FileAttributePlatform_UNIX, (NewAttributes << 16) | LowerBits);
				}
			}

			// Save it out
			Zip.Save(IPAName);
		}
	}

	private static void CopyFileWithReplacements(string SourceFilename, string DestFilename, Dictionary<string, string> Replacements)
	{
		if (!File.Exists(SourceFilename))
		{
			return;
		}

		// make the dst filename with the same structure as it was in SourceDir
		if (File.Exists(DestFilename))
		{
			File.Delete(DestFilename);
		}

		// make the subdirectory if needed
		string DestSubdir = Path.GetDirectoryName(DestFilename);
		if (!Directory.Exists(DestSubdir))
		{
			Directory.CreateDirectory(DestSubdir);
		}

		// some files are handled specially
		string Ext = Path.GetExtension(SourceFilename);
		if (Ext == ".plist")
		{
			string Contents = File.ReadAllText(SourceFilename);

			// replace some varaibles
			foreach (var Pair in Replacements)
			{
				Contents = Contents.Replace(Pair.Key, Pair.Value);
			}

			// write out file
			File.WriteAllText(DestFilename, Contents);
		}
		else
		{
			File.Copy(SourceFilename, DestFilename);

			// remove any read only flags
			FileInfo DestFileInfo = new FileInfo(DestFilename);
			DestFileInfo.Attributes = DestFileInfo.Attributes & ~FileAttributes.ReadOnly;
		}
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac)
		{
			// copy the icons/launch screens from the engine
			{
				string SourcePath = CombinePaths(SC.LocalRoot, "Engine", "Build", "IOS", "Resources", "Graphics");
				SC.StageFiles(StagedFileType.NonUFS, SourcePath, "*.png", false, null, "", true, false);
			}

			// copy the icons/launch screens from the game (may stomp the engine copies)
			{
				string SourcePath = CombinePaths(SC.ProjectRoot, "Build", "IOS", "Resources", "Graphics");
				SC.StageFiles(StagedFileType.NonUFS, SourcePath, "*.png", false, null, "", true, false);
			}

			// copy the plist (only if code signing, as it's protected by the code sign blob in the executable and can't be modified independently)
			if (GetCodeSignDesirability(Params))
			{
				string SourcePListFile = CombinePaths(SC.LocalRoot, "Engine", "Build", "IOS", "UE4Game-Info.plist");
				if (File.Exists(SC.ProjectRoot + "/Build/IOS/" + SC.ShortProjectName + "-Info.plist"))
				{
					SourcePListFile = CombinePaths(SC.ProjectRoot, "Build", "IOS", SC.ShortProjectName + "-Info.plist");
				}

				//@TODO: This is writing to the engine directory!
				string SourcePath = CombinePaths((SC.IsCodeBasedProject ? SC.ProjectRoot : SC.LocalRoot + "\\Engine"), "Intermediate", "IOS");
				string TargetPListFile = Path.Combine(SourcePath, (SC.IsCodeBasedProject ? SC.ShortProjectName : "UE4Game") + "-Info.plist");

				Dictionary<string, string> Replacements = new Dictionary<string, string>();
				Replacements.Add("${EXECUTABLE_NAME}", (SC.IsCodeBasedProject ? SC.ShortProjectName : "UE4Game"));
				Replacements.Add("${BUNDLE_IDENTIFIER}", SC.ShortProjectName.Replace("_", ""));
				CopyFileWithReplacements(SourcePListFile, TargetPListFile, Replacements);

				SC.StageFiles(StagedFileType.NonUFS, SourcePath, Path.GetFileName(TargetPListFile), false, null, "", false, false, "Info.plist");
			}

			// Now do the .mobileprovision
			//@TODO: Remove this mobileprovision copy, and move to a library approach like Xcode/codesign does
			if (GetCodeSignDesirability(Params))
			{
				string SourceProvision = CombinePaths(SC.LocalRoot, "Engine", "Build", "IOS", "UE4Game.mobileprovision");
				string GameSourceProvision = SC.ProjectRoot + "/Build/IOS/" + SC.ShortProjectName + ".mobileprovision";
				if (File.Exists(GameSourceProvision))
				{
					SourceProvision = GameSourceProvision;
				}

				SC.StageFiles(StagedFileType.NonUFS, Path.GetDirectoryName(SourceProvision), Path.GetFileName(SourceProvision), false, null, "", false, false, "embedded.mobileprovision");
			}
		}
	}

	public override void GetFilesToArchive(ProjectParams Params, DeploymentContext SC)
	{
		if (SC.StageTargetConfigurations.Count != 1)
		{
			throw new AutomationException("iOS is currently only able to package one target configuration at a time, but StageTargetConfigurations contained {0} configurations", SC.StageTargetConfigurations.Count);
		}
		var TargetConfiguration = SC.StageTargetConfigurations[0];

		string ProjectGameExeFilename = Params.ProjectGameExeFilename;
		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () == UnrealTargetPlatform.Mac)
		{
			ProjectGameExeFilename = CombinePaths (Path.GetDirectoryName(Params.RawProjectPath), "Binaries", "IOS", Path.GetFileName (Params.ProjectGameExeFilename));
		}
		var ProjectIPA = MakeIPAFileName( TargetConfiguration, ProjectGameExeFilename );
		if (!Params.IsCodeBasedProject)
		{
			ProjectIPA = ProjectIPA.Replace("UE4Game", Params.ShortProjectName);
		}

		// verify the .ipa exists
		if (!FileExists(ProjectIPA))
		{
			throw new AutomationException("ARCHIVE FAILED - {0} was not found", ProjectIPA);
		}

		SC.ArchiveFiles(Path.GetDirectoryName(ProjectIPA), Path.GetFileName(ProjectIPA));
	}

	public override void Deploy(ProjectParams Params, DeploymentContext SC)
    {
		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac)
		{
			if (SC.StageTargetConfigurations.Count != 1)
			{
				throw new AutomationException ("iOS is currently only able to package one target configuration at a time, but StageTargetConfigurations contained {0} configurations", SC.StageTargetConfigurations.Count);
			}
			var TargetConfiguration = SC.StageTargetConfigurations[0];

//			var ProjectStub = Params.ProjectGameExeFilename;	
			var ProjectIPA = MakeIPAFileName(TargetConfiguration, Params.ProjectGameExeFilename);
			if (!Params.IsCodeBasedProject)
			{
				ProjectIPA = ProjectIPA.Replace("UE4Game", Params.ShortProjectName);
			}

			var StagedIPA = SC.StageDirectory + "\\" + Path.GetFileName(ProjectIPA);
			// verify the .ipa exists
			if (!FileExists(StagedIPA))
			{
				StagedIPA = ProjectIPA;
				if(!FileExists(StagedIPA))
				{
					throw new AutomationException("DEPLOY FAILED - {0} was not found", ProjectIPA);
				}
			}

			// deploy the .ipa
			var IPPExe = CombinePaths(CmdEnv.LocalRoot, "Engine/Binaries/DotNET/IOS/IPhonePackager.exe");

			// check for it in the stage directory
			RunAndLog(CmdEnv, IPPExe, "Deploy " + Path.GetFullPath(StagedIPA));
		}
        PrintRunTime();
    }

	public override string GetCookPlatform(bool bDedicatedServer, bool bIsClientOnly, string CookFlavor)
	{
		return "IOS";
	}

	public override bool DeployPakInternalLowerCaseFilenames()
	{
		return false;
	}

	public override bool DeployLowerCaseFilenames(bool bUFSFile)
	{
		// we shouldn't modify the case on files like Info.plist or the icons
		return bUFSFile;
	}

	public override string LocalPathToTargetPath(string LocalPath, string LocalRoot)
	{
		return LocalPath.Replace("\\", "/").Replace(LocalRoot, "../../..");
	}

	public override bool IsSupported { get { return true; } }

	public override bool LaunchViaUFE { get { return UnrealBuildTool.ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac; } }

	public override string Remap(string Dest)
	{
		return "cookeddata/" + Dest;
	}
    public override string GUBP_GetPlatformFailureEMails(string Branch)
    {
        return "Peter.Sauerbrei[epic] Michael.Trepka[epic]";
    }

	public override ProcessResult RunClient(ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () == UnrealTargetPlatform.Mac)
		{
			string AppDirectory = string.Format("{0}/Payload/{1}.app",
				Path.GetDirectoryName(Params.ProjectGameExeFilename), 
				Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename));
			string GameName = Path.GetFileNameWithoutExtension (ClientApp);
			string GameApp = AppDirectory + "/" + GameName;
			bool bWasGenerated = false;
			string XcodeProj = EnsureXcodeProjectExists (Params.RawProjectPath, out bWasGenerated);
			string Arguments = "UBT_NO_POST_DEPLOY=true /usr/bin/xcrun xcodebuild test -project \"" + XcodeProj + "\"";
			Arguments += " -scheme '";
			Arguments += GameName;
			Arguments += " - iOS (Run)'";
			Arguments += " -configuration " + Params.ClientConfigsToBuild [0].ToString();
			Arguments += " -destination 'platform=iOS,id=" + Params.Device.Substring(4) + "'";
			Arguments += " TEST_HOST=\"";
			Arguments += GameApp;
			Arguments += "\" BUNDLE_LOADER=\"";
			Arguments += GameApp + "\"";
			ProcessResult ClientProcess = Run ("/usr/bin/env", Arguments, null, ClientRunFlags | ERunOptions.NoWaitForExit);
			return ClientProcess;
		}
		else
		{
			return base.RunClient(ClientRunFlags, ClientApp, ClientCmdLine, Params);
		}
	}

	#region Hooks

	public override void PreBuildAgenda(UE4Build Build, UE4Build.BuildAgenda Agenda)
	{
		if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac)
		{
			Agenda.DotNetProjects.Add (@"Engine\Source\Programs\IOS\IPhonePackager\IPhonePackager.csproj");
		}
	}

	#endregion
}
