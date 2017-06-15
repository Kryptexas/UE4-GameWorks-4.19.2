// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using AutomationTool;
using UnrealBuildTool;

public struct StageTarget
{
	public TargetReceipt Receipt;
	public bool RequireFilesExist;
}

public class FilesToStage
{
	/// <summary>
	/// After staging, this is a map from staged file to source file. These file are binaries, etc and can't go into a pak file.
	/// </summary>
	public Dictionary<StagedFileReference, FileReference> NonUFSStagingFiles = new Dictionary<StagedFileReference, FileReference>();

	/// <summary>
	/// After staging, this is a map from staged file to source file. These file are for debugging, and should not go into a pak file.
	/// </summary>
	public Dictionary<StagedFileReference, FileReference> NonUFSStagingFilesDebug = new Dictionary<StagedFileReference, FileReference>();

	/// <summary>
	/// After staging, this is a map from staged file to source file. These file are content, and can go into a pak file.
	/// </summary>
	public Dictionary<StagedFileReference, FileReference> UFSStagingFiles = new Dictionary<StagedFileReference, FileReference>();

	/// <summary>
	/// Adds a file to be staged as the given type
	/// </summary>
	/// <param name="FileType">The type of file to be staged</param>
	/// <param name="StagedFile">The staged file location</param>
	/// <param name="InputFile">The input file</param>
	public void Add(StagedFileType FileType, StagedFileReference StagedFile, FileReference InputFile)
	{
		if (FileType == StagedFileType.UFS)
		{
			AddToDictionary(UFSStagingFiles, StagedFile, InputFile);
		}
		else if (FileType == StagedFileType.NonUFS)
		{
			AddToDictionary(NonUFSStagingFiles, StagedFile, InputFile);
		}
		else if (FileType == StagedFileType.DebugNonUFS)
		{
			AddToDictionary(NonUFSStagingFilesDebug, StagedFile, InputFile);
		}
	}

	/// <summary>
	/// Adds a file to be staged to the given dictionary
	/// </summary>
	/// <param name="FilesToStage">Dictionary of files to be staged</param>
	/// <param name="StagedFile">The staged file location</param>
	/// <param name="InputFile">The input file</param>
	private void AddToDictionary(Dictionary<StagedFileReference, FileReference> FilesToStage, StagedFileReference StagedFile, FileReference InputFile)
	{
		FilesToStage[StagedFile] = InputFile;
	}
}

public class DeploymentContext //: ProjectParams
{
	/// <summary>
	/// Full path to the .uproject file
	/// </summary>
	public FileReference RawProjectPath;

	/// <summary>
	///  true if we should stage crash reporter
	/// </summary>
	public bool bStageCrashReporter;

    /// <summary>
    ///  CookPlatform, where to get the cooked data from and use for sandboxes
    /// </summary>
    public string CookPlatform;

	/// <summary>
    ///  FinalCookPlatform, directory to stage and archive the final result to
	/// </summary>
    public string FinalCookPlatform;

    /// <summary>
    ///  Source platform to get the cooked data from
    /// </summary>
    public Platform CookSourcePlatform;

	/// <summary>
	///  Target platform used for sandboxes and stage directory names
	/// </summary>
	public Platform StageTargetPlatform;

	/// <summary>
	///  Configurations to stage. Used to determine which ThirdParty configurations to copy.
	/// </summary>
	public List<UnrealTargetConfiguration> StageTargetConfigurations;

	/// <summary>
	/// Receipts for the build targets that should be staged.
	/// </summary>
	public List<StageTarget> StageTargets;

	/// <summary>
	/// This is the root directory that contains the engine: d:\a\UE4\
	/// </summary>
	public DirectoryReference LocalRoot;

	/// <summary>
	/// This is the directory that contains the engine.
	/// </summary>
	public DirectoryReference EngineRoot;

	/// <summary>
	/// The directory that contains the project: d:\a\UE4\ShooterGame
	/// </summary>
	public DirectoryReference ProjectRoot;

	/// <summary>
	///  raw name used for platform subdirectories Win32
	/// </summary>
	public string PlatformDir;

	/// <summary>
	/// Directory to put all of the files in: d:\stagedir\WindowsNoEditor
	/// </summary>
	public DirectoryReference StageDirectory;

	/// <summary>
	/// Directory name for staged projects
	/// </summary>
	public StagedDirectoryReference RelativeProjectRootForStage;

	/// <summary>
	/// This is what you use to test the engine which uproject you want. Many cases.
	/// </summary>
	public string ProjectArgForCommandLines;

	/// <summary>
	/// The directory containing the cooked data to be staged. This may be different to the target platform, eg. when creating cooked data for dedicated servers.
	/// </summary>
	public DirectoryReference CookSourceRuntimeRootDir;

	/// <summary>
	/// This is the root that we are going to run from. This will be the stage directory if we're staging, or the input directory if not.
	/// </summary>
	public DirectoryReference RuntimeRootDir;

	/// <summary>
	/// This is the project root that we are going to run from. Many cases.
	/// </summary>
	public DirectoryReference RuntimeProjectRootDir;

	/// <summary>
	/// List of executables we are going to stage
	/// </summary>
	public List<string> StageExecutables;

	/// <summary>
	/// Probably going away, used to construct ProjectArgForCommandLines in the case that we are running staged
	/// </summary>
	public const string UProjectCommandLineArgInternalRoot = "../../../";

	/// <summary>
	/// Probably going away, used to construct the pak file list
	/// </summary>
	public string PakFileInternalRoot = "../../../";

	/// <summary>
	/// List of files to be staged
	/// </summary>
	public FilesToStage FilesToStage = new FilesToStage();

	/// <summary>
	/// List of files to be archived
	/// </summary>
	public Dictionary<string, string> ArchivedFiles = new Dictionary<string, string>();

	/// <summary>
	///  Directory to archive all of the files in: d:\archivedir\WindowsNoEditor
	/// </summary>
	public DirectoryReference ArchiveDirectory;

	/// <summary>
	///  Directory to project binaries
	/// </summary>
	public DirectoryReference ProjectBinariesFolder;

	/// <summary>
	/// Filename for the manifest of file changes for iterative deployment.
	/// </summary>
	public const string UFSDeployDeltaFileName = "Manifest_DeltaUFSFiles.txt";	
	public const string NonUFSDeployDeltaFileName = "Manifest_DeltaNonUFSFiles.txt";

	/// <summary>
	/// Filename for the manifest of files to delete during deployment.
	/// </summary>
	public const string UFSDeployObsoleteFileName = "Manifest_ObsoleteUFSFiles.txt";
	public const string NonUFSDeployObsoleteFileName = "Manifest_ObsoleteNonUFSFiles.txt";

	/// <summary>
	/// The client connects to dedicated server to get data
	/// </summary>
	public bool DedicatedServer;

	/// <summary>
	/// True if this build is staged
	/// </summary>
	public bool Stage;

	/// <summary>
	/// True if this build is archived
	/// </summary>
	public bool Archive;

	/// <summary>
	/// True if this project has code
	/// </summary>	
	public bool IsCodeBasedProject;

	/// <summary>
	/// Project name (name of the uproject file without extension or directory name where the project is localed)
	/// </summary>
	public string ShortProjectName;

	/// <summary>
	/// If true, multiple platforms are being merged together - some behavior needs to change (but not much)
	/// </summary>
	public bool bIsCombiningMultiplePlatforms = false;

    /// <summary>
    /// If true if this platform is using streaming install chunk manifests
    /// </summary>
    public bool PlatformUsesChunkManifests = false;

    public DeploymentContext(
		FileReference RawProjectPathOrName,
		DirectoryReference InLocalRoot,
		DirectoryReference BaseStageDirectory,
		DirectoryReference BaseArchiveDirectory,
		Platform InSourcePlatform,
        Platform InTargetPlatform,
		List<UnrealTargetConfiguration> InTargetConfigurations,
		IEnumerable<StageTarget> InStageTargets,
		List<String> InStageExecutables,
		bool InServer,
		bool InCooked,
		bool InStageCrashReporter,
		bool InStage,
		bool InCookOnTheFly,
		bool InArchive,
		bool InProgram,
		bool IsClientInsteadOfNoEditor,
        bool InForceChunkManifests
		)
	{
		bStageCrashReporter = InStageCrashReporter;
		RawProjectPath = RawProjectPathOrName;
		DedicatedServer = InServer;
		LocalRoot = InLocalRoot;
        CookSourcePlatform = InSourcePlatform;
		StageTargetPlatform = InTargetPlatform;
		StageTargetConfigurations = new List<UnrealTargetConfiguration>(InTargetConfigurations);
		StageTargets = new List<StageTarget>(InStageTargets);
		StageExecutables = InStageExecutables;
        IsCodeBasedProject = ProjectUtils.IsCodeBasedUProjectFile(RawProjectPath);
		ShortProjectName = ProjectUtils.GetShortProjectName(RawProjectPath);
		Stage = InStage;
		Archive = InArchive;

        if (CookSourcePlatform != null && InCooked)
        {
			CookPlatform = CookSourcePlatform.GetCookPlatform(DedicatedServer, IsClientInsteadOfNoEditor);
        }
        else if (CookSourcePlatform != null && InProgram)
        {
            CookPlatform = CookSourcePlatform.GetCookPlatform(false, false);
        }
        else
        {
            CookPlatform = "";
        }

		if (StageTargetPlatform != null && InCooked)
		{
			FinalCookPlatform = StageTargetPlatform.GetCookPlatform(DedicatedServer, IsClientInsteadOfNoEditor);
		}
		else if (StageTargetPlatform != null && InProgram)
		{
            FinalCookPlatform = StageTargetPlatform.GetCookPlatform(false, false);
		}
		else
		{
            FinalCookPlatform = "";
		}

		PlatformDir = StageTargetPlatform.PlatformType.ToString();

		if (BaseStageDirectory != null)
		{
			StageDirectory = DirectoryReference.Combine(BaseStageDirectory, FinalCookPlatform);
		}

		if(BaseArchiveDirectory != null)
		{
			ArchiveDirectory = DirectoryReference.Combine(BaseArchiveDirectory, FinalCookPlatform);
		}

		if (!FileReference.Exists(RawProjectPath))
		{
			throw new AutomationException("Can't find uproject file {0}.", RawProjectPathOrName);
		}

		EngineRoot = DirectoryReference.Combine(LocalRoot, "Engine");
		ProjectRoot = RawProjectPath.Directory;

		RelativeProjectRootForStage = new StagedDirectoryReference(ShortProjectName);

		ProjectArgForCommandLines = CommandUtils.MakePathSafeToUseWithCommandLine(RawProjectPath.FullName);
		CookSourceRuntimeRootDir = RuntimeRootDir = LocalRoot;
		RuntimeProjectRootDir = ProjectRoot;
       
        if (Stage)
		{
			CommandUtils.CreateDirectory(StageDirectory.FullName);

			RuntimeRootDir = StageDirectory;
			CookSourceRuntimeRootDir = DirectoryReference.Combine(BaseStageDirectory, CookPlatform);
			RuntimeProjectRootDir = DirectoryReference.Combine(StageDirectory, RelativeProjectRootForStage.Name);
			ProjectArgForCommandLines = CommandUtils.MakePathSafeToUseWithCommandLine(UProjectCommandLineArgInternalRoot + RelativeProjectRootForStage.Name + "/" + ShortProjectName + ".uproject");
		}
		if (Archive)
		{
			CommandUtils.CreateDirectory(ArchiveDirectory.FullName);
		}
		ProjectArgForCommandLines = ProjectArgForCommandLines.Replace("\\", "/");
		ProjectBinariesFolder = DirectoryReference.Combine(ProjectUtils.GetClientProjectBinariesRootPath(RawProjectPath, TargetType.Game, IsCodeBasedProject), PlatformDir);

        // If we were configured to use manifests across the whole project, then this platform should use manifests.
        // Otherwise, read whether we are generating chunks from the ProjectPackagingSettings ini.
        if (InForceChunkManifests)
        {
            PlatformUsesChunkManifests = true;
        }
        else
        {
            ConfigHierarchy GameIni = ConfigCache.ReadHierarchy(ConfigHierarchyType.Game, ProjectRoot, InTargetPlatform.PlatformType);
            String IniPath = "/Script/UnrealEd.ProjectPackagingSettings";
            bool bSetting = false;
            if (GameIni.GetBool(IniPath, "bGenerateChunks", out bSetting))
            {
                PlatformUsesChunkManifests = bSetting;
            }
        }
    }

	public void StageFile(StagedFileType FileType, FileReference InputFile, StagedFileReference OutputFile = null, bool bRemap = true)
	{
		if(OutputFile == null)
		{
			if(InputFile.IsUnderDirectory(ProjectRoot))
			{
				OutputFile = StagedFileReference.Combine(RelativeProjectRootForStage, InputFile.MakeRelativeTo(ProjectRoot));
			}
            else if (InputFile.HasExtension(".uplugin"))
            {
                if (InputFile.IsUnderDirectory(EngineRoot))
				{
					OutputFile = new StagedFileReference(InputFile.MakeRelativeTo(LocalRoot));
				}
                else
				{
					// This is a plugin that lives outside of the Engine/Plugins or Game/Plugins directory so needs to be remapped for staging/packaging
					// We need to remap C:\SomePath\PluginName\PluginName.uplugin to RemappedPlugins\PluginName\PluginName.uplugin
					OutputFile = new StagedFileReference(String.Format("RemappedPlugins/{0}/{1}", InputFile.GetFileNameWithoutExtension(), InputFile.GetFileName()));
				}
            }
            else if (InputFile.IsUnderDirectory(LocalRoot))
            {
				OutputFile = new StagedFileReference(InputFile.MakeRelativeTo(LocalRoot));
            }
            else
            {
				throw new AutomationException("Can't deploy {0} because it doesn't start with {1} or {2}", InputFile, ProjectRoot, LocalRoot);
			}
		}

		if(bRemap)
		{
			OutputFile = StageTargetPlatform.Remap(OutputFile);
		}

		FilesToStage.Add(FileType, OutputFile, InputFile);
	}

	public void StageBuildProductsFromReceipt(TargetReceipt Receipt, bool RequireDependenciesToExist, bool TreatNonShippingBinariesAsDebugFiles)
	{
		// Stage all the build products needed at runtime
		foreach(BuildProduct BuildProduct in Receipt.BuildProducts)
		{
			// allow missing files if needed
			if (RequireDependenciesToExist == false && FileReference.Exists(BuildProduct.Path) == false)
			{
				continue;
			}

			if(BuildProduct.Type == BuildProductType.Executable || BuildProduct.Type == BuildProductType.DynamicLibrary || BuildProduct.Type == BuildProductType.RequiredResource)
			{
				StagedFileType FileTypeToUse = StagedFileType.NonUFS;
				if (TreatNonShippingBinariesAsDebugFiles && Receipt.Configuration != UnrealTargetConfiguration.Shipping)
				{
					FileTypeToUse = StagedFileType.DebugNonUFS;
				}

				StageFile(FileTypeToUse, BuildProduct.Path);
			}
			else if(BuildProduct.Type == BuildProductType.SymbolFile || BuildProduct.Type == BuildProductType.MapFile)
			{
				// Symbol files aren't true dependencies so we can skip if they don't exist
				if (FileReference.Exists(BuildProduct.Path))
				{
					StageFile(StagedFileType.DebugNonUFS, BuildProduct.Path);
				}
			}
		}
	}

	public void StageRuntimeDependenciesFromReceipt(TargetReceipt Receipt, bool RequireDependenciesToExist, bool bUsingPakFile)
	{
		// Patterns to exclude from wildcard searches. Any maps and assets must be cooked. 
		List<string> ExcludePatterns = new List<string>();
		ExcludePatterns.Add(".../*.umap");
		ExcludePatterns.Add(".../*.uasset");

		// Also stage any additional runtime dependencies, like ThirdParty DLLs
		foreach(RuntimeDependency RuntimeDependency in Receipt.RuntimeDependencies)
		{
			// allow missing files if needed
			if ((RequireDependenciesToExist && RuntimeDependency.Type != StagedFileType.DebugNonUFS) || FileReference.Exists(RuntimeDependency.Path))
			{
				bool bRemap = RuntimeDependency.Type != StagedFileType.UFS || !bUsingPakFile;
				StageFile(RuntimeDependency.Type, RuntimeDependency.Path, bRemap: bRemap);
			}
		}
	}

	/// <summary>
	/// Correctly collapses any ../ or ./ entries in a path.
	/// </summary>
	/// <param name="InPath">The path to be collapsed</param>
	/// <returns>true if the path could be collapsed, false otherwise.</returns>
	static bool CollapseRelativeDirectories(ref string InPath)
	{
		string LocalString = InPath;
		bool bHadBackSlashes = false;
		// look to see what kind of slashes we had
		if (LocalString.IndexOf("\\") != -1)
		{
			LocalString = LocalString.Replace("\\", "/");
			bHadBackSlashes = true;
		}

		string ParentDir = "/..";
		int ParentDirLength = ParentDir.Length;

		for (; ; )
		{
			// An empty path is finished
			if (string.IsNullOrEmpty(LocalString))
				break;

			// Consider empty paths or paths which start with .. or /.. as invalid
			if (LocalString.StartsWith("..") || LocalString.StartsWith(ParentDir))
				return false;

			// If there are no "/.."s left then we're done
			int Index = LocalString.IndexOf(ParentDir);
			if (Index == -1)
				break;

			int PreviousSeparatorIndex = Index;
			for (; ; )
			{
				// Find the previous slash
				PreviousSeparatorIndex = Math.Max(0, LocalString.LastIndexOf("/", PreviousSeparatorIndex - 1));

				// Stop if we've hit the start of the string
				if (PreviousSeparatorIndex == 0)
					break;

				// Stop if we've found a directory that isn't "/./"
				if ((Index - PreviousSeparatorIndex) > 1 && (LocalString[PreviousSeparatorIndex + 1] != '.' || LocalString[PreviousSeparatorIndex + 2] != '/'))
					break;
			}

			// If we're attempting to remove the drive letter, that's illegal
			int Colon = LocalString.IndexOf(":", PreviousSeparatorIndex);
			if (Colon >= 0 && Colon < Index)
				return false;

			LocalString = LocalString.Substring(0, PreviousSeparatorIndex) + LocalString.Substring(Index + ParentDirLength);
		}

		LocalString = LocalString.Replace("./", "");

		// restore back slashes now
		if (bHadBackSlashes)
		{
			LocalString = LocalString.Replace("/", "\\");
		}

		// and pass back out
		InPath = LocalString;
		return true;
	}

	bool IsFileForOtherPlatform(FileReference InputFile)
	{
        foreach (UnrealTargetPlatform Plat in Enum.GetValues(typeof(UnrealTargetPlatform)))
        {
			bool bMatchesIniPlatform = (InputFile.HasExtension(".ini") && Plat == StageTargetPlatform.IniPlatformType); // filter ini files for the ini file platform
			bool bMatchesTargetPlatform = (Plat == StageTargetPlatform.PlatformType || Plat == UnrealTargetPlatform.Unknown); // filter platform files for the target platform
			if (!bMatchesIniPlatform && !bMatchesTargetPlatform)
			{
				FileSystemName PlatformName = new FileSystemName(Plat.ToString());
				if (InputFile.IsUnderDirectory(ProjectRoot))
				{
					if (InputFile.ContainsName(PlatformName, ProjectRoot))
					{
						return true;
					}
				}
				else if (InputFile.IsUnderDirectory(LocalRoot))
				{
					if (InputFile.ContainsName(PlatformName, LocalRoot))
					{
						return true;
					}
				}
			}
        }
		return false;
	}

    public void StageFiles(StagedFileType FileType, DirectoryReference InputDir, string Wildcard = "*", bool bRecursive = true, string[] ExcludeWildcards = null, StagedDirectoryReference NewPath = null, bool bAllowNone = false, bool bRemap = true, string NewName = null, bool bAllowNotForLicenseesFiles = true, bool bStripFilesForOtherPlatforms = true, bool bConvertToLower = false)
	{
		int FilesAdded = 0;

		if (DirectoryReference.Exists(InputDir))
		{
			FileReference[] InputFiles = CommandUtils.FindFiles(Wildcard, bRecursive, InputDir);

			HashSet<FileReference> ExcludeFiles = new HashSet<FileReference>();
			if (ExcludeWildcards != null)
			{
				foreach (string ExcludeWildcard in ExcludeWildcards)
				{
					ExcludeFiles.UnionWith(CommandUtils.FindFiles(ExcludeWildcard, bRecursive, InputDir));
				}
			}

			foreach (FileReference InputFile in InputFiles)
			{
				if (ExcludeFiles.Contains(InputFile))
				{
					continue;
				}
                
				if (bStripFilesForOtherPlatforms && !bIsCombiningMultiplePlatforms && IsFileForOtherPlatform(InputFile))
                {
					continue;
                }

				// Get the staged location for this file
				StagedFileReference Dest;
				if (NewPath != null)
				{
					// If the specified a new directory, first we deal with that, then apply the other things. This is used to collapse the sandbox, among other things.
					Dest = StagedFileReference.Combine(NewPath, InputFile.MakeRelativeTo(InputDir));
                }
				else if (InputFile.IsUnderDirectory(ProjectRoot))
				{
					// Project relative file
					Dest = StagedFileReference.Combine(RelativeProjectRootForStage, InputFile.MakeRelativeTo(ProjectRoot));
				}
				else if (InputFile.IsUnderDirectory(LocalRoot))
				{
					// Engine relative file
					Dest = new StagedFileReference(InputFile.MakeRelativeTo(LocalRoot));
				}
				else
				{
					throw new AutomationException("Can't deploy {0} because it doesn't start with {1} or {2}", InputFile, ProjectRoot, LocalRoot);
				}

                if (!bAllowNotForLicenseesFiles && (Dest.ContainsName(new FileSystemName("NotForLicensees")) || Dest.ContainsName(new FileSystemName("NoRedist"))))
                {
                    continue;
                }

				if (NewName != null)
				{
					Dest = StagedFileReference.Combine(Dest.Directory, NewName);
				}

				if (bRemap)
				{
					Dest = StageTargetPlatform.Remap(Dest);
				}

				if (bConvertToLower)
				{
					Dest = Dest.ToLowerInvariant();
				}

				FilesToStage.Add(FileType, Dest, InputFile);

				FilesAdded++;
			}
		}

		if (FilesAdded == 0 && !bAllowNone && !bIsCombiningMultiplePlatforms)
		{
			throw new AutomationException(ExitCode.Error_StageMissingFile, "No files found to deploy for {0} with wildcard {1} and exclusions {2}", InputDir, Wildcard, String.Join(", ", ExcludeWildcards));
		}

	}

	public int ArchiveFiles(string InPath, string Wildcard = "*", bool bRecursive = true, string[] ExcludeWildcard = null, string NewPath = null)
	{
		int FilesAdded = 0;

		if (CommandUtils.DirectoryExists(InPath))
		{
			var All = CommandUtils.FindFiles(Wildcard, bRecursive, InPath);

			var Exclude = new HashSet<string>();
			if (ExcludeWildcard != null)
			{
				foreach (var Excl in ExcludeWildcard)
				{
					var Remove = CommandUtils.FindFiles(Excl, bRecursive, InPath);
					foreach (var File in Remove)
					{
						Exclude.Add(CommandUtils.CombinePaths(File));
					}
				}
			}
			foreach (var AllFile in All)
			{
				var FileToCopy = CommandUtils.CombinePaths(AllFile);
				if (Exclude.Contains(FileToCopy))
				{
					continue;
				}

				if (!bIsCombiningMultiplePlatforms)
				{
					FileReference InputFile = new FileReference(FileToCopy);

					bool OtherPlatform = false;
					foreach (UnrealTargetPlatform Plat in Enum.GetValues(typeof(UnrealTargetPlatform)))
					{
                        if (Plat != StageTargetPlatform.PlatformType && Plat != UnrealTargetPlatform.Unknown)
                        {
                            var Search = FileToCopy;
                            if (InputFile.IsUnderDirectory(LocalRoot))
                            {
								Search = InputFile.MakeRelativeTo(LocalRoot);
                            }
							else if (InputFile.IsUnderDirectory(ProjectRoot))
							{
								Search = InputFile.MakeRelativeTo(ProjectRoot);
							}
                            if (Search.IndexOf(CommandUtils.CombinePaths("/" + Plat.ToString() + "/"), 0, StringComparison.InvariantCultureIgnoreCase) >= 0)
                            {
                                OtherPlatform = true;
                                break;
                            }
                        }
					}
					if (OtherPlatform)
					{
						continue;
					}
				}

				string Dest;
				if (!FileToCopy.StartsWith(InPath))
				{
					throw new AutomationException("Can't archive {0}; it was supposed to start with {1}", FileToCopy, InPath);
				}

				// If the specified a new directory, first we deal with that, then apply the other things
				// this is used to collapse the sandbox, among other things
				if (NewPath != null)
				{
					Dest = FileToCopy.Substring(InPath.Length);
					if (Dest.StartsWith("/") || Dest.StartsWith("\\"))
					{
						Dest = Dest.Substring(1);
					}
					Dest = CommandUtils.CombinePaths(NewPath, Dest);
				}
				else
				{
					Dest = FileToCopy.Substring(InPath.Length);
				}

				if (Dest.StartsWith("/") || Dest.StartsWith("\\"))
				{
					Dest = Dest.Substring(1);
				}

				if (ArchivedFiles.ContainsKey(FileToCopy))
				{
					if (ArchivedFiles[FileToCopy] != Dest)
					{
						throw new AutomationException("Can't archive {0}: it was already in the files to archive with a different destination '{1}'", FileToCopy, Dest);
					}
				}
				else
				{
					ArchivedFiles.Add(FileToCopy, Dest);
				}

				FilesAdded++;
			}
		}

		return FilesAdded;
	}

	public String GetUFSDeploymentDeltaPath(string DeviceName)
	{
		//replace the port name in the case of deploy while adb is using wifi
		string SanitizedDeviceName = DeviceName.Replace(":", "_");

		return Path.Combine(StageDirectory.FullName, UFSDeployDeltaFileName + SanitizedDeviceName);
	}

	public String GetNonUFSDeploymentDeltaPath(string DeviceName)
	{
		//replace the port name in the case of deploy while adb is using wifi
		string SanitizedDeviceName = DeviceName.Replace(":", "_");

		return Path.Combine(StageDirectory.FullName, NonUFSDeployDeltaFileName + SanitizedDeviceName);
	}

	public String GetUFSDeploymentObsoletePath(string DeviceName)
	{
		//replace the port name in the case of deploy while adb is using wifi
		string SanitizedDeviceName = DeviceName.Replace(":", "_");

		return Path.Combine(StageDirectory.FullName, UFSDeployObsoleteFileName + SanitizedDeviceName);
	}

	public String GetNonUFSDeploymentObsoletePath(string DeviceName)
	{
		//replace the port name in the case of deploy while adb is using wifi
		string SanitizedDeviceName = DeviceName.Replace(":", "_");

		return Path.Combine(StageDirectory.FullName, NonUFSDeployObsoleteFileName + SanitizedDeviceName);
	}

	public string UFSDeployedManifestFileName
	{
		get
		{
			return "Manifest_UFSFiles_" + StageTargetPlatform.PlatformType.ToString() + ".txt";
		}
	}

	public string NonUFSDeployedManifestFileName
	{
		get
		{
			return "Manifest_NonUFSFiles_" + StageTargetPlatform.PlatformType.ToString() + ".txt";
		}
	}

	public static string GetNonUFSDeployedManifestFileName(UnrealTargetPlatform PlatformType)
	{
		return "Manifest_NonUFSFiles_" + PlatformType.ToString() + ".txt";
	}

	public static string GetUFSDeployedManifestFileName(UnrealTargetPlatform PlatformType)
	{
		return "Manifest_UFSFiles_" + PlatformType.ToString() + ".txt";
	}

	public static string GetDebugFilesManifestFileName(UnrealTargetPlatform PlatformType)
	{
		return "Manifest_DebugFiles_" + PlatformType.ToString() + ".txt";
	}
}
