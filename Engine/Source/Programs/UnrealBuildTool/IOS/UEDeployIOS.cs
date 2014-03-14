// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.IO;

namespace UnrealBuildTool.IOS
{
	class UEDeployIOS : UEBuildDeploy
	{
		/**
		 *	Register the platform with the UEBuildDeploy class
		 */
		public override void RegisterBuildDeploy()
		{
			// TODO: print debug info and handle any cases that would keep this from registering
			UEBuildDeploy.RegisterBuildDeploy(UnrealTargetPlatform.IOS, this);
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

		public override bool PrepForUATPackageOrDeploy(string InProjectName, string InProjectDirectory, string InExecutablePath, string InEngineDir, bool bForDistribution)
		{
			bool bIsUE4Game = InExecutablePath.Contains ("UE4Game");
			string BinaryPath = Path.GetDirectoryName (InExecutablePath);
			string GameExeName = Path.GetFileName (InExecutablePath);
			string GameName = bIsUE4Game ? "UE4Game" : InProjectName;
			string PayloadDirectory =  BinaryPath + "/Payload";
			string AppDirectory = PayloadDirectory + "/" + GameName + ".app";
			string CookedContentDirectory = AppDirectory + "/cookeddata";
			string BuildDirectory = InProjectDirectory + "/Build/IOS";
			string IntermediateDirectory = (bIsUE4Game ? InEngineDir : InProjectDirectory) + "/Intermediate/IOS";

			// don't delete the payload directory, just update the data in it
			//if (Directory.Exists(PayloadDirectory))
			//{
			//	Directory.Delete(PayloadDirectory, true);
			//}

			Directory.CreateDirectory(BinaryPath);
			Directory.CreateDirectory(PayloadDirectory);
			Directory.CreateDirectory(AppDirectory);
			Directory.CreateDirectory(BuildDirectory);
			Directory.CreateDirectory(CookedContentDirectory);

			// create the entitlements file
			WriteEntitlementsFile(Path.Combine(IntermediateDirectory, GameName + ".entitlements"));

			// delete some old files if they exist
			if (Directory.Exists(AppDirectory + "/_CodeSignature"))
			{
				Directory.Delete(AppDirectory + "/_CodeSignature", true);
			}
			if (File.Exists(AppDirectory + "/CustomResourceRules.plist"))
			{
				File.Delete(AppDirectory + "/CustomResourceRules.plist");
			}
			if (File.Exists(AppDirectory + "/embedded.mobileprovision"))
			{
				File.Delete(AppDirectory + "/embedded.mobileprovision");
			}
			if (File.Exists(AppDirectory + "/PkgInfo"))
			{
				File.Delete(AppDirectory + "/PkgInfo");
			}

			// install the provision
			string ProvisionWithPrefix = InEngineDir + "/Build/IOS/UE4Game.mobileprovision";
			if (File.Exists(BuildDirectory + "/" + InProjectName + ".mobileprovision"))
			{
				ProvisionWithPrefix = BuildDirectory + "/" + InProjectName + ".mobileprovision";
			}
			if (File.Exists (ProvisionWithPrefix))
			{
				Directory.CreateDirectory (Environment.GetEnvironmentVariable ("HOME") + "/Library/MobileDevice/Provisioning Profiles/");
				File.Copy (ProvisionWithPrefix, Environment.GetEnvironmentVariable ("HOME") + "/Library/MobileDevice/Provisioning Profiles/" + InProjectName + ".mobileprovision", true);
				FileInfo DestFileInfo = new FileInfo (Environment.GetEnvironmentVariable ("HOME") + "/Library/MobileDevice/Provisioning Profiles/" + InProjectName + ".mobileprovision");
				DestFileInfo.Attributes = DestFileInfo.Attributes & ~FileAttributes.ReadOnly;
			}

			// install the distribution provision
			ProvisionWithPrefix = InEngineDir + "/Build/IOS/UE4Game_Distro.mobileprovision";
			if (File.Exists(BuildDirectory + "/" + InProjectName + "_Distro.mobileprovision"))
			{
				ProvisionWithPrefix = BuildDirectory + "/" + InProjectName + "_Distro.mobileprovision";
			}
			if (File.Exists (ProvisionWithPrefix))
			{
				File.Copy (ProvisionWithPrefix, Environment.GetEnvironmentVariable ("HOME") + "/Library/MobileDevice/Provisioning Profiles/" + InProjectName + "_Distro.mobileprovision", true);
				FileInfo DestFileInfo = new FileInfo (Environment.GetEnvironmentVariable ("HOME") + "/Library/MobileDevice/Provisioning Profiles/" + InProjectName + "_Distro.mobileprovision");
				DestFileInfo.Attributes = DestFileInfo.Attributes & ~FileAttributes.ReadOnly;
			}

			// copy plist file
			string PListFile = InEngineDir + "/Build/IOS/UE4Game-Info.plist";
			if (File.Exists(BuildDirectory + "/" + InProjectName + "-Info.plist"))
			{
				PListFile = BuildDirectory + "/" + InProjectName + "-Info.plist";
			}

			Dictionary<string, string> Replacements = new Dictionary<string, string>();
			Replacements.Add("${EXECUTABLE_NAME}", GameName);
			Replacements.Add("${BUNDLE_IDENTIFIER}", InProjectName.Replace("_", ""));
			CopyFileWithReplacements(PListFile, AppDirectory + "/Info.plist", Replacements);
			CopyFileWithReplacements(PListFile, IntermediateDirectory + "/" + GameName + "-Info.plist", Replacements);

			// copy engine assets in
			CopyFiles(InEngineDir + "/Build/IOS/Resources/Graphics", AppDirectory, "*.png", true);
			// merge game assets on top
			if (Directory.Exists(BuildDirectory + "/Resources/Graphics"))
			{
				CopyFiles(BuildDirectory + "/Resources/Graphics", AppDirectory, "*.png", true);
			}

			//CopyFiles(BuildDirectory, PayloadDirectory, null, "iTunesArtwork", null);
			CopyFolder(InEngineDir + "/Content/Stats", AppDirectory + "/cookeddata/engine/content/stats", true);

			// ensure the destination is writable
			FileInfo GameFileInfo = new FileInfo (AppDirectory + "/" + GameName);
			GameFileInfo.Attributes = GameFileInfo.Attributes & ~FileAttributes.ReadOnly;

			// copy the GameName binary
			File.Copy(BinaryPath + "/" + GameExeName, AppDirectory + "/" + GameName, true);

			return true;
		}

		public override bool PrepTargetForDeployment(UEBuildTarget InTarget)
		{
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac && Environment.GetEnvironmentVariable("UBT_NO_POST_DEPLOY") != "true")
			{
				string GameName = InTarget.AppName;

				string DecoratedGameName;
				if (InTarget.Configuration == UnrealTargetConfiguration.Development)
				{
					DecoratedGameName = GameName;
				}
				else
				{
					DecoratedGameName = String.Format("{0}-{1}-{2}", GameName, InTarget.Platform.ToString(), InTarget.Configuration.ToString());
				}

				string BuildPath = InTarget.ProjectDirectory + "/Binaries/IOS";
				string ProjectDirectory = InTarget.ProjectDirectory;

				return PrepForUATPackageOrDeploy(GameName, ProjectDirectory, BuildPath + "/" + DecoratedGameName, "../../Engine", false);
			}

			return true;
		}

		private void WriteEntitlementsFile(string OutputFilename)
		{
			Directory.CreateDirectory(Path.GetDirectoryName(OutputFilename));
			// we need to have something so Xcode will compile, so we just set the get-task-allow, since we know the value, 
			// which is based on distribution or not (true means debuggable)
			File.WriteAllText(OutputFilename, string.Format("<plist><dict><key>get-task-allow</key><{0}/></dict></plist>",
				/*Config.bForDistribution ? "false" : */"true"));
		}

		static void SafeFileCopy(FileInfo SourceFile, string DestinationPath, bool bOverwrite)
		{
			FileInfo DI = new FileInfo(DestinationPath);
			if (DI.Exists && bOverwrite)
			{
				DI.IsReadOnly = false;
				DI.Delete();
			}

			SourceFile.CopyTo(DestinationPath, bOverwrite);
		}

		private void CopyFiles(string SourceDirectory, string DestinationDirectory, string TargetFiles, bool bOverwrite = false)
		{
			DirectoryInfo SourceFolderInfo = new DirectoryInfo(SourceDirectory);
			FileInfo[] SourceFiles = SourceFolderInfo.GetFiles(TargetFiles);
			foreach (FileInfo SourceFile in SourceFiles)
			{
				string DestinationPath = Path.Combine(DestinationDirectory, SourceFile.Name);
				SafeFileCopy(SourceFile, DestinationPath, bOverwrite);
			}
		}

		private void CopyFolder(string SourceDirectory, string DestinationDirectory, bool bOverwrite = false)
		{
			Directory.CreateDirectory(DestinationDirectory);
			RecursiveFolderCopy(new DirectoryInfo(SourceDirectory), new DirectoryInfo(DestinationDirectory), bOverwrite);
		}

		static private void RecursiveFolderCopy(DirectoryInfo SourceFolderInfo, DirectoryInfo DestFolderInfo, bool bOverwrite = false)
		{
			foreach (FileInfo SourceFileInfo in SourceFolderInfo.GetFiles())
			{
				string DestinationPath = Path.Combine(DestFolderInfo.FullName, SourceFileInfo.Name);
				SafeFileCopy(SourceFileInfo, DestinationPath, bOverwrite);
			}

			foreach (DirectoryInfo SourceSubFolderInfo in SourceFolderInfo.GetDirectories())
			{
				string DestFolderName = Path.Combine(DestFolderInfo.FullName, SourceSubFolderInfo.Name);
				Directory.CreateDirectory(DestFolderName);
				RecursiveFolderCopy(SourceSubFolderInfo, new DirectoryInfo(DestFolderName), bOverwrite);
			}
		}
	}
}
