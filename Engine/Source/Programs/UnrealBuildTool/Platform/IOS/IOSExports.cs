// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	/// <summary>
	/// Public IOS functions exposed to UAT
	/// </summary>
	public static class IOSExports
	{
		/// <summary>
		/// 
		/// </summary>
		/// <returns></returns>
		public static bool UseRPCUtil()
		{
			XmlConfig.ReadConfigFiles();
			return RemoteToolChain.bUseRPCUtil;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="InProject"></param>
		/// <param name="Distribution"></param>
		/// <param name="MobileProvision"></param>
		/// <param name="SigningCertificate"></param>
		/// <param name="TeamUUID"></param>
		/// <param name="bAutomaticSigning"></param>
		public static void GetProvisioningData(FileReference InProject, bool Distribution, out string MobileProvision, out string SigningCertificate, out string TeamUUID, out bool bAutomaticSigning)
		{
			IOSProjectSettings ProjectSettings = ((IOSPlatform)UEBuildPlatform.GetBuildPlatform(UnrealTargetPlatform.IOS)).ReadProjectSettings(InProject);
			if (ProjectSettings == null)
			{
				MobileProvision = null;
				SigningCertificate = null;
				TeamUUID = null;
				bAutomaticSigning = false;
				return;
			}
			if (ProjectSettings.bAutomaticSigning)
			{
				MobileProvision = null;
				SigningCertificate = null;
				TeamUUID = ProjectSettings.TeamID;
				bAutomaticSigning = true;
			}
			else
			{
				IOSProvisioningData Data = ((IOSPlatform)UEBuildPlatform.GetBuildPlatform(UnrealTargetPlatform.IOS)).ReadProvisioningData(ProjectSettings, Distribution);
				if (Data == null)
				{ // no provisioning, swith to automatic
					MobileProvision = null;
					SigningCertificate = null;
					TeamUUID = ProjectSettings.TeamID;
					bAutomaticSigning = true;
				}
				else
				{
					MobileProvision = Data.MobileProvision;
					SigningCertificate = Data.SigningCertificate;
					TeamUUID = Data.TeamUUID;
					bAutomaticSigning = false;
				}
			}
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="Config"></param>
		/// <param name="ProjectFile"></param>
		/// <param name="InProjectName"></param>
		/// <param name="InProjectDirectory"></param>
		/// <param name="InExecutablePath"></param>
		/// <param name="InEngineDir"></param>
		/// <param name="bForDistribution"></param>
		/// <param name="CookFlavor"></param>
		/// <param name="bIsDataDeploy"></param>
		/// <param name="bCreateStubIPA"></param>
		/// <returns></returns>
		public static bool PrepForUATPackageOrDeploy(UnrealTargetConfiguration Config, FileReference ProjectFile, string InProjectName, DirectoryReference InProjectDirectory, string InExecutablePath, DirectoryReference InEngineDir, bool bForDistribution, string CookFlavor, bool bIsDataDeploy, bool bCreateStubIPA)
		{
			return new UEDeployIOS().PrepForUATPackageOrDeploy(Config, ProjectFile, InProjectName, InProjectDirectory.FullName, InExecutablePath, InEngineDir.FullName, bForDistribution, CookFlavor, bIsDataDeploy, bCreateStubIPA);
		}

        /// <summary>
        /// 
        /// </summary>
        /// <param name="ProjectFile"></param>
        /// <param name="Config"></param>
        /// <param name="ProjectDirectory"></param>
        /// <param name="bIsUE4Game"></param>
        /// <param name="GameName"></param>
        /// <param name="ProjectName"></param>
        /// <param name="InEngineDir"></param>
        /// <param name="AppDirectory"></param>
        /// <param name="bSupportsPortrait"></param>
        /// <param name="bSupportsLandscape"></param>
        /// <param name="bSkipIcons"></param>
        /// <returns></returns>
        public static bool GeneratePList(FileReference ProjectFile, UnrealTargetConfiguration Config, DirectoryReference ProjectDirectory, bool bIsUE4Game, string GameName, string ProjectName, DirectoryReference InEngineDir, DirectoryReference AppDirectory, out bool bSupportsPortrait, out bool bSupportsLandscape, out bool bSkipIcons)
		{
			return new UEDeployIOS().GeneratePList(ProjectFile, Config, ProjectDirectory.FullName, bIsUE4Game, GameName, ProjectName, InEngineDir.FullName, AppDirectory.FullName, out bSupportsPortrait, out bSupportsLandscape, out bSkipIcons);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="PlatformType"></param>
		/// <param name="SourceFile"></param>
		/// <param name="TargetFile"></param>
		public static void StripSymbols(UnrealTargetPlatform PlatformType, FileReference SourceFile, FileReference TargetFile)
		{
			IOSProjectSettings ProjectSettings = ((IOSPlatform)UEBuildPlatform.GetBuildPlatform(PlatformType)).ReadProjectSettings(null);
			IOSToolChain ToolChain = new IOSToolChain(null, ProjectSettings);
			ToolChain.StripSymbols(SourceFile, TargetFile);
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="ProjectFile"></param>
		/// <param name="Executable"></param>
		/// <param name="StageDirectory"></param>
		/// <param name="PlatformType"></param>
		public static void GenerateAssetCatalog(FileReference ProjectFile, string Executable, string StageDirectory, UnrealTargetPlatform PlatformType)
		{
			// Initialize the toolchain.
			IOSProjectSettings ProjectSettings = ((IOSPlatform)UEBuildPlatform.GetBuildPlatform(PlatformType)).ReadProjectSettings(null);
			IOSToolChain ToolChain = new IOSToolChain(ProjectFile, ProjectSettings);

			// Determine whether the user has modified icons that require a remote Mac to build.
			CppPlatform Platform = PlatformType == UnrealTargetPlatform.IOS ? CppPlatform.IOS : CppPlatform.TVOS;
			bool bUserImagesExist = false;
			ToolChain.GenerateAssetCatalog(Platform, ref bUserImagesExist);

			// Don't attempt to do anything remotely if the user is using the default UE4 images.
			if (!bUserImagesExist)
			{
				return;
			}

            // Also don't attempt to use a remote Mac if packaging for TVOS on PC.
            if (Platform == CppPlatform.TVOS && BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Mac)
            {
                return;
            }

			// Save off the current bUseRPCUtil setting to restore at the end of this function.
			// At this time, iPhonePackager needs to be called with bUseRPCUtil == true.
			bool bSaveUseRPCUtil = RemoteToolChain.bUseRPCUtil;

			// Initialize the remote calling environment, taking into account the user's SSH setting.
			ToolChain.SetUpGlobalEnvironment(false);

			// Build the asset catalog ActionGraph.
			ActionGraph ActionGraph = new ActionGraph();
			List<FileItem> OutputFiles = new List<FileItem>();
			ToolChain.CompileAssetCatalog(FileItem.GetItemByPath(Executable), Platform, ActionGraph, OutputFiles);

			ActionGraph.FinalizeActionGraph();

			// I'm not sure how to derive the UE4Game and Development arguments programmatically.
			string[] Arguments = new string[] { "UE4Game", (PlatformType == UnrealTargetPlatform.IOS ? "IOS" : "TVOS"), "Development" };

			// Perform all of the setup necessary to actually execute the ActionGraph instance.
			ReadOnlyBuildVersion Version = new ReadOnlyBuildVersion(BuildVersion.ReadDefault());
			List<string[]> TargetSettings = new List<string[]>();
			TargetSettings.Add(Arguments);
			var Targets = new List<UEBuildTarget>();
			Dictionary<UEBuildTarget, CPPHeaders> TargetToHeaders = new Dictionary<UEBuildTarget, CPPHeaders>();
			List<TargetDescriptor> TargetDescs = new List<TargetDescriptor>();
			foreach (string[] TargetSetting in TargetSettings)
			{
				TargetDescs.AddRange(UEBuildTarget.ParseTargetCommandLine(TargetSetting, ref ProjectFile));
			}
			foreach (TargetDescriptor TargetDesc in TargetDescs)
			{
				UEBuildTarget Target = UEBuildTarget.CreateTarget(TargetDesc, Arguments, false, Version);
				if (Target == null)
				{
					continue;
				}
				Targets.Add(Target);
				TargetToHeaders.Add(Target, null);
			}

			bool bIsRemoteCompile = BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Mac;

			// Create the build configuration object, and read the settings
			BuildConfiguration BuildConfiguration = new BuildConfiguration();
			XmlConfig.ApplyTo(BuildConfiguration);
			CommandLine.ParseArguments(Arguments, BuildConfiguration);
			BuildConfiguration.bUseUBTMakefiles = false;

			Action[] PrerequisiteActions;
			{
				HashSet<Action> PrerequisiteActionsSet = new HashSet<Action>();
				foreach (FileItem OutputFile in OutputFiles)
				{
					ActionGraph.GatherPrerequisiteActions(OutputFile, ref PrerequisiteActionsSet);
				}
				PrerequisiteActions = PrerequisiteActionsSet.ToArray();
			}

			// Copy any asset catalog files to the remote Mac, if necessary.
			foreach (UEBuildTarget Target in Targets)
			{
				UEBuildPlatform.GetBuildPlatform(Target.Platform).PreBuildSync();
			}

			// Begin execution of the ActionGraph.
			Dictionary<UEBuildTarget, List<FileItem>> TargetToOutdatedPrerequisitesMap;
			List<Action> ActionsToExecute = ActionGraph.GetActionsToExecute(BuildConfiguration, PrerequisiteActions, Targets, TargetToHeaders, out TargetToOutdatedPrerequisitesMap);
			string ExecutorName = "Unknown";
			bool bSuccess = ActionGraph.ExecuteActions(BuildConfiguration, ActionsToExecute, bIsRemoteCompile, out ExecutorName, "", EHotReload.Disabled);
			if (bSuccess)
			{
				if (bIsRemoteCompile)
				{
					// Copy the remotely built AssetCatalog directory locally.
					foreach (FileItem OutputFile in OutputFiles)
					{
						string RemoteDirectory = System.IO.Path.GetDirectoryName(OutputFile.AbsolutePath).Replace("\\", "/");
						FileItem LocalExecutable = ToolChain.RemoteToLocalFileItem(FileItem.GetItemByPath(Executable));
						string LocalDirectory = System.IO.Path.Combine(System.IO.Path.GetDirectoryName(LocalExecutable.AbsolutePath), "AssetCatalog");
						LocalDirectory = StageDirectory;
						RPCUtilHelper.CopyDirectory(RemoteDirectory, LocalDirectory, RPCUtilHelper.ECopyOptions.DoNotReplace);
					}
				}
				else
				{
					// Copy the built AssetCatalog directory to the StageDirectory.
					foreach (FileItem OutputFile in OutputFiles)
					{
						string SourceDirectory = System.IO.Path.GetDirectoryName(OutputFile.AbsolutePath).Replace("\\", "/");
						System.IO.DirectoryInfo SourceDirectoryInfo = new System.IO.DirectoryInfo(SourceDirectory);
						if (!System.IO.Directory.Exists(StageDirectory))
						{
							System.IO.Directory.CreateDirectory(StageDirectory);
						}
						System.IO.FileInfo[] SourceFiles = SourceDirectoryInfo.GetFiles();
						foreach (System.IO.FileInfo SourceFile in SourceFiles)
						{
							string DestinationPath = System.IO.Path.Combine(StageDirectory, SourceFile.Name);
							SourceFile.CopyTo(DestinationPath, true);
						}
					}
				}
			}

			// Restore the former bUseRPCUtil setting.
			RemoteToolChain.bUseRPCUtil = bSaveUseRPCUtil;
		}

        /// <summary>
        /// 
        /// </summary>
        public static bool SupportsIconCatalog(UnrealTargetConfiguration Config, DirectoryReference ProjectDirectory, bool bIsUE4Game, string ProjectName)
        {
            // get the receipt
            FileReference ReceiptFilename;
            if (bIsUE4Game)
            {
                ReceiptFilename = TargetReceipt.GetDefaultPath(UnrealBuildTool.EngineDirectory, "UE4Game", UnrealTargetPlatform.IOS, Config, "");
            }
            else
            {
                ReceiptFilename = TargetReceipt.GetDefaultPath(ProjectDirectory, ProjectName, UnrealTargetPlatform.IOS, Config, "");
            }

            string RelativeEnginePath = UnrealBuildTool.EngineDirectory.MakeRelativeTo(DirectoryReference.GetCurrentDirectory());

            if (System.IO.File.Exists(ReceiptFilename.FullName))
            {
                TargetReceipt Receipt = TargetReceipt.Read(ReceiptFilename, UnrealBuildTool.EngineDirectory, ProjectDirectory);
                var Results = Receipt.AdditionalProperties.Where(x => x.Name == "SDK");

                if (Results.Count() > 0)
                {
                    if (Single.Parse(Results.ElementAt(0).Value) >= 11.0f)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }
            return false;
        }
    }
}
