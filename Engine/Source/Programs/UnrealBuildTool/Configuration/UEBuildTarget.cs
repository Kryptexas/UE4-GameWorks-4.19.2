// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Xml;

namespace UnrealBuildTool
{
	public enum UnrealTargetPlatform
	{
		Unknown,
		Win32,
		Win64,
		Mac,
		XboxOne,
		PS4,
		IOS,
		Android,
		WinRT,
		WinRT_ARM,
		HTML5,
        Linux,
	}

	public enum UnrealPlatformGroup
	{
		Windows,	// this group is just to lump Win32 and Win64 into Windows directories, removing the special Windows logic in MakeListOfUnsupportedPlatforms
		Microsoft,
		Apple,
		Unix,
		Android,
		Sony,
          /*
          *  These two groups can be further used to conditionally compile files for a given platform. e.g
          *  Core/Private/HTML5/Simulator/<VC tool chain files>
          *  Core/Private/HTML5/Device/<emscripten toolchain files>.  
          *  Note: There's no default group - if the platform is not registered as device or simulator - both are rejected. 
          */
        Device, 
        Simulator, 
	}

	public enum UnrealTargetConfiguration
	{
		Unknown,
		Debug,
		DebugGame,
		Development,
		Shipping,
		Test,
	}

	public enum UnrealProjectType
	{
		CPlusPlus,	// C++ or C++/CLI
		CSharp,		// C#
	}

	public enum ErrorMode
	{
		Ignore,
		Suppress,
		Check,
	}

	/** Helper class for holding project name, platform and config together. */
	public class UnrealBuildConfig
	{
		/** Project to build. */
		public string Name;
		/** Platform to build the project for. */ 
		public UnrealTargetPlatform Platform;
		/** Config to build the project with. */
		public UnrealTargetConfiguration Config;

		/** Constructor */
		public UnrealBuildConfig(string InName, UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfig)
		{
			Name = InName;
			Platform = InPlatform;
			Config = InConfig;
		}

		/** Overriden ToString() to make this class esier to read when debugging. */
		public override string ToString()
		{
			return String.Format("{0}, {1}, {2}", Name, Platform, Config);
		}
	};

	delegate UEBuildTarget TargetCreationDelegate();

	/// <summary>
	/// A container for a binary files (dll, exe) with its associated debug info.
	/// </summary>
	public class FileManifest
	{
		public readonly List<string> FileManifestItems = new List<string>();

		public FileManifest()
		{
		}

		public void AddFileName( string FilePath )
		{
			FileManifestItems.Add( Path.GetFullPath( FilePath ) );
		}

		public void AddBinaryNames(string OutputFilePath, string DebugInfoExtension)
		{
			// Only add unique files to the manifest. Multiple games have many shared binaries.
			if( !FileManifestItems.Any( x => Path.GetFullPath( OutputFilePath ) == x ) )
			{
				string FullPath = Path.GetFullPath(OutputFilePath);
				FileManifestItems.Add( FullPath );
				if (!string.IsNullOrEmpty(DebugInfoExtension))
				{
					FileManifestItems.Add(Path.ChangeExtension(FullPath, DebugInfoExtension));
				}
			}
		}
	}

	public class OnlyModule
	{
		public OnlyModule(string InitOnlyModuleName)
		{
			OnlyModuleName = InitOnlyModuleName;
			OnlyModuleSuffix = String.Empty;
		}

		public OnlyModule(string InitOnlyModuleName, string InitOnlyModuleSuffix)
		{
			OnlyModuleName = InitOnlyModuleName;
			OnlyModuleSuffix = InitOnlyModuleSuffix;
		}

		/** If building only a single module, this is the module name to build */
		public readonly string OnlyModuleName;

		/** When building only a single module, the optional suffix for the module file name */
		public readonly string OnlyModuleSuffix;
	}

	public class UEBuildTarget
	{
		public string GetAppName()
		{
			return AppName;
		}

		public string GetTargetName()
		{
			return !String.IsNullOrEmpty(GameName) ? GameName : AppName;
		}

		public static UnrealTargetPlatform CPPTargetPlatformToUnrealTargetPlatform(CPPTargetPlatform InCPPPlatform)
		{
			switch (InCPPPlatform)
			{
				case CPPTargetPlatform.Win32:			return UnrealTargetPlatform.Win32;
				case CPPTargetPlatform.Win64:			return UnrealTargetPlatform.Win64;
				case CPPTargetPlatform.Mac:				return UnrealTargetPlatform.Mac;
				case CPPTargetPlatform.XboxOne:			return UnrealTargetPlatform.XboxOne;
				case CPPTargetPlatform.PS4:				return UnrealTargetPlatform.PS4;
				case CPPTargetPlatform.Android:			return UnrealTargetPlatform.Android;
				case CPPTargetPlatform.WinRT: 			return UnrealTargetPlatform.WinRT;
				case CPPTargetPlatform.WinRT_ARM: 		return UnrealTargetPlatform.WinRT_ARM;
				case CPPTargetPlatform.IOS:				return UnrealTargetPlatform.IOS;
				case CPPTargetPlatform.HTML5:			return UnrealTargetPlatform.HTML5;
                case CPPTargetPlatform.Linux:           return UnrealTargetPlatform.Linux;
			}
			throw new BuildException("CPPTargetPlatformToUnrealTargetPlatform: Unknown CPPTargetPlatform {0}", InCPPPlatform.ToString());
		}

		public static UEBuildTarget CreateTarget(string[] SourceArguments)
		{
			TargetCreationDelegate TargetCreationDelegate = null;
			var AdditionalDefinitions = new List<string>();
			var Platform = UnrealTargetPlatform.Unknown;
			var Configuration = UnrealTargetConfiguration.Unknown;
			string RemoteRoot = null;

			var OnlyModules = new List<OnlyModule>();

			// Combine the two arrays of arguments
			List<string> Arguments = new List<string>(SourceArguments.Length);
			Arguments.AddRange(SourceArguments);

			// If true, the recompile was launched by the editor.
			// This means we actually have to 
			bool bIsEditorRecompile = false;

			List<string> PossibleTargetNames = new List<string>();
			for (int ArgumentIndex = 0; ArgumentIndex < Arguments.Count; ArgumentIndex++)
			{
				UnrealTargetPlatform ParsedPlatform = UEBuildPlatform.ConvertStringToPlatform(Arguments[ArgumentIndex]);
				if (ParsedPlatform != UnrealTargetPlatform.Unknown)
				{
					Platform = ParsedPlatform;
				}
				else
				{
					switch (Arguments[ArgumentIndex].ToUpperInvariant())
					{
						case "-MODULE":
							// Specifies a module to recompile.  Can be specified more than once on the command-line to compile multiple specific modules.
							{
								if (ArgumentIndex + 1 >= Arguments.Count)
								{
									throw new BuildException("Expected module name after -Module argument, but found nothing.");
								}
								var OnlyModuleName = Arguments[++ArgumentIndex];

								OnlyModules.Add(new OnlyModule(OnlyModuleName));
							}
							break;

						case "-MODULEWITHSUFFIX":
							{
								// Specifies a module name to compile along with a suffix to append to the DLL file name.  Can be specified more than once on the command-line to compile multiple specific modules.
								if (ArgumentIndex + 2 >= Arguments.Count)
								{
									throw new BuildException("Expected module name and module suffix -ModuleWithSuffix argument");
								}

								var OnlyModuleName = Arguments[++ArgumentIndex];
								var OnlyModuleSuffix = Arguments[++ArgumentIndex];

								OnlyModules.Add(new OnlyModule(OnlyModuleName, OnlyModuleSuffix));
							}
							break;

						// Configuration names:
						case "DEBUG":
							Configuration = UnrealTargetConfiguration.Debug;
							break;
						case "DEBUGGAME":
							Configuration = UnrealTargetConfiguration.DebugGame;
							break;
						case "DEVELOPMENT":
							Configuration = UnrealTargetConfiguration.Development;
							break;
						case "SHIPPING":
							Configuration = UnrealTargetConfiguration.Shipping;
							break;
						case "TEST":
							Configuration = UnrealTargetConfiguration.Test;
							break;

						// -Define <definition> adds a definition to the global C++ compilation environment.
						case "-DEFINE":
							if (ArgumentIndex + 1 >= Arguments.Count)
							{
								throw new BuildException("Expected path after -define argument, but found nothing.");
							}
							ArgumentIndex++;
							AdditionalDefinitions.Add(Arguments[ArgumentIndex]);
							break;

						// -RemoteRoot <RemoteRoot> sets where the generated binaries are CookerSynced.
						case "-REMOTEROOT":
							if (ArgumentIndex + 1 >= Arguments.Count)
							{
								throw new BuildException("Expected path after -RemoteRoot argument, but found nothing.");
							}
							ArgumentIndex++;
							if (Arguments[ArgumentIndex].StartsWith("xe:\\") == true)
							{
								RemoteRoot = Arguments[ArgumentIndex].Substring("xe:\\".Length);
							}
							else if (Arguments[ArgumentIndex].StartsWith("devkit:\\") == true)
							{
								RemoteRoot = Arguments[ArgumentIndex].Substring("devkit:\\".Length);
							}
							break;

						// Disable editor support
						case "-NOEDITOR":
							UEBuildConfiguration.bBuildEditor = false;
							break;

						case "-NOEDITORONLYDATA":
							UEBuildConfiguration.bBuildWithEditorOnlyData = false;
							break;

						case "-DEPLOY":
							// Does nothing at the moment...
							break;

						case "-PROJECTFILES":
							{
								// Force platform to Win64 for building IntelliSense files
								Platform = UnrealTargetPlatform.Win64;

								// Force configuration to Development for IntelliSense
								Configuration = UnrealTargetConfiguration.Development;
							}
							break;

						case "-XCODEPROJECTFILE":
							{
								// @todo Mac: Don't want to force a platform/config for generated projects, in case they affect defines/includes (each project's individual configuration should be generated with correct settings)

								// Force platform to Mac for building IntelliSense files
								Platform = UnrealTargetPlatform.Mac;

								// Force configuration to Development for IntelliSense
								Configuration = UnrealTargetConfiguration.Development;
							}
							break;

						case "-EDITORRECOMPILE":
							{
								bIsEditorRecompile = true;
							}
							break;

						default:
							PossibleTargetNames.Add(Arguments[ArgumentIndex]);
							break;
					}
				}
			}

			if (Platform == UnrealTargetPlatform.Unknown)
			{
				throw new BuildException("Couldn't find platform name.");
			}
			if (Configuration == UnrealTargetConfiguration.Unknown)
			{
				throw new BuildException("Couldn't determine configuration name.");
			}

			if (TargetCreationDelegate != null)
			{
				return TargetCreationDelegate();
			}

			if (PossibleTargetNames.Count > 0)
			{
				// We have possible targets!
				UEBuildTarget BuildTarget = null;
				foreach (string PossibleTargetName in PossibleTargetNames)
				{
					// If running Rocket, the PossibleTargetName could contain a path
					var TargetName = PossibleTargetName;

					// If a project file was not specified see if we can find one
					string CheckProjectFile = UProjectInfo.GetProjectForTarget(TargetName);
					if (string.IsNullOrEmpty(CheckProjectFile) == false)
					{
						Log.TraceVerbose("Found project file for {0} - {1}", TargetName, CheckProjectFile);
						if (UnrealBuildTool.HasUProjectFile() == false)
						{
							string NewProjectFilename = CheckProjectFile;
							if (Path.IsPathRooted(NewProjectFilename) == false)
							{
								NewProjectFilename = Path.GetFullPath(NewProjectFilename);
							}

 							NewProjectFilename = NewProjectFilename.Replace("\\", "/");
							UnrealBuildTool.SetProjectFile(NewProjectFilename);
						}
					}

					if (UnrealBuildTool.HasUProjectFile())
					{
						if( TargetName.Contains( "/" ) || TargetName.Contains( "\\" ) )
						{
							// Parse off the path
							TargetName = Path.GetFileNameWithoutExtension( TargetName );
						}
					}

					if( !ProjectFileGenerator.bGenerateProjectFiles )
					{
						// Configure the rules compiler
						string PossibleAssemblyName = TargetName;
						if (bIsEditorRecompile == true)
						{
							PossibleAssemblyName += "_EditorRecompile";
						}

						// Scan the disk to find source files for all known targets and modules, and generate "in memory" project
						// file data that will be used to determine what to build
						RulesCompiler.SetAssemblyNameAndGameFolders( PossibleAssemblyName, UEBuildTarget.DiscoverAllGameFolders() );
					}

					// Try getting it from the RulesCompiler
					UEBuildTarget Target = RulesCompiler.CreateTarget(
						TargetName:TargetName, 
						Target:new TargetInfo(Platform, Configuration),
						InAdditionalDefinitions:AdditionalDefinitions, 
						InRemoteRoot:RemoteRoot, 
						InOnlyModules:OnlyModules, 
						bInEditorRecompile:bIsEditorRecompile);
					if (Target == null)
					{
						if (UEBuildConfiguration.bCleanProject)
						{
							return null;
						}
						throw new BuildException( "Couldn't find target name {0}.", TargetName );
					}
					else
					{
						BuildTarget = Target;
						break;
					}
				}
				return BuildTarget;
			}
			throw new BuildException("No target name was specified on the command-line.");
		}

		/// Parses only the target platform and configuration from the specified command-line argument list
		public static void ParsePlatformAndConfiguration(string[] SourceArguments, 
			out UnrealTargetPlatform Platform, out UnrealTargetConfiguration Configuration,
			bool bThrowExceptionOnFailure = true)
		{
			Platform = UnrealTargetPlatform.Unknown;
			Configuration = UnrealTargetConfiguration.Unknown;

			foreach (var CurArgument in SourceArguments)
			{
				UnrealTargetPlatform ParsedPlatform = UEBuildPlatform.ConvertStringToPlatform(CurArgument);
				if (ParsedPlatform != UnrealTargetPlatform.Unknown)
				{
					Platform = ParsedPlatform;
				}
				else
				{
					switch (CurArgument.ToUpperInvariant())
					{
						// Configuration names:
						case "DEBUG":
							Configuration = UnrealTargetConfiguration.Debug;
							break;
						case "DEBUGGAME":
							Configuration = UnrealTargetConfiguration.DebugGame;
							break;
						case "DEVELOPMENT":
							Configuration = UnrealTargetConfiguration.Development;
							break;
						case "SHIPPING":
							Configuration = UnrealTargetConfiguration.Shipping;
							break;
						case "TEST":
							Configuration = UnrealTargetConfiguration.Test;
							break;

						case "-PROJECTFILES":
							// Force platform to Win64 and configuration to Development for building IntelliSense files
							Platform = UnrealTargetPlatform.Win64;
							Configuration = UnrealTargetConfiguration.Development;
							break;

						case "-XCODEPROJECTFILE":
							// @todo Mac: Don't want to force a platform/config for generated projects, in case they affect defines/includes (each project's individual configuration should be generated with correct settings)

							// Force platform to Mac and configuration to Development for building IntelliSense files
							Platform = UnrealTargetPlatform.Mac;
							Configuration = UnrealTargetConfiguration.Development;
							break;
					}
				}
			}

			if (bThrowExceptionOnFailure == true)
			{
				if (Platform == UnrealTargetPlatform.Unknown)
				{
					throw new BuildException("Couldn't find platform name.");
				}
				if (Configuration == UnrealTargetConfiguration.Unknown)
				{
					throw new BuildException("Couldn't determine configuration name.");
				}
			}
		}


		/** 
		 * Look for all folders ending in "Game" in the branch root that have a "Source" subfolder
		 * This is defined as a valid game
		 */
		public static List<string> DiscoverAllGameFolders()
		{
			List<string> AllGameFolders = new List<string>();

			// Add all the normal game folders. The UProjectInfo list is already filtered for projects specified on the command line.
			List<UProjectInfo> GameProjects = UProjectInfo.FilterGameProjects(true, null);
			foreach (UProjectInfo GameProject in GameProjects)
			{
				AllGameFolders.Add(GameProject.Folder);
			}

			// @todo: Perversely, if we're not running rocket we need to add the Rocket root folder so that the Rocket build automation script can be found...
			if (!UnrealBuildTool.RunningRocket())
			{
				string RocketFolder = "../../Rocket";
				if (Directory.Exists(RocketFolder))
				{
					AllGameFolders.Add(RocketFolder);
				}
			}

			return AllGameFolders;
		}

		/// <summary>
		/// 
		/// </summary>
		/// <param name="InGameName"></param>
		/// <param name="InRulesObject"></param>
		/// <param name="InPlatform"></param>
		/// <param name="InConfiguration"></param>
		/// <returns></returns>
		public static string GetBinaryBaseName(string InGameName, TargetRules InRulesObject, UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration, string InNameAppend)
		{
			//@todo. Allow failure to get build platform here?
			bool bPlatformRequiresMonolithic = false;
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(InPlatform, true);
			if (BuildPlatform != null)
			{
				bPlatformRequiresMonolithic = BuildPlatform.ShouldCompileMonolithicBinary(InPlatform);
			}

			if (InRulesObject.ShouldCompileMonolithic(InPlatform, InConfiguration) || bPlatformRequiresMonolithic || (InRulesObject.Type == TargetRules.TargetType.RocketGame))
			{
				return InGameName;
			}
			else
			{
				return "UE4" + InNameAppend;
			}
		}

		/** The target rules */
		public TargetRules Rules = null;

		/** The name of the application the target is part of. */
		public string AppName;

		/** The name of the game the target is part of - can be empty */
		public string GameName;

		/** Platform as defined by the VCProject and passed via the command line. Not the same as internal config names. */
		public UnrealTargetPlatform Platform;

		/** Target as defined by the VCProject and passed via the command line. Not necessarily the same as internal name. */
		public UnrealTargetConfiguration Configuration;

		/** Root directory for the active project. Typically contains the .uproject file, or the engine root. */
		public string ProjectDirectory;

		/** Default directory for intermediate files. Typically underneath ProjectDirectory. */
		public string ProjectIntermediateDirectory;

		/** Directory for engine intermediates. For an agnostic editor/game executable, this will be under the engine directory. For monolithic executables this will be the same as the project intermediate directory. */
		public string EngineIntermediateDirectory;

		/** Output path of final executable. */
		public string OutputPath;

		/** Remote path of the binary if it is to be synced with CookerSync */
		public string RemoteRoot;

		/** The C++ environment that all the environments used to compile UE-based modules are derived from. */
		public CPPEnvironment GlobalCompileEnvironment = new CPPEnvironment();

		/** The link environment all binary link environments are derived from. */
		public LinkEnvironment GlobalLinkEnvironment = new LinkEnvironment();

		/** All plugins enabled for this target */
		public List<PluginInfo> EnabledPlugins = new List<PluginInfo>();

		/** All application binaries; may include binaries not built by this target. */
		public List<UEBuildBinary> AppBinaries = new List<UEBuildBinary>();

		/** Extra engine module names to either include in the binary (monolithic) or create side-by-side DLLs for (modular) */
		public List<string> ExtraModuleNames = new List<string>();

		/** If building only a specific set of modules, these are the modules to build */
		protected List<OnlyModule> OnlyModules = new List<OnlyModule>();

		/** true if target should be compiled in monolithic mode, false if not */
		protected bool bCompileMonolithic = false;

		/** Used to keep track of all modules by name. */
		private Dictionary<string, UEBuildModule> Modules = new Dictionary<string, UEBuildModule>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// Whether this target should be compiled in monolithic mode
		/// </summary>
		/// <returns>true if it should, false if it shouldn't</returns>
		public bool ShouldCompileMonolithic()
		{
			return bCompileMonolithic;
		}

		/** 
		 * @param InAppName - The name of the application being built, which is used to scope all intermediate and output file names.
		 * @param InGameName - The name of the game being build - can be empty
		 * @param InPlatform - The platform the target is being built for.
		 * @param InConfiguration - The configuration the target is being built for.
		 * @param InAdditionalDefinitions - Additional definitions provided on the UBT command-line for the target.
		 * @param InRemoteRoot - The remote path that the build output is synced to.
		 */
		public UEBuildTarget(
			string InAppName,
			string InGameName,
			UnrealTargetPlatform InPlatform,
			UnrealTargetConfiguration InConfiguration,
			TargetRules InRulesObject,
			List<string> InAdditionalDefinitions,
			string InRemoteRoot,
			List<OnlyModule> InOnlyModules)
		{
			AppName = InAppName;
			GameName = InGameName;
			Platform = InPlatform;
			Configuration = InConfiguration;
			Rules = InRulesObject;

			{
				bCompileMonolithic = (Rules != null) ? Rules.ShouldCompileMonolithic(InPlatform, InConfiguration) : false;

				// Platforms may *require* monolithic compilation...
				bCompileMonolithic |= UEBuildPlatform.PlatformRequiresMonolithicBuilds(InPlatform, InConfiguration);

				// Force monolithic or modular mode if we were asked to
				if( UnrealBuildTool.CommandLineContains("-Monolithic") ||
					UnrealBuildTool.CommandLineContains("MONOLITHIC_BUILD=1") )
				{
					bCompileMonolithic = true;
				}
				else if( UnrealBuildTool.CommandLineContains( "-Modular" ) )
				{
					bCompileMonolithic = false;
				}
			}

			// Figure out what the project directory is. If we have a uproject file, use that. Otherwise use the engine directory.
			if (UnrealBuildTool.HasUProjectFile())
			{
				ProjectDirectory = Path.GetFullPath(UnrealBuildTool.GetUProjectPath());
			}
			else
			{
				ProjectDirectory = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);
			}

			// Build the project intermediate directory
			ProjectIntermediateDirectory = Path.GetFullPath(Path.Combine(ProjectDirectory, BuildConfiguration.PlatformIntermediateFolder, GetTargetName(), Configuration.ToString()));

			// Build the engine intermediate directory. If we're building agnostic engine binaries, we can use the engine intermediates folder. Otherwise we need to use the project intermediates directory.
			if (ShouldCompileMonolithic())
			{
				EngineIntermediateDirectory = ProjectIntermediateDirectory;
			}
			else if(Configuration == UnrealTargetConfiguration.DebugGame)
			{
				EngineIntermediateDirectory = Path.GetFullPath(Path.Combine(BuildConfiguration.RelativeEnginePath, BuildConfiguration.PlatformIntermediateFolder, AppName, UnrealTargetConfiguration.Development.ToString()));
			}
			else
			{
				EngineIntermediateDirectory = Path.GetFullPath(Path.Combine(BuildConfiguration.RelativeEnginePath, BuildConfiguration.PlatformIntermediateFolder, AppName, Configuration.ToString()));
			}

			RemoteRoot = InRemoteRoot;

			OnlyModules = InOnlyModules;

			bool bIsRocketGame = (InRulesObject != null) ? (InRulesObject.Type == TargetRules.TargetType.RocketGame) : false;

			// Construct the output path based on configuration, platform, game if not specified.
			TargetRules.TargetType? TargetType = (Rules != null) ? Rules.Type : (TargetRules.TargetType?)null;
			OutputPath = Path.GetFullPath(MakeBinaryPath("", AppName, UEBuildBinaryType.Executable, TargetType, bIsRocketGame, null, InAppName));

			if (bCompileMonolithic && TargetRules.IsGameType(InRulesObject.Type))
			{
				// For Rocket, UE4Game.exe and UE4Editor.exe still go into Engine/Binaries/<PLATFORM>
				if (!InRulesObject.bOutputToEngineBinaries)
				{
					// We are compiling a monolithic game...
					// We want the output to go into the <GAME>\Binaries folder
					if (UnrealBuildTool.HasUProjectFile() == false)
					{
						OutputPath = OutputPath.Replace(Path.Combine("Engine", "Binaries"), Path.Combine(InGameName,"Binaries"));
					}
					else
					{
						string EnginePath = Path.GetFullPath(Path.Combine(ProjectFileGenerator.EngineRelativePath, "Binaries"));
						string UProjectPath = UnrealBuildTool.GetUProjectPath();
						if (Path.IsPathRooted(UProjectPath) == false)
						{
							string FilePath = UProjectInfo.GetProjectForTarget(InGameName);
							string FullPath = Path.GetFullPath(FilePath);
							UProjectPath = Path.GetDirectoryName(FullPath);
						}
						string ProjectPath = Path.GetFullPath(Path.Combine(UProjectPath, "Binaries"));
						OutputPath = OutputPath.Replace(EnginePath, ProjectPath);
					}
				}
			}

			// handle some special case defines (so build system can pass -DEFINE as normal instead of needing
			// to know about special parameters)
			foreach (string Define in InAdditionalDefinitions)
			{
				switch (Define)
				{
					case "WITH_EDITOR=0":
						UEBuildConfiguration.bBuildEditor = false;
						break;

					case "WITH_EDITORONLY_DATA=0":
						UEBuildConfiguration.bBuildWithEditorOnlyData = false;
						break;

					// Memory profiler doesn't work if frame pointers are omitted
					case "USE_MALLOC_PROFILER=1":
						BuildConfiguration.bOmitFramePointers = false;
						break;

					case "WITH_LEAN_AND_MEAN_UE=1":
						UEBuildConfiguration.bCompileLeanAndMeanUE = true;
						break;
				}
			}

			// Add the definitions specified on the command-line.
			GlobalCompileEnvironment.Config.Definitions.AddRange(InAdditionalDefinitions);
		}

		/// <summary>
		/// Attempts to delete a file. Will retry a few times before failing.
		/// </summary>
		/// <param name="Filename"></param>
		void CleanFile(string Filename)
		{
			const int RetryDelayStep = 200;
			int RetryDelay = 1000;
			int RetryCount = 10;
			bool bResult = false;
			do
			{
				try
				{
					File.Delete(Filename);
					bResult = true;
				}
				catch (Exception Ex)
				{
					// This happens mostly because some other stale process is still locking this file
					Log.TraceVerbose(Ex.Message);
					if (--RetryCount < 0)
					{
						throw Ex;						
					}
					System.Threading.Thread.Sleep(RetryDelay);
					// Try with a slightly longer delay next time
					RetryDelay += RetryDelayStep;
				}
			}
			while (!bResult);
		}

		/// <summary>
		/// Attempts to delete a directory. Will retry a few times before failing.
		/// </summary>
		/// <param name="DirectoryPath"></param>
		void CleanDirectory(string DirectoryPath)
		{						
			const int RetryDelayStep = 200;
			int RetryDelay = 1000;
			int RetryCount = 10;
			bool bResult = false;
			do
			{
				try
				{
					Directory.Delete(DirectoryPath, true);
					bResult = true;
				}
				catch (Exception Ex)
				{
					// This happens mostly because some other stale process is still locking this file
					Log.TraceVerbose(Ex.Message);
					if (--RetryCount < 0)
					{
						throw Ex;
					}
					System.Threading.Thread.Sleep(RetryDelay);
					// Try with a slightly longer delay next time
					RetryDelay += RetryDelayStep;
				}
			}
			while (!bResult);
		}

		/// <summary>
		/// Cleans UnralHeaderTool
		/// </summary>
		private void CleanUnrealHeaderTool()
		{
			if (!UnrealBuildTool.RunningRocket())
			{
				var UBTArguments = new StringBuilder();

				UBTArguments.Append("UnrealHeaderTool");
				// Which desktop platform do we need to clean UHT for?
				var UHTPlatform = UnrealTargetPlatform.Win64;
				if (Utils.IsRunningOnMono)
				{
					UHTPlatform = UnrealTargetPlatform.Mac;
				}
				UBTArguments.Append(" " + UHTPlatform.ToString());
				UBTArguments.Append(" " + UnrealTargetConfiguration.Development.ToString());
				// NOTE: We disable mutex when launching UBT from within UBT to clean UHT
				UBTArguments.Append(" -NoMutex -Clean");

				ExternalExecution.RunExternalExecutable(UnrealBuildTool.GetUBTPath(), UBTArguments.ToString());
			}
		}

		/// <summary>
		/// Cleans all target intermediate files. May also clean UHT if the target uses UObjects.
		/// </summary>
		/// <param name="Binaries">Target binaries</param>
		/// <param name="Platform">Tareet platform</param>
		/// <param name="Manifest">Manifest</param>
		protected void CleanTarget(List<UEBuildBinary> Binaries, CPPTargetPlatform Platform, FileManifest Manifest)
		{
			if (Rules != null)
			{
				var TargetFilename = RulesCompiler.GetTargetFilename(GameName);
				var LocalTargetName = (Rules.Type == TargetRules.TargetType.Program) ? AppName : GameName;

				Log.TraceVerbose("Cleaning target {0} - AppName {1}", LocalTargetName, AppName);
				Log.TraceVerbose("\tTargetFilename {0}", TargetFilename);

				var TargetFolder = "";
				if (String.IsNullOrEmpty(TargetFilename) == false)
				{
					var TargetInfo = new FileInfo(TargetFilename);
					TargetFolder = TargetInfo.Directory.FullName;
					var SourceIdx = TargetFolder.LastIndexOf("\\Source");
					if (SourceIdx != -1)
					{
						TargetFolder = TargetFolder.Substring(0, SourceIdx + 1);
					}
				}

				// Collect all files to delete.
				var AdditionalFileExtensions = new string[] { ".lib", ".exp", ".dll.response" };
				var AllFilesToDelete = new List<string>(Manifest.FileManifestItems);
				foreach (var FileManifestItem in Manifest.FileManifestItems)
				{
					var FileExt = Path.GetExtension(FileManifestItem);
					if (FileExt == ".dll" || FileExt == ".exe")
					{
						var ManifestFileWithoutExtension = Utils.GetPathWithoutExtension(FileManifestItem);
						foreach (var AdditionalExt in AdditionalFileExtensions)
						{
							var AdditionalFileToDelete = ManifestFileWithoutExtension + AdditionalExt;
							AllFilesToDelete.Add(AdditionalFileToDelete);
						}
					}
				}

				//@todo. This does not clean up files that are no longer built by the target...				
				// Delete all output files listed in the manifest as well as any additional files.
				foreach (var FileToDelete in AllFilesToDelete)
				{
					if (File.Exists(FileToDelete))
					{
						Log.TraceVerbose("\t\tDeleting " + FileToDelete);
						CleanFile(FileToDelete);
					}
				}

				// Generate a list of all the modules of each AppBinaries entry
				var ModuleList = new List<string>();
				var bTargetUsesUObjectModule = false;
				foreach (var AppBin in AppBinaries)
				{
					UEBuildBinaryCPP AppBinCPP = AppBin as UEBuildBinaryCPP;
					if (AppBinCPP != null)
					{
						// Collect all modules used by this binary.
						Log.TraceVerbose("\tProcessing AppBinary " + AppBin.Config.OutputFilePath);
						foreach (string ModuleName in AppBinCPP.ModuleNames)
						{					
							if (ModuleList.Contains(ModuleName) == false)
							{
								Log.TraceVerbose("\t\tModule: " + ModuleName);
								ModuleList.Add(ModuleName);
								if (ModuleName == "CoreUObject")
								{
									bTargetUsesUObjectModule = true;
								}
							}
						}
					}
					else
					{
						Log.TraceVerbose("\t********* Skipping " + AppBin.Config.OutputFilePath);
					}
				}

				var BaseEngineBuildDataFolder = Path.GetFullPath(BuildConfiguration.BaseIntermediatePath).Replace("\\", "/");
				var PlatformEngineBuildDataFolder = BuildConfiguration.BaseIntermediatePath;
				var GameBuildDataFolder = (TargetFolder.Length > 0) ? Path.Combine(TargetFolder, BuildConfiguration.BaseIntermediateFolder) : "";

				// Delete generated header files
				foreach (var ModuleName in ModuleList)
				{
					var Module = GetModuleByName(ModuleName);
					var ModuleIncludeDir = UEBuildModuleCPP.GetGeneratedCodeDirectoryForModule(this, Module.ModuleDirectory, ModuleName).Replace("\\", "/");
					if (!UnrealBuildTool.RunningRocket() || !ModuleIncludeDir.StartsWith(BaseEngineBuildDataFolder, StringComparison.InvariantCultureIgnoreCase))
					{
						if (Directory.Exists(ModuleIncludeDir))
						{
							Log.TraceVerbose("\t\tDeleting " + ModuleIncludeDir);
							CleanDirectory(ModuleIncludeDir);
						}
					}
				}

				//
				{
					var AppEnginePath = Path.Combine(PlatformEngineBuildDataFolder, LocalTargetName, Configuration.ToString());
					if (Directory.Exists(AppEnginePath))
					{
						CleanDirectory(AppEnginePath);
					}
				}

				// Clean the intermediate directory
				if (!UnrealBuildTool.RunningRocket())
				{
					// This is always under Rocket installation folder
					if (Directory.Exists(GlobalLinkEnvironment.Config.IntermediateDirectory))
					{
						Log.TraceVerbose("\tDeleting " + GlobalLinkEnvironment.Config.IntermediateDirectory);
						CleanDirectory(GlobalLinkEnvironment.Config.IntermediateDirectory);
					}
				}
				else if (ShouldCompileMonolithic())
				{
					// Only in monolithic, otherwise it's pointing to Rocket installation folder
					if (Directory.Exists(GlobalLinkEnvironment.Config.OutputDirectory))
					{
						Log.TraceVerbose("\tDeleting " + GlobalLinkEnvironment.Config.OutputDirectory);
						CleanDirectory(GlobalCompileEnvironment.Config.OutputDirectory);
					}
				}

				// Delete the dependency cache
				{
					var DependencyCacheFilename = DependencyCache.GetDependencyCachePathForTarget(this);
					if (File.Exists(DependencyCacheFilename))
					{
						Log.TraceVerbose("\tDeleting " + DependencyCacheFilename);
						CleanFile(DependencyCacheFilename);
					}
				}

				// Delete the action history
				{
					var ActionHistoryFilename = ActionHistory.GeneratePathForTarget(this);
					if (File.Exists(ActionHistoryFilename))
					{
						Log.TraceVerbose("\tDeleting " + ActionHistoryFilename);
						CleanFile(ActionHistoryFilename);
					}
				}

				// Finally clean UnrealHeaderTool if this target uses CoreUObject modules and we're not cleaning UHT already
				// and we want UHT to be cleaned.
				if (!UEBuildConfiguration.bDoNotBuildUHT && bTargetUsesUObjectModule && GetTargetName() != "UnrealHeaderTool")
				{
					CleanUnrealHeaderTool();
				}
			}
			else
			{
				Log.TraceVerbose("Cannot clean target with no Rules object: {0}", GameName);
			}
		}

		/** Generates a public manifest file for writing out */
		public void GenerateManifest( List<UEBuildBinary> Binaries, CPPTargetPlatform Platform )
		{
			string ManifestPath;
			if (UnrealBuildTool.RunningRocket())
			{
				ManifestPath = Path.Combine(UnrealBuildTool.GetUProjectPath(), BuildConfiguration.BaseIntermediateFolder, "Manifest.xml");
			}
			else
			{
				ManifestPath = "../Intermediate/Build/Manifest.xml";
			}

			FileManifest Manifest = new FileManifest();
			if (UEBuildConfiguration.bMergeManifests)
			{
				// Load in existing manifest (if any)
				Manifest = Utils.ReadClass<FileManifest>(ManifestPath);
			}

			UnrealTargetPlatform TargetPlatform = CPPTargetPlatformToUnrealTargetPlatform( Platform );
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform( TargetPlatform );

			// Iterate over all the binaries, and add the relevant info to the manifest
			foreach( UEBuildBinary Binary in Binaries )
			{
				// Get the platform specific extension for debug info files
				string DebugInfoExtension = BuildPlatform.GetDebugInfoExtension( Binary.Config.Type );

				// Don't add static library files to the manifest as we do not check them into perforce.
				// However, add them to the manifest when cleaning the project as we do want to delete 
				// them in that case.
				if (UEBuildConfiguration.bCleanProject == false)
				{
					if (Binary.Config.Type == UEBuildBinaryType.StaticLibrary)
					{
						continue;
					}
				}
				// Create and add the binary and associated debug info
				Manifest.AddBinaryNames(Binary.Config.OutputFilePath, DebugInfoExtension);
				if (Binary.Config.Type == UEBuildBinaryType.Executable && 
					  GlobalLinkEnvironment.Config.CanProduceAdditionalConsoleApp &&
					  UEBuildConfiguration.bBuildEditor)
				{
					Manifest.AddBinaryNames(UEBuildBinary.GetAdditionalConsoleAppPath(Binary.Config.OutputFilePath), DebugInfoExtension);
				}

				if (TargetPlatform == UnrealTargetPlatform.Mac)
				{
					// Add all the resources and third party dylibs stored in app bundle
					MacToolChain.AddAppBundleContentsToManifest(ref Manifest, Binary);
				}
			}

			if (UEBuildConfiguration.bCleanProject)
			{
				CleanTarget(Binaries, Platform, Manifest);
			}
			if (UEBuildConfiguration.bGenerateManifest)
			{
				Utils.WriteClass<FileManifest>(Manifest, ManifestPath, "");
			}
		}

		/** Builds the target, returning a list of output files. */
		public List<FileItem> Build()
		{
			// Set up the global compile and link environment in GlobalCompileEnvironment and GlobalLinkEnvironment.
			SetupGlobalEnvironment();

			// Setup the target's modules.
			SetupModules();

			// Setup the target's binaries.
			SetupBinaries();

			// Setup the target's plugins
			SetupPlugins();

			// Add the enabled plugins to the build
			foreach (PluginInfo Plugin in EnabledPlugins)
			{
				AddPlugin(Plugin);
			}

			// Describe what's being built.
			Log.TraceVerbose("Building {0} - {1} - {2} - {3}", AppName, GameName, Platform, Configuration);

			// Put the non-executable output files (PDB, import library, etc) in the intermediate directory.
			GlobalLinkEnvironment.Config.IntermediateDirectory = Path.GetFullPath(GlobalCompileEnvironment.Config.OutputDirectory);
			GlobalLinkEnvironment.Config.OutputDirectory = GlobalLinkEnvironment.Config.IntermediateDirectory;

			// By default, shadow source files for this target in the root OutputDirectory
			GlobalLinkEnvironment.Config.LocalShadowDirectory = GlobalLinkEnvironment.Config.OutputDirectory;

			// Add all of the extra modules, including game modules, that need to be compiled along
			// with this app.  These modules aren't necessarily statically linked against, but may
			// still be required at runtime in order for the application to load and function properly!
			AddExtraModules();

			// Bind modules for all app binaries.
			foreach (var Binary in AppBinaries)
			{
				Binary.BindModules();
			}

			// Process all referenced modules and create new binaries for DLL dependencies if needed
			var NewBinaries = new List<UEBuildBinary>();
			foreach (var Binary in AppBinaries)
			{
				// Add binaries for all of our dependent modules
				var FoundBinaries = Binary.ProcessUnboundModules();
				if (FoundBinaries != null)
				{
					NewBinaries.AddRange(FoundBinaries);
				}
			}
			AppBinaries.AddRange( NewBinaries );

			// If we're building a single module, then find the binary for that module and add it to our target
			if (OnlyModules.Count > 0)
			{
				var FilteredBinaries = new List<UEBuildBinary>();

				var AnyBinariesAdded = false;
				foreach (var DLLBinary in AppBinaries)
				{
					var FoundOnlyModule = DLLBinary.FindOnlyModule(OnlyModules);
					if (FoundOnlyModule != null)
					{
						FilteredBinaries.Add( DLLBinary );
						AnyBinariesAdded = true;

						if (!String.IsNullOrEmpty(FoundOnlyModule.OnlyModuleSuffix))
						{
							var Appendage = "-" + FoundOnlyModule.OnlyModuleSuffix;

							var MatchPos = DLLBinary.Config.OutputFilePath.LastIndexOf(FoundOnlyModule.OnlyModuleName, StringComparison.InvariantCultureIgnoreCase);
							if (MatchPos < 0)
							{
								throw new BuildException("Failed to find module name \"{0}\" specified on the command line inside of the output filename \"{1}\" to add appendage.", FoundOnlyModule.OnlyModuleName, DLLBinary.Config.OutputFilePath);
							}
							DLLBinary.Config.OriginalOutputFilePath = DLLBinary.Config.OutputFilePath;
							DLLBinary.Config.OutputFilePath = DLLBinary.Config.OutputFilePath.Insert(MatchPos + FoundOnlyModule.OnlyModuleName.Length, Appendage);
						}
					}
				}

				// Copy the result into the final list
				AppBinaries = FilteredBinaries;

				if (!AnyBinariesAdded)
				{
					throw new BuildException("One or more of the modules specified using the '-module' argument could not be found.");
				}
			}

			// Filter out binaries that were already built and are just used for linking. We will not build these binaries but we need them for link information
			{
				var FilteredBinaries = new List<UEBuildBinary>();

				foreach (var DLLBinary in AppBinaries)
				{
					if ( DLLBinary.Config.bAllowCompilation )
					{
						FilteredBinaries.Add(DLLBinary);
					}
				}

				// Copy the result into the final list
				AppBinaries = FilteredBinaries;

				if (AppBinaries.Count == 0)
				{
					throw new BuildException("No modules found to build. All requested binaries were already built.");
				}
			}

            // If we are only interested in platform specific binaries, filter everything else out now
            if (UnrealBuildTool.GetOnlyPlatformSpecificFor() != UnrealTargetPlatform.Unknown)
            {
                var FilteredBinaries = new List<UEBuildBinary>();

                var OtherThingsWeNeedToBuild = new List<OnlyModule>();

                foreach (var DLLBinary in AppBinaries)
                {
                    if (DLLBinary.Config.bIsCrossTarget)
                    {
                        FilteredBinaries.Add(DLLBinary);
                        bool bIncludeDynamicallyLoaded = false;
                        var AllReferencedModules = DLLBinary.GetAllDependencyModules(bIncludeDynamicallyLoaded, bForceCircular: true);
                        foreach (var Other in AllReferencedModules)
                        {
                            OtherThingsWeNeedToBuild.Add(new OnlyModule(Other.Name));
                        }
                    }
                }
                foreach (var DLLBinary in AppBinaries)
                {
                    if (!FilteredBinaries.Contains(DLLBinary) && DLLBinary.FindOnlyModule(OtherThingsWeNeedToBuild) != null)
                    {
                        if (!File.Exists(DLLBinary.Config.OutputFilePath))
                        {
                            throw new BuildException("Module {0} is potentially needed for the {1} platform to work, but it isn't actually compiled. This either needs to be marked as platform specific or made a dynamic dependency of something compiled with the base editor.", DLLBinary.Config.OutputFilePath, UnrealBuildTool.GetOnlyPlatformSpecificFor().ToString());
                        }
                    }
                }

                // Copy the result into the final list
                AppBinaries = FilteredBinaries;
            }

			//@todo.Rocket: Will users be able to rebuild UnrealHeaderTool? NO
			if (UnrealBuildTool.RunningRocket() && AppName != "UnrealHeaderTool")
			{
				var FilteredBinaries = new List<UEBuildBinary>();

				// Have to do absolute here as this could be a project that is under the root
				string FullUProjectPath = Path.GetFullPath(UnrealBuildTool.GetUProjectPath());

				// We only want to build rocket projects...
				foreach (var DLLBinary in AppBinaries)
				{
					if (Utils.IsFileUnderDirectory( DLLBinary.Config.OutputFilePath, FullUProjectPath ))
					{
						FilteredBinaries.Add(DLLBinary);
					}
				}

				// Copy the result into the final list
				AppBinaries = FilteredBinaries;

				if (AppBinaries.Count == 0)
				{
					throw new BuildException("Rocket: No modules found to build?");
				}
			}

			// If we're compiling monolithic, make sure the executable knows about all referenced modules
			if (ShouldCompileMonolithic())
			{
				var ExecutableBinary = AppBinaries[0];
				bool bIncludeDynamicallyLoaded = true;
				var AllReferencedModules = ExecutableBinary.GetAllDependencyModules( bIncludeDynamicallyLoaded, bForceCircular:true );
				foreach (var CurModule in AllReferencedModules)
				{
					if ( CurModule.Binary == null || CurModule.Binary == ExecutableBinary )
					{
						ExecutableBinary.AddModule(CurModule.Name);
					}
				}

                bool IsCurrentPlatform;
                if (Utils.IsRunningOnMono)
                {
                    IsCurrentPlatform = Platform == UnrealTargetPlatform.Mac;
                }
                else
                {
                    IsCurrentPlatform = Platform == UnrealTargetPlatform.Win64 || Platform == UnrealTargetPlatform.Win32;

                }

                if (TargetRules.IsAGame(Rules.Type) && IsCurrentPlatform)
				{
					// The hardcoded engine directory needs to be a relative path to match the normal EngineDir format. Not doing so breaks the network file system (TTP#315861).
					string EnginePath = Utils.CleanDirectorySeparators(Utils.MakePathRelativeTo(ProjectFileGenerator.EngineRelativePath, Path.GetDirectoryName(ExecutableBinary.Config.OutputFilePath)), '/');
					if (EnginePath.EndsWith("/") == false)
					{
						EnginePath += "/";
					}
					GlobalCompileEnvironment.Config.Definitions.Add("UE_ENGINE_DIRECTORY=" + EnginePath);
				}

				// Generate static libraries for monolithic games in Rocket
				if ((UnrealBuildTool.BuildingRocket() || UnrealBuildTool.RunningRocket()) && TargetRules.IsAGame(Rules.Type))
				{
					List<UEBuildModule> Modules = ExecutableBinary.GetAllDependencyModules(true, false);
					foreach (UEBuildModuleCPP Module in Modules.OfType<UEBuildModuleCPP>())
					{
						if(Utils.IsFileUnderDirectory(Module.ModuleDirectory, BuildConfiguration.RelativeEnginePath) && Module.Binary == ExecutableBinary)
						{
							UnrealTargetConfiguration LibraryConfiguration = (Configuration == UnrealTargetConfiguration.DebugGame)? UnrealTargetConfiguration.Development : Configuration;
							Module.RedistStaticLibraryPath = MakeBinaryPath("", "UE4Game-Redist-" + Module.Name, Platform, LibraryConfiguration, UEBuildBinaryType.StaticLibrary, Rules.Type, false, null, AppName);
							Module.bBuildingRedistStaticLibrary = UnrealBuildTool.BuildingRocket();
						}
					}
				}
			}

			if (ShouldCompileMonolithic() && !ProjectFileGenerator.bGenerateProjectFiles && Rules != null && Rules.Type != TargetRules.TargetType.Program)
			{
				// All non-program monolithic binaries implicitly depend on all static plugin libraries so they are always linked appropriately
				// In order to do this, we create a new module here with a cpp file we emit that invokes an empty function in each library.
				// If we do not do this, there will be no static initialization for libs if no symbols are referenced in them.
				CreateLinkerFixupsCPPFile();
			}

			// On Mac we have actions that should be executed after all the binaries are created
			if (GlobalLinkEnvironment.Config.TargetPlatform == CPPTargetPlatform.Mac)
			{
				// Also, for non-console targets, AppBinaries paths need to be adjusted to be inside the app bundle
				if (!GlobalLinkEnvironment.Config.bIsBuildingConsoleApplication)
				{
					MacToolChain.FixBundleBinariesPaths( this, AppBinaries );
				}

				MacToolChain.SetupBundleDependencies(AppBinaries, GameName);
			}

			// If we're only generating the manifest, return now
			if (UEBuildConfiguration.bGenerateManifest || UEBuildConfiguration.bCleanProject)
			{
				GenerateManifest( AppBinaries, GlobalLinkEnvironment.Config.TargetPlatform );
                if (!BuildConfiguration.bXGEExport)
                {
                    return new List<FileItem>();
                }
			}

			// We don't need to worry about shared PCH headers when only generating project files.  This is
			// just an optimization
			if( !ProjectFileGenerator.bGenerateProjectFiles )
			{
				var SharedPCHHeaderFiles = FindSharedPCHHeaders();
				GlobalCompileEnvironment.SharedPCHHeaderFiles = SharedPCHHeaderFiles;
			}

			// Build the target's binaries.
			var OutputItems = new List<FileItem>();
			foreach (var Binary in AppBinaries)
			{
				OutputItems.AddRange(Binary.Build(GlobalCompileEnvironment, GlobalLinkEnvironment));
			}

			if( (BuildConfiguration.bXGEExport && UEBuildConfiguration.bGenerateManifest) || (!UEBuildConfiguration.bGenerateManifest && !UEBuildConfiguration.bCleanProject && !ProjectFileGenerator.bGenerateProjectFiles) )
			{
				var UObjectModules = new List<UHTModuleInfo>();

				// Figure out which modules have UObjects that we may need to generate headers for
				{
					foreach( var Binary in AppBinaries )
					{
						var DependencyModules = Binary.GetAllDependencyModules(bIncludeDynamicallyLoaded: false, bForceCircular: false);
						foreach( var DependencyModule in DependencyModules )
						{
							if( !DependencyModule.bIncludedInTarget )
							{
								throw new BuildException( "Expecting module {0} to have bIncludeInTarget set", DependencyModule.Name );
							}

							UEBuildModuleCPP DependencyModuleCPP = DependencyModule as UEBuildModuleCPP;
							if( DependencyModuleCPP != null )
							{
								// Do we already have this module?
								if( !UObjectModules.Any(Module => Module.ModuleName == DependencyModuleCPP.Name ) )
								{
									var UHTModuleInfo = DependencyModuleCPP.GetUHTModuleInfo();
									if( UHTModuleInfo.PublicUObjectClassesHeaders.Count > 0 || UHTModuleInfo.PrivateUObjectHeaders.Count > 0 || UHTModuleInfo.PublicUObjectHeaders.Count > 0 )
									{
										UObjectModules.Add( UHTModuleInfo );
										Log.TraceVerbose( "Detected UObject module: " + DependencyModuleCPP.Name );
									}
								}
								else
								{
									// This module doesn't define any UObjects
								}
							}
							else
							{
								// Non-C++ modules never have UObjects
							}
						}
					}
				}

				if( UObjectModules.Count > 0 )
				{
					// Execute the header tool
					var TargetName = !String.IsNullOrEmpty( GameName ) ? GameName : AppName;
					string ModuleInfoFileName = Path.GetFullPath( Path.Combine( ProjectIntermediateDirectory, "UnrealHeaderTool.manifest" ) );
					if (!ExternalExecution.ExecuteHeaderToolIfNecessary(this, UObjectModules, ModuleInfoFileName))
					{
						throw new BuildException( "UnrealHeaderTool failed for target '" + TargetName + "' (platform: " + Platform.ToString() + ", module info: " + ModuleInfoFileName + ")." );
					}
				}

			}

			if (BuildConfiguration.WriteTargetInfoPath != null)
			{
				Log.TraceInformation("Writing build environment to " + BuildConfiguration.WriteTargetInfoPath + "...");

				XmlWriterSettings Settings = new XmlWriterSettings();
				Settings.Indent = true;

				using (XmlWriter Writer = XmlWriter.Create(BuildConfiguration.WriteTargetInfoPath, Settings))
				{
					Writer.WriteStartElement("target");
					Writer.WriteAttributeString("name", AppName);
					foreach (UEBuildBinary Binary in AppBinaries)
					{
						Binary.WriteBuildEnvironment(GlobalCompileEnvironment, GlobalLinkEnvironment, Writer);
					}
					Writer.WriteEndElement();
				}
				return new List<FileItem>();
			}

			return OutputItems;
		}

		/// <summary>
		/// All non-program monolithic binaries implicitly depend on all static plugin libraries so they are always linked appropriately
		/// In order to do this, we create a new module here with a cpp file we emit that invokes an empty function in each library.
		/// If we do not do this, there will be no static initialization for libs if no symbols are referenced in them.
		/// </summary>
		private void CreateLinkerFixupsCPPFile()
		{
			var ExecutableBinary = AppBinaries[0];

			List<string> PrivateDependencyModuleNames = new List<string>();

			UEBuildBinaryCPP BinaryCPP = ExecutableBinary as UEBuildBinaryCPP;
			if (BinaryCPP != null)
			{
				TargetInfo CurrentTarget = new TargetInfo(Platform, Configuration, (Rules != null) ? Rules.Type : (TargetRules.TargetType?)null);
				foreach (var TargetModuleName in BinaryCPP.ModuleNames)
				{
					string UnusedFilename;
					ModuleRules CheckRules = RulesCompiler.CreateModuleRules(TargetModuleName, CurrentTarget, out UnusedFilename);
					if ((CheckRules != null) && (CheckRules.Type != ModuleRules.ModuleType.External))
					{
						PrivateDependencyModuleNames.Add(TargetModuleName);
					}
				}
			}
			foreach (PluginInfo Plugin in EnabledPlugins)
			{
				foreach (PluginInfo.PluginModuleInfo Module in Plugin.Modules)
				{
					if(ShouldIncludePluginModule(Plugin, Module))
					{
						PrivateDependencyModuleNames.Add(Module.Name);
					}
				}
			}

			// We ALWAYS have to write this file as the IMPLEMENT_PRIMARY_GAME_MODULE function depends on the UELinkerFixups function existing.
			{
				List<string> LinkerFixupsFileContents = new List<string>();

				string LinkerFixupsName = "UELinkerFixups";

				// Include an empty header so UEBuildModule.Compile does not complain about a lack of PCH
				string HeaderFilename = LinkerFixupsName + "Name.h";
				LinkerFixupsFileContents.Add("#include \"" + HeaderFilename + "\"");

				// Add a function that is not referenced by anything that invokes all the empty functions in the different static libraries
				LinkerFixupsFileContents.Add("void " + LinkerFixupsName + "()");
				LinkerFixupsFileContents.Add("{");

				// Fill out the body of the function with the empty function calls. This is what causes the static libraries to be considered relevant
				foreach (var DependencyModuleName in PrivateDependencyModuleNames)
				{
					LinkerFixupsFileContents.Add("    extern void EmptyLinkFunctionForStaticInitialization" + DependencyModuleName + "();");
					LinkerFixupsFileContents.Add("    EmptyLinkFunctionForStaticInitialization" + DependencyModuleName + "();");
				}

				// End the function body that was started above
				LinkerFixupsFileContents.Add("}");

				// Create the cpp file
				string LinkerFixupCPPFilename = Path.Combine(GlobalCompileEnvironment.Config.OutputDirectory, LinkerFixupsName + ".cpp");

				// Determine if the file changed. Write it if it either doesn't exist or the contents are different.
				bool bShouldWriteFile = true;
				if (File.Exists(LinkerFixupCPPFilename))
				{
					string[] ExistingFixupText = File.ReadAllLines(LinkerFixupCPPFilename);
					string JoinedNewContents = string.Join("", LinkerFixupsFileContents.ToArray());
					string JoinedOldContents = string.Join("", ExistingFixupText);
					bShouldWriteFile = (JoinedNewContents != JoinedOldContents);
				}

				// If we determined that we should write the file, write it now.
				if (bShouldWriteFile)
				{
					string LinkerFixupHeaderFilenameWithPath = Path.Combine(GlobalCompileEnvironment.Config.OutputDirectory, HeaderFilename);
					ResponseFile.Create(LinkerFixupHeaderFilenameWithPath, new List<string>());
					ResponseFile.Create(LinkerFixupCPPFilename, LinkerFixupsFileContents);
				}

				// Create the source file list (just the one cpp file)
				List<FileItem> SourceFiles = new List<FileItem>();
				SourceFiles.Add(FileItem.GetItemByPath(LinkerFixupCPPFilename));

				// Create the CPP module
				var FakeModuleDirectory = Path.GetDirectoryName( LinkerFixupCPPFilename );
				UEBuildModuleCPP NewModule = new UEBuildModuleCPP(
					InTarget: this,
					InName: LinkerFixupsName,
					InType: UEBuildModuleType.GameModule,
					InModuleDirectory:FakeModuleDirectory,
					InOutputDirectory: GlobalCompileEnvironment.Config.OutputDirectory,
					InIntelliSenseGatherer: null,
					InSourceFiles: SourceFiles,
					InPublicIncludePaths: new List<string>(),
					InPublicSystemIncludePaths: new List<string>(),
					InDefinitions: new List<string>(),
					InPublicIncludePathModuleNames: new List<string>(),
					InPublicDependencyModuleNames: new List<string>(),
					InPublicDelayLoadDLLs: new List<string>(),
					InPublicAdditionalLibraries: new List<string>(),
					InPublicFrameworks: new List<string>(),
					InPublicAdditionalFrameworks: new List<string>(),
					InPrivateIncludePaths: new List<string>(),
					InPrivateIncludePathModuleNames: new List<string>(),
					InPrivateDependencyModuleNames: PrivateDependencyModuleNames,
					InCircularlyReferencedDependentModules: new List<string>(),
					InDynamicallyLoadedModuleNames: new List<string>(),
                    InPlatformSpecificDynamicallyLoadedModuleNames: new List<string>(),
                    InOptimizeCode: ModuleRules.CodeOptimization.Default,
					InAllowSharedPCH: false,
					InSharedPCHHeaderFile: "",
					InUseRTTI: false,
					InEnableBufferSecurityChecks: true,
					InFasterWithoutUnity: true,
					InMinFilesUsingPrecompiledHeaderOverride: 0,
					InEnableExceptions: false,
					InEnableInlining: true
					);

				// Now bind this new module to the executable binary so it will link the plugin libs correctly
				NewModule.Binary = ExecutableBinary;
				NewModule.bIncludedInTarget = true;
				ExecutableBinary.AddModule(NewModule.Name);
			}
		}


		/// <summary>
		/// For any dependent module that has "SharedPCHHeaderFile" set in its module rules, we gather these headers
		/// and sort them in order from least-dependent to most-dependent such that larger, more complex PCH headers
		/// appear last in the list
		/// </summary>
		/// <returns>List of shared PCH headers to use</returns>
		private List<SharedPCHHeaderInfo> FindSharedPCHHeaders()
		{
			// List of modules, with all of the dependencies of that module
			var SharedPCHHeaderFiles = new List<SharedPCHHeaderInfo>();

			// Build up our list of modules with "shared PCH headers".  The list will be in dependency order, with modules
			// that depend on previous modules appearing later in the list
			foreach( var Binary in AppBinaries )
			{
				var CPPBinary = Binary as UEBuildBinaryCPP;
				if( CPPBinary != null )
				{
					foreach( var ModuleName in CPPBinary.ModuleNames )
					{
						var CPPModule = GetModuleByName( ModuleName ) as UEBuildModuleCPP;
						if( CPPModule != null )
						{
							if( !String.IsNullOrEmpty( CPPModule.SharedPCHHeaderFile ) )
							{
								// @todo SharedPCH: Ideally we could figure the PCH header name automatically, and simply use a boolean in the module
								//     definition to opt into exposing a shared PCH.  Unfortunately we don't determine which private PCH header "goes with"
								//     a module until a bit later in the process.  It shouldn't be hard to change that though.
								var SharedPCHHeaderFilePath = ProjectFileGenerator.RootRelativePath + "/Engine/Source/" + CPPModule.SharedPCHHeaderFile;
								var SharedPCHHeaderFileItem = FileItem.GetExistingItemByPath( SharedPCHHeaderFilePath );
								if( SharedPCHHeaderFileItem != null )
								{
									var ModuleDependencies = new Dictionary<string, UEBuildModule>();
									var ModuleList = new List<UEBuildModule>();
									bool bIncludeDynamicallyLoaded = false;
									CPPModule.GetAllDependencyModules(ref ModuleDependencies, ref ModuleList, bIncludeDynamicallyLoaded, bForceCircular: false);

									// Figure out where to insert the shared PCH into our list, based off the module dependency ordering
									int InsertAtIndex = SharedPCHHeaderFiles.Count;
									for( var ExistingModuleIndex = SharedPCHHeaderFiles.Count - 1; ExistingModuleIndex >= 0; --ExistingModuleIndex )
									{
										var ExistingModule = SharedPCHHeaderFiles[ ExistingModuleIndex ].Module;
										var ExistingModuleDependencies = SharedPCHHeaderFiles[ ExistingModuleIndex ].Dependencies;

										// If the module to add to the list is dependent on any modules already in our header list, that module
										// must be inserted after any of those dependencies in the list
										foreach( var ExistingModuleDependency in ExistingModuleDependencies )
										{
											if( ExistingModuleDependency.Value == CPPModule )
											{
												// Make sure we're not a circular dependency of this module.  Circular dependencies always
												// point "upstream".  That is, modules like Engine point to UnrealEd in their
												// CircularlyReferencedDependentModules array, but the natural dependency order is
												// that UnrealEd depends on Engine.  We use this to avoid having modules such as UnrealEd
												// appear before Engine in our shared PCH list.
												// @todo SharedPCH: This is not very easy for people to discover.  Luckily we won't have many shared PCHs in total.
												if( !ExistingModule.CircularlyReferencedDependentModules.Contains( CPPModule.Name ) )
												{
													// We are at least dependent on this module.  We'll keep searching the list to find
													// further-descendant modules we might be dependent on
													InsertAtIndex = ExistingModuleIndex;
													break;
												}
											}
										}
									}

									var NewSharedPCHHeaderInfo = new SharedPCHHeaderInfo();
									NewSharedPCHHeaderInfo.PCHHeaderFile = SharedPCHHeaderFileItem;
									NewSharedPCHHeaderInfo.Module = CPPModule;
									NewSharedPCHHeaderInfo.Dependencies = ModuleDependencies;
									SharedPCHHeaderFiles.Insert( InsertAtIndex, NewSharedPCHHeaderInfo );
								}
								else
								{
									throw new BuildException( "Could not locate the specified SharedPCH header file '{0}' for module '{1}'.", SharedPCHHeaderFilePath, CPPModule.Name );
								}
							}
						}
					}
				}
			}

			if( SharedPCHHeaderFiles.Count > 0 )
			{
				Log.TraceVerbose("Detected {0} shared PCH headers (listed in dependency order):", SharedPCHHeaderFiles.Count);
				foreach( var CurSharedPCH in SharedPCHHeaderFiles )
				{
					Log.TraceVerbose("	" + CurSharedPCH.PCHHeaderFile.AbsolutePath + "  (module: " + CurSharedPCH.Module.Name + ")");
				}
			}
			else
			{
				Log.TraceVerbose("Did not detect any shared PCH headers");
			}
			return SharedPCHHeaderFiles;
		}

		/// <summary>
		/// Include the given plugin in the target. It may be included as a separate binary, or compiled into a monolithic executable.
		/// </summary>
		public void AddPlugin(PluginInfo Plugin)
		{
			foreach(PluginInfo.PluginModuleInfo Module in Plugin.Modules)
			{
				if (ShouldIncludePluginModule(Plugin, Module))
				{
					AddPluginModule(Plugin, Module);
				}
			}
		}

		/// <summary>
		/// Include the given plugin module in the target. Will be built in the appropriate subfolder under the plugin directory.
		/// </summary>
		public void AddPluginModule(PluginInfo Plugin, PluginInfo.PluginModuleInfo Module)
		{
			// Is this a Rocket module?
			bool bIsRocketModule = RulesCompiler.IsRocketProjectModule(Module.Name);

			// Get the binary type to build
			UEBuildBinaryType BinaryType = ShouldCompileMonolithic()? UEBuildBinaryType.StaticLibrary : UEBuildBinaryType.DynamicLinkLibrary;

			// Make the plugin intermediate path
			string IntermediateDirectory = Path.Combine(Plugin.Directory, BuildConfiguration.PlatformIntermediateFolder, Plugins.GetPluginSubfolderName(BinaryType, AppName), Configuration.ToString());

			// Get the output path. Don't prefix the app name for Rocket
			string OutputFilePath;
			if ((UnrealBuildTool.BuildingRocket() || UnrealBuildTool.RunningRocket()) && BinaryType == UEBuildBinaryType.StaticLibrary)
			{
				OutputFilePath = MakeBinaryPath(Module.Name, Module.Name, BinaryType, Rules.Type, bIsRocketModule, Plugin, AppName);
			}
			else
			{
				OutputFilePath = MakeBinaryPath(Module.Name, GetAppName() + "-" + Module.Name, BinaryType, Rules.Type, bIsRocketModule, Plugin, AppName);
			}

			// Figure out whether we should build it from source
			string ModuleSourceFolder = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(Module.Name));
			bool bShouldBeBuiltFromSource = Directory.GetFiles(ModuleSourceFolder, "*.cpp", SearchOption.AllDirectories).Length > 0;

			// Create the binary
			UEBuildBinaryConfiguration Config = new UEBuildBinaryConfiguration( InType: BinaryType, 
																				InOutputFilePath: OutputFilePath,
																				InIntermediateDirectory: IntermediateDirectory,
																				bInAllowExports: true,
																				bInAllowCompilation: bShouldBeBuiltFromSource,
																				InModuleNames: new List<string> { Module.Name } );
			AppBinaries.Add(new UEBuildBinaryCPP(this, Config));
		}

		/// When building a target, this is called to add any additional modules that should be compiled along
		/// with the main target.  If you override this in a derived class, remember to call the base implementation!
		protected virtual void AddExtraModules()
		{
			// Add extra modules that will either link into the main binary (monolithic), or be linked into separate DLL files (modular)
			foreach (var ModuleName in ExtraModuleNames)
			{
				if (ShouldCompileMonolithic())
				{
					// Add this module to the executable's list of included modules
					var ExecutableBinary = AppBinaries[0];
					ExecutableBinary.AddModule(ModuleName);
				}
				else
				{
					// Is this a Rocket module?
					bool bIsRocketModule = RulesCompiler.IsRocketProjectModule(ModuleName);

					// Create a DLL binary for this module
					string OutputFilePath = MakeBinaryPath(ModuleName, GetAppName() + "-" + ModuleName, UEBuildBinaryType.DynamicLinkLibrary, Rules.Type, bIsRocketModule, null, AppName);
					UEBuildBinaryConfiguration Config = new UEBuildBinaryConfiguration( InType: UEBuildBinaryType.DynamicLinkLibrary,
																						InOutputFilePath: OutputFilePath,
																						InIntermediateDirectory: RulesCompiler.IsGameModule(ModuleName)? ProjectIntermediateDirectory : EngineIntermediateDirectory,
																						bInAllowExports: true,
																						InModuleNames: new List<string> { ModuleName } );

					// Tell the target about this new binary
					AppBinaries.Add( new UEBuildBinaryCPP( this, Config ) );
				}
			}
		}


		/**
		 * @return true if debug information is created, false otherwise.
		 */
		public bool IsCreatingDebugInfo()
		{
			return GlobalCompileEnvironment.Config.bCreateDebugInfo;
		}

		/**
		 * @return The overall output directory of actions for this target.
		 */
		public string GetOutputDirectory()
		{
			return GlobalCompileEnvironment.Config.OutputDirectory;
		}

		/** 
		 * @return true if we are building for dedicated server, false otherwise.
		 */
		public bool IsBuildingDedicatedServer()
		{
			return UEBuildConfiguration.bBuildDedicatedServer;
		}

		/** Given a UBT-built binary name (e.g. "Core"), returns a relative path to the binary for the current build configuration (e.g. "../Binaries/Win64/Core-Win64-Debug.lib") */
		public static string MakeBinaryPath(string ModuleName, string BinaryName, UnrealTargetPlatform Platform, 
			UnrealTargetConfiguration Configuration, UEBuildBinaryType BinaryType, TargetRules.TargetType? TargetType, bool bIsRocketModule, PluginInfo PluginInfo, string AppName)
		{
			// Determine the binary extension for the platform and binary type.
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(Platform);
			string BinaryExtension = BuildPlatform.GetBinaryExtension(BinaryType);

			UnrealTargetConfiguration LocalConfig = Configuration;
			if(Configuration == UnrealTargetConfiguration.DebugGame && !String.IsNullOrEmpty(ModuleName) && !RulesCompiler.IsGameModule(ModuleName))
			{
				LocalConfig = UnrealTargetConfiguration.Development;
			}
			
			string ModuleBinariesSubDir = "";
			if ((BinaryType == UEBuildBinaryType.DynamicLinkLibrary) && (string.IsNullOrEmpty(ModuleName) == false))
			{
				// Allow for modules to specify sub-folders in the Binaries folder
				string IgnoredRulesFilename;
				ModuleRules ModuleRulesObj = RulesCompiler.CreateModuleRules(ModuleName, new TargetInfo(Platform, Configuration, TargetType), out IgnoredRulesFilename);
				if (ModuleRulesObj != null)
				{
					ModuleBinariesSubDir = ModuleRulesObj.BinariesSubFolder;
				}
			}

			//@todo.Rocket: This just happens to work since exp and lib files go into intermediate...

			// Build base directory string ("../Binaries/<Platform>/")
			string BinariesDirName;
			if( PluginInfo != null )
			{
				BinariesDirName = Path.Combine( PluginInfo.Directory, "Binaries" );
			}
			else
			{
				BinariesDirName = Path.Combine( "..", "Binaries" );
			}
			var BaseDirectory = Path.Combine( BinariesDirName, Platform.ToString());
			if (ModuleBinariesSubDir.Length > 0)
			{
				BaseDirectory = Path.Combine(BaseDirectory, ModuleBinariesSubDir);
			}

			string BinarySuffix = "";
			if ((PluginInfo != null) && (BinaryType != UEBuildBinaryType.DynamicLinkLibrary))
			{
				BinarySuffix = "-Static";
			}

			// append the architecture to the end of the binary name
			BinarySuffix += BuildPlatform.GetActiveArchitecture();

			string OutBinaryPath = "";
			// Append binary file name
			if (LocalConfig == UnrealTargetConfiguration.Development)
			{
				OutBinaryPath = Path.Combine(BaseDirectory, String.Format("{0}{1}{2}", BinaryName, BinarySuffix, BinaryExtension));
			}
			else
			{
				OutBinaryPath = Path.Combine(BaseDirectory, String.Format("{0}-{1}-{2}{3}{4}", 
					BinaryName, Platform.ToString(), LocalConfig.ToString(), BinarySuffix, BinaryExtension));
			}
			return OutBinaryPath;
		}

		/** Given a UBT-built binary name (e.g. "Core"), returns a relative path to the binary for the current build configuration (e.g. "../../Binaries/Win64/Core-Win64-Debug.lib") */
		public string MakeBinaryPath(string ModuleName, string BinaryName, UEBuildBinaryType BinaryType, TargetRules.TargetType? TargetType, bool bIsRocketModule, PluginInfo PluginInfo, string AppName )
		{
			if (String.IsNullOrEmpty(ModuleName) && Configuration == UnrealTargetConfiguration.DebugGame && !bCompileMonolithic)
			{
				return MakeBinaryPath(ModuleName, BinaryName, Platform, UnrealTargetConfiguration.Development, BinaryType, TargetType, bIsRocketModule, PluginInfo, AppName);
			}
			else
			{
				return MakeBinaryPath(ModuleName, BinaryName, Platform, Configuration, BinaryType, TargetType, bIsRocketModule, PluginInfo, AppName);
			}
		}

		/// <summary>
		/// Determines whether the given plugin module is part of the current build.
		/// </summary>
		private bool ShouldIncludePluginModule(PluginInfo Plugin, PluginInfo.PluginModuleInfo Module)
		{
			// Check it can be built for this platform...
			if (Module.Platforms.Contains(Platform))
			{
				// ...and that it's appropriate for the current build environment.
				switch (Module.Type)
				{
					case PluginInfo.PluginModuleType.Runtime:
					case PluginInfo.PluginModuleType.RuntimeNoCommandlet:
						return true;

					case PluginInfo.PluginModuleType.Developer:
						return UEBuildConfiguration.bBuildDeveloperTools;

					case PluginInfo.PluginModuleType.Editor:
					case PluginInfo.PluginModuleType.EditorNoCommandlet:
						return UEBuildConfiguration.bBuildEditor;
				}
			}
			return false;
		}

		/** Sets up the modules for the target. */
		protected virtual void SetupModules()
		{
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(Platform);
			List<string> PlatformExtraModules = new List<string>();
			BuildPlatform.GetExtraModules(new TargetInfo(Platform, Configuration, Rules.Type), this, ref PlatformExtraModules);
			ExtraModuleNames.AddRange(PlatformExtraModules);			
		}

		/** Sets up the plugins for this target */
		protected virtual void SetupPlugins()
		{
			if (!UEBuildConfiguration.bExcludePlugins)
			{
				// Filter the plugins list by the current project
				List<PluginInfo> ValidPlugins = new List<PluginInfo>();
				foreach(PluginInfo Plugin in Plugins.AllPlugins)
				{
					if(Plugin.LoadedFrom != PluginInfo.LoadedFromType.GameProject || Plugin.Directory.StartsWith(ProjectDirectory, StringComparison.InvariantCultureIgnoreCase))
					{
						ValidPlugins.Add(Plugin);
					}
				}

				// Build the enabled plugin list
				if (ShouldCompileMonolithic() || Rules.Type == TargetRules.TargetType.Program)
				{
					List<string> FilterPluginNames = new List<string>(Rules.AdditionalPlugins);
					if (UEBuildConfiguration.bCompileAgainstEngine)
					{
						EngineConfiguration.ReadArray(ProjectDirectory, Platform, "Engine", "Plugins", "EnabledPlugins", FilterPluginNames);
					}
					EnabledPlugins.AddRange(ValidPlugins.Where(x => FilterPluginNames.Contains(x.Name)));
				}
				else
				{
					EnabledPlugins.AddRange(ValidPlugins);
				}
			}
		}

		/** Sets up the binaries for the target. */
		protected virtual void SetupBinaries()
		{
			if (Rules != null)
			{
				List<UEBuildBinaryConfiguration> RulesBuildBinaryConfigurations = new List<UEBuildBinaryConfiguration>();
				List<string> RulesExtraModuleNames = new List<string>();
				Rules.SetupBinaries(
					new TargetInfo(Platform, Configuration, Rules.Type),
					ref RulesBuildBinaryConfigurations,
					ref RulesExtraModuleNames
					);

				foreach (UEBuildBinaryConfiguration BinaryConfig in RulesBuildBinaryConfigurations)
				{
					BinaryConfig.OutputFilePath = OutputPath;
					BinaryConfig.IntermediateDirectory = ProjectIntermediateDirectory;

					if (BinaryConfig.ModuleNames.Count > 0)
					{
						AppBinaries.Add(new UEBuildBinaryCPP( this, BinaryConfig ));
					}
					else
					{
						AppBinaries.Add(new UEBuildBinaryCSDLL( this, BinaryConfig ));
					}
				}

				ExtraModuleNames.AddRange(RulesExtraModuleNames);
			}
		}

		public virtual void SetupDefaultGlobalEnvironment(
			TargetInfo Target,
			ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
			ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
			)
		{
			// Default does nothing
		}

		/** Sets up the global compile and link environment for the target. */
		public virtual void SetupGlobalEnvironment()
		{
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(Platform);

			UEToolChain ToolChain = UEToolChain.GetPlatformToolChain(BuildPlatform.GetCPPTargetPlatform(Platform));
			ToolChain.SetUpGlobalEnvironment();

			// Allow each target type (Game/Editor/Server) to set a default global environment state
			{
				LinkEnvironmentConfiguration RulesLinkEnvConfig = GlobalLinkEnvironment.Config;
				CPPEnvironmentConfiguration RulesCPPEnvConfig = GlobalCompileEnvironment.Config;
				SetupDefaultGlobalEnvironment(
						new TargetInfo(Platform, Configuration, Rules.Type),
						ref RulesLinkEnvConfig,
						ref RulesCPPEnvConfig
						);
				// Copy them back...
				GlobalLinkEnvironment.Config = RulesLinkEnvConfig;
				GlobalCompileEnvironment.Config = RulesCPPEnvConfig;
			}

			// If there is a Rules object present, let it set things up before validation.
			// This mirrors the behavior of the UEBuild<TARGET> setup, where the 
			// SetupGlobalEnvironment call would first set the target-specific values, 
			// and then call Base.SetupGlobalEnvironment
			if (Rules != null)
			{
				LinkEnvironmentConfiguration RulesLinkEnvConfig = GlobalLinkEnvironment.Config;
				CPPEnvironmentConfiguration RulesCPPEnvConfig = GlobalCompileEnvironment.Config;

				// Don't let games override the global environment...
				// This will potentially cause problems due to them generating a game-agnostic exe.
				// RocketEditor is a special case.
				// 
				bool bAllowGameOverride = !TargetRules.IsGameType(Rules.Type);
				if (bAllowGameOverride ||
					(Rules.ToString() == "UE4EditorTarget" && UnrealBuildTool.BuildingRocket()) ||	// @todo Rocket: Hard coded target name hack
					Rules.ShouldCompileMonolithic(Platform, Configuration))
				{
					Rules.SetupGlobalEnvironment(
						new TargetInfo(Platform, Configuration, Rules.Type),
						ref RulesLinkEnvConfig,
						ref RulesCPPEnvConfig
						);

					// Copy them back...
					GlobalLinkEnvironment.Config = RulesLinkEnvConfig;
					GlobalCompileEnvironment.Config = RulesCPPEnvConfig;
				}
				GlobalCompileEnvironment.Config.Definitions.Add(String.Format("IS_PROGRAM={0}", Rules.Type == TargetRules.TargetType.Program ? "1" : "0"));
			}

			// Validate UE configuration - needs to happen before setting any environment mojo and after argument parsing.
			BuildPlatform.ValidateUEBuildConfiguration();
			UEBuildConfiguration.ValidateConfiguration();

			// Add the 'Engine/Source' path as a global include path for all modules
			var EngineSourceDirectory = Path.GetFullPath( Path.Combine( "..", "..", "Engine", "Source" ) );
			if( !Directory.Exists( EngineSourceDirectory ) )
			{
				throw new BuildException( "Couldn't find Engine/Source directory using relative path" );
			}
			GlobalCompileEnvironment.Config.IncludePaths.Add( EngineSourceDirectory );

			//@todo.PLATFORM: Do any platform specific tool chain initialization here if required

			if (BuildConfiguration.bUseMallocProfiler)
			{
				BuildConfiguration.bOmitFramePointers = false;
				GlobalCompileEnvironment.Config.Definitions.Add("USE_MALLOC_PROFILER=1");
			}

			// Incorporate toolchain in platform name.
			string PlatformString = Platform.ToString();

			string OutputAppName = GetAppName();
		
			
			// Rocket intermediates go to the project's intermediate folder.  Rocket never writes to the engine intermediate folder. (Those files are immutable)
			// Also, when compiling in monolithic, all intermediates go to the project's folder.  This is because a project can change definitions that affects all engine translation
			// units too, so they can't be shared between different targets.  They are effectively project-specific engine intermediates.
			if( UnrealBuildTool.RunningRocket() || ( UnrealBuildTool.HasUProjectFile() && ShouldCompileMonolithic() ) )
			{
				//@todo SAS: Add a true Debug mode!
				var IntermediateConfiguration = Configuration;
				if( UnrealBuildTool.RunningRocket() )
				{
					// Only Development and Shipping are supported for engine modules
					if( IntermediateConfiguration != UnrealTargetConfiguration.Development && IntermediateConfiguration != UnrealTargetConfiguration.Shipping )
					{
						IntermediateConfiguration = UnrealTargetConfiguration.Development;
					}
				}

				GlobalCompileEnvironment.Config.OutputDirectory = Path.Combine(BuildConfiguration.PlatformIntermediatePath, OutputAppName, IntermediateConfiguration.ToString());
				if (ShouldCompileMonolithic())
				{
					GlobalCompileEnvironment.Config.OutputDirectory = Path.Combine(UnrealBuildTool.GetUProjectPath(), BuildConfiguration.PlatformIntermediateFolder, OutputAppName, IntermediateConfiguration.ToString());
				}
			}
			else
			{
				GlobalCompileEnvironment.Config.OutputDirectory = Path.Combine(BuildConfiguration.PlatformIntermediatePath, OutputAppName, Configuration.ToString());
			}

			// By default, shadow source files for this target in the root OutputDirectory
			GlobalCompileEnvironment.Config.LocalShadowDirectory = GlobalCompileEnvironment.Config.OutputDirectory;

			GlobalCompileEnvironment.Config.Definitions.Add("UNICODE");
			GlobalCompileEnvironment.Config.Definitions.Add("_UNICODE");
			GlobalCompileEnvironment.Config.Definitions.Add("__UNREAL__");

			GlobalCompileEnvironment.Config.Definitions.Add(String.Format("IS_MONOLITHIC={0}", ShouldCompileMonolithic() ? "1" : "0"));

			if (UEBuildConfiguration.bCompileAgainstEngine)
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_ENGINE=1");
				GlobalCompileEnvironment.Config.Definitions.Add(
					String.Format("WITH_UNREAL_DEVELOPER_TOOLS={0}",UEBuildConfiguration.bBuildDeveloperTools ? "1" : "0"));
				// Automatically include CoreUObject
				UEBuildConfiguration.bCompileAgainstCoreUObject = true;
			}
			else
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_ENGINE=0");
				// Can't have developer tools w/out engine
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_UNREAL_DEVELOPER_TOOLS=0");
			}

			if (UEBuildConfiguration.bCompileAgainstCoreUObject)
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_COREUOBJECT=1");
			}
			else
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_COREUOBJECT=0");
			}

			if (UEBuildConfiguration.bCompileWithStatsWithoutEngine)
            {
				GlobalCompileEnvironment.Config.Definitions.Add("USE_STATS_WITHOUT_ENGINE=1");
            }
            else
            {
				GlobalCompileEnvironment.Config.Definitions.Add("USE_STATS_WITHOUT_ENGINE=0");
            }

			if (UEBuildConfiguration.bCompileWithPluginSupport)
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_PLUGIN_SUPPORT=1");
			}
			else
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_PLUGIN_SUPPORT=0");
			}

            if (UEBuildConfiguration.bUseLoggingInShipping)
            {
                GlobalCompileEnvironment.Config.Definitions.Add("USE_LOGGING_IN_SHIPPING=1");
            }
            else
            {
                GlobalCompileEnvironment.Config.Definitions.Add("USE_LOGGING_IN_SHIPPING=0");
            }

			// Propagate whether we want a lean and mean build to the C++ code.
			if (UEBuildConfiguration.bCompileLeanAndMeanUE)
			{
				GlobalCompileEnvironment.Config.Definitions.Add("UE_BUILD_MINIMAL=1");
			}
			else
			{
				GlobalCompileEnvironment.Config.Definitions.Add("UE_BUILD_MINIMAL=0");
			}

			// Disable editor when its not needed
			if (BuildPlatform.ShouldNotBuildEditor(Platform, Configuration) == true)
			{
				UEBuildConfiguration.bBuildEditor = false;
			}

			// Disable the DDC and a few other things related to preparing assets
			if (BuildPlatform.BuildRequiresCookedData(Platform, Configuration) == true)
			{
				UEBuildConfiguration.bBuildRequiresCookedData = true;
			}

			// bBuildEditor has now been set appropriately for all platforms, so this is here to make sure the #define 
			if (UEBuildConfiguration.bBuildEditor)
			{
				// Must have editor only data if building the editor.
				UEBuildConfiguration.bBuildWithEditorOnlyData = true;
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_EDITOR=1");
			}
			else if (!GlobalCompileEnvironment.Config.Definitions.Contains("WITH_EDITOR=0"))
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_EDITOR=0");
			}

			if (UEBuildConfiguration.bBuildWithEditorOnlyData == false)
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_EDITORONLY_DATA=0");
			}

			// Check if server-only code should be compiled out.
			if (UEBuildConfiguration.bWithServerCode == true)
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_SERVER_CODE=1");
			}
			else
			{
				GlobalCompileEnvironment.Config.Definitions.Add("WITH_SERVER_CODE=0");
			}

			// tell the compiled code the name of the UBT platform (this affects folder on disk, etc that the game may need to know)
			GlobalCompileEnvironment.Config.Definitions.Add("UBT_COMPILED_PLATFORM=" + Platform.ToString());

			// Initialize the compile and link environments for the platform and configuration.
			SetUpPlatformEnvironment();
			SetUpConfigurationEnvironment();

			// Validates current settings and updates if required.
			BuildConfiguration.ValidateConfiguration(
				GlobalCompileEnvironment.Config.TargetConfiguration,
				GlobalCompileEnvironment.Config.TargetPlatform,
				GlobalCompileEnvironment.Config.bCreateDebugInfo);
		}

		void SetUpPlatformEnvironment()
		{
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(Platform);

			CPPTargetPlatform MainCompilePlatform = BuildPlatform.GetCPPTargetPlatform(Platform);

			GlobalLinkEnvironment.Config.TargetPlatform = MainCompilePlatform;
			GlobalCompileEnvironment.Config.TargetPlatform = MainCompilePlatform;

			string ActiveArchitecture = BuildPlatform.GetActiveArchitecture();
			GlobalCompileEnvironment.Config.TargetArchitecture = ActiveArchitecture;
			GlobalLinkEnvironment.Config.TargetArchitecture = ActiveArchitecture;


			// Set up the platform-specific environment.
			BuildPlatform.SetUpEnvironment(this);
		}

		void SetUpConfigurationEnvironment()
		{
			UEBuildPlatform.GetBuildPlatform(Platform).SetUpConfigurationEnvironment(this);

			// Check to see if we're compiling a library or not
			bool bIsBuildingDLL = Path.GetExtension(OutputPath).ToUpperInvariant() == ".DLL";
			GlobalCompileEnvironment.Config.bIsBuildingDLL = bIsBuildingDLL;
			GlobalLinkEnvironment.Config.bIsBuildingDLL = bIsBuildingDLL;
			bool bIsBuildingLibrary = Path.GetExtension(OutputPath).ToUpperInvariant() == ".LIB";
			GlobalCompileEnvironment.Config.bIsBuildingLibrary = bIsBuildingLibrary;
			GlobalLinkEnvironment.Config.bIsBuildingLibrary = bIsBuildingLibrary;
		}

		/** Constructs a TargetInfo object for this target. */
		public TargetInfo GetTargetInfo()
		{
			if(Rules == null)
			{
				return new TargetInfo(Platform, Configuration);
			}
			else
			{
				return new TargetInfo(Platform, Configuration, Rules.Type);
			}
		}

		/** Registers a module with this target. */
		public void RegisterModule(UEBuildModule Module)
		{
			Debug.Assert(Module.Target == this);
			Modules.Add(Module.Name, Module);
		}

		/** Finds a module given its name.  Throws an exception if the module couldn't be found. */
		public UEBuildModule FindOrCreateModuleByName(string Name)
		{
			UEBuildModule Result;
			if (Modules.TryGetValue(Name, out Result))
			{
				return Result;
			}
			else
			{
				TargetInfo TargetInfo = GetTargetInfo();

				// Create the module!  (It will be added to our hash table in its constructor)
				var NewModule = CreateModule(Name);

				UnrealTargetPlatform Only = UnrealBuildTool.GetOnlyPlatformSpecificFor();

				if (Only == UnrealTargetPlatform.Unknown && UnrealBuildTool.SkipNonHostPlatforms())
				{
					Only = Platform;
				}
				// Allow all build platforms to 'adjust' the module setting. 
				// This will allow undisclosed platforms to make changes without 
				// exposing information about the platform in publicly accessible 
				// locations.
				UEBuildPlatform.PlatformModifyNewlyLoadedModule(NewModule, TargetInfo, Only);
				return NewModule;
			}
		}

		/** Finds a module given its name.  Throws an exception if the module couldn't be found. */
		public UEBuildModule GetModuleByName(string Name)
		{
			UEBuildModule Result;
			if (Modules.TryGetValue(Name, out Result))
			{
				return Result;
			}
			else
			{
				throw new BuildException("Couldn't find referenced module '{0}'.", Name);
			}
		}


		/// <summary>
		/// Combines a list of paths with a base path.
		/// </summary>
		/// <param name="BasePath">Base path to combine with. May be null or empty.</param>
		/// <param name="PathList">List of input paths to combine with. May be null.</param>
		/// <returns>List of paths relative The build module object for the specified build rules source file</returns>
		private static List<string> CombinePathList(string BasePath, List<string> PathList)
		{
			List<string> NewPathList = PathList;
			if (PathList != null && !String.IsNullOrEmpty(BasePath))
			{
				NewPathList = new List<string>();
				foreach (string Path in PathList)
				{
					NewPathList.Add(System.IO.Path.Combine(BasePath, Path));
				}
			}
			return NewPathList;
		}


		/// <summary>
		/// Given a list of source files for a module, filters them into a list of files that should actually be included in a build
		/// </summary>
		/// <param name="SourceFiles">Original list of files, which may contain non-source</param>
		/// <param name="SourceFilesBaseDirectory">Directory that the source files are in</param>
		/// <param name="TargetPlatform">The platform we're going to compile for</param>
		/// <returns>The list of source files to actually compile</returns>
		static List<FileItem> GetCPlusPlusFilesToBuild(List<string> SourceFiles, string SourceFilesBaseDirectory, UnrealTargetPlatform TargetPlatform)
		{
			// Make a list of all platform name strings that we're *not* currently compiling, to speed
			// up file path comparisons later on
			var SupportedPlatforms = new List<UnrealTargetPlatform>();
			SupportedPlatforms.Add(TargetPlatform);
			var OtherPlatformNameStrings = Utils.MakeListOfUnsupportedPlatforms(SupportedPlatforms);


			// @todo projectfiles: Consider saving out cached list of source files for modules so we don't need to harvest these each time

			var FilteredFileItems = new List<FileItem>();
			FilteredFileItems.Capacity = SourceFiles.Count;

			// @todo projectfiles: hard-coded source file set.  Should be made extensible by platform tool chains.
			var CompilableSourceFileTypes = new string[]
				{
					".cpp",
					".c",
					".cc",
					".mm",
					".m",
					".rc",
					".manifest"
				};

			// When generating project files, we have no file to extract source from, so we'll locate the code files manually
			foreach (var SourceFilePath in SourceFiles)
			{
				// We're only able to compile certain types of files
				bool IsCompilableSourceFile = false;
				foreach (var CurExtension in CompilableSourceFileTypes)
				{
					if (SourceFilePath.EndsWith(CurExtension, StringComparison.InvariantCultureIgnoreCase))
					{
						IsCompilableSourceFile = true;
						break;
					}
				}

				if (IsCompilableSourceFile)
				{
					if (SourceFilePath.StartsWith(SourceFilesBaseDirectory + Path.DirectorySeparatorChar))
					{
						// Store the path as relative to the project file
						var RelativeFilePath = Utils.MakePathRelativeTo(SourceFilePath, SourceFilesBaseDirectory);

						// All compiled files should always be in a sub-directory under the project file directory.  We enforce this here.
						if (Path.IsPathRooted(RelativeFilePath) || RelativeFilePath.StartsWith(".."))
						{
							throw new BuildException("Error: Found source file {0} in project whose path was not relative to the base directory of the source files", RelativeFilePath);
						}

						// Check for source files that don't belong to the platform we're currently compiling.  We'll filter
						// those source files out
						bool IncludeThisFile = true;
						foreach (var CurPlatformName in OtherPlatformNameStrings)
						{
							if (RelativeFilePath.IndexOf(Path.DirectorySeparatorChar + CurPlatformName + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase) != -1
								|| RelativeFilePath.StartsWith(CurPlatformName + Path.DirectorySeparatorChar))
							{
								IncludeThisFile = false;
								break;
							}
						}

						if (IncludeThisFile)
						{
							FilteredFileItems.Add(FileItem.GetItemByFullPath(SourceFilePath));
						}
					}
				}
			}

			// @todo projectfiles: Consider enabling this error but changing it to a warning instead.  It can fire for
			//    projects that are being digested for IntelliSense (because the module was set as a cross-
			//	  platform dependency), but all of their source files were filtered out due to platform masking
			//    in the project generator
			bool AllowEmptyProjects = true;
			if (!AllowEmptyProjects)
			{
				if (FilteredFileItems.Count == 0)
				{
					throw new BuildException("Could not find any valid source files for base directory {0}.  Project has {1} files in it", SourceFilesBaseDirectory, SourceFiles.Count);
				}
			}

			return FilteredFileItems;
		}

		/// <summary>
		/// Creates a module object for the specified module name.
		/// </summary>
		/// <param name="ModuleName">Name of the module</param>
		/// <param name="Target">Information about the target associated with this module</param>
		/// <param name="bBuildFiles">True to add files to build, false to skip</param>
		/// <returns>The build module object for the specified build rules source file</returns>
		private UEBuildModule CreateModule(string ModuleName)
		{
			// Figure out whether we need to build this module
			bool bBuildFiles = OnlyModules.Count == 0 || OnlyModules.Any(x => x.OnlyModuleName == ModuleName);
			
			// @todo projectfiles: Cross-platform modules can appear here during project generation, but they may have already
			//   been filtered out by the project generator.  This causes the projects to not be added to directories properly.
			string ModuleFileName;
			var RulesObject = RulesCompiler.CreateModuleRules(ModuleName, GetTargetInfo(), out ModuleFileName);
			var ModuleDirectory = Path.GetDirectoryName(ModuleFileName);

			// Making an assumption here that any project file path that contains '../../'
			// is NOT from the engine and therefore must be an application-specific module.
			bool IsGameModule = false;
			string ApplicationOutputPath = "";
			var ModuleFileRelativeToEngineDirectory = Utils.MakePathRelativeTo(ModuleFileName, ProjectFileGenerator.EngineRelativePath);
			if (ModuleFileRelativeToEngineDirectory.StartsWith("..") || Path.IsPathRooted(ModuleFileRelativeToEngineDirectory))
			{
				ApplicationOutputPath = ModuleFileName;
				int SourceIndex = ApplicationOutputPath.IndexOf(Path.DirectorySeparatorChar + "Source");
				if (SourceIndex != -1)
				{
					ApplicationOutputPath = ApplicationOutputPath.Substring(0, SourceIndex + 1);
					IsGameModule = true;
				}
				else
				{
					throw new BuildException("Unable to parse application directory for module '{0}' ({1}). Is it in '../../<APP>/Source'?",
						ModuleName, ModuleFileName);
				}
			}

			// Get the base directory for paths referenced by the module. If the module's under the UProject source directory use that, otherwise leave it relative to the Engine source directory.
			string ProjectSourcePath = UnrealBuildTool.GetUProjectSourcePath();
			if (ProjectSourcePath != null)
			{
				string FullProjectSourcePath = Path.GetFullPath(ProjectSourcePath);
				if (Utils.IsFileUnderDirectory(ModuleFileName, FullProjectSourcePath))
				{
					RulesObject.PublicIncludePaths = CombinePathList(ProjectSourcePath, RulesObject.PublicIncludePaths);
					RulesObject.PrivateIncludePaths = CombinePathList(ProjectSourcePath, RulesObject.PrivateIncludePaths);
					RulesObject.PublicLibraryPaths = CombinePathList(ProjectSourcePath, RulesObject.PublicLibraryPaths);
					RulesObject.PublicAdditionalShadowFiles = CombinePathList(ProjectSourcePath, RulesObject.PublicAdditionalShadowFiles);
				}
			}

			// Don't generate include paths for third party modules.  They don't follow our conventions.
			if (RulesObject.Type != ModuleRules.ModuleType.External)
			{
				// Add the default include paths to the module rules, if they exist.
				RulesCompiler.AddDefaultIncludePathsToModuleRules(this, ModuleName, ModuleFileName, ModuleFileRelativeToEngineDirectory, IsGameModule: IsGameModule, RulesObject: ref RulesObject);
			}

			IntelliSenseGatherer IntelliSenseGatherer = null;
			List<FileItem> ModuleSourceFiles = new List<FileItem>();
			if (RulesObject.Type == ModuleRules.ModuleType.CPlusPlus || RulesObject.Type == ModuleRules.ModuleType.CPlusPlusCLR)
			{
				ProjectFile ProjectFile = null;
				if (ProjectFileGenerator.bGenerateProjectFiles && ProjectFileGenerator.ModuleToProjectFileMap.TryGetValue(ModuleName, out ProjectFile))
				{
					IntelliSenseGatherer = ProjectFile;
				}

				if (!ProjectFileGenerator.bGenerateProjectFiles && bBuildFiles)	// We don't care about actual source files when generating projects, as these are discovered separately
				{
					// So all we care about are the game module and/or plugins.
					//@todo Rocket: This assumes plugins that have source will be under the game folder...
					if (!UnrealBuildTool.RunningRocket() || Utils.IsFileUnderDirectory(ModuleFileName, UnrealBuildTool.GetUProjectPath()))
					{
						var SourceFilePaths = new List<string>();

						if (ProjectFile != null)
						{
							foreach (var SourceFile in ProjectFile.SourceFiles)
							{
								SourceFilePaths.Add(SourceFile.FilePath);
							}
						}
						else
						{
							// Don't have a project file for this module with the source file names cached already, so find the source files ourselves
							SourceFilePaths = SourceFileSearch.FindModuleSourceFiles(
								ModuleRulesFile: ModuleFileName,
								ExcludeNoRedistFiles: false);
						}
						ModuleSourceFiles = GetCPlusPlusFilesToBuild(SourceFilePaths, ModuleDirectory, Platform);
					}
				}
			}

			// Get the type of module we're creating
			UEBuildModuleType ModuleType = IsGameModule ? UEBuildModuleType.GameModule : UEBuildModuleType.EngineModule;

			// Now, go ahead and create the module builder instance
			UEBuildModule Module = null;
			switch (RulesObject.Type)
			{
				case ModuleRules.ModuleType.CPlusPlus:
					{
						Module = new UEBuildModuleCPP(
							InTarget: this,
							InName: ModuleName,
							InType: ModuleType,
							InModuleDirectory: ModuleDirectory,
							InOutputDirectory: ApplicationOutputPath,
							InIntelliSenseGatherer: IntelliSenseGatherer,
							InSourceFiles: ModuleSourceFiles,
							InPublicSystemIncludePaths: RulesObject.PublicSystemIncludePaths,
							InPublicIncludePaths: RulesObject.PublicIncludePaths,
							InDefinitions: RulesObject.Definitions,
							InPublicIncludePathModuleNames: RulesObject.PublicIncludePathModuleNames,
							InPublicDependencyModuleNames: RulesObject.PublicDependencyModuleNames,
							InPublicDelayLoadDLLs: RulesObject.PublicDelayLoadDLLs,
							InPublicAdditionalLibraries: RulesObject.PublicAdditionalLibraries,
							InPublicFrameworks: RulesObject.PublicFrameworks,
							InPublicAdditionalFrameworks: RulesObject.PublicAdditionalFrameworks,
							InPrivateIncludePaths: RulesObject.PrivateIncludePaths,
							InPrivateIncludePathModuleNames: RulesObject.PrivateIncludePathModuleNames,
							InPrivateDependencyModuleNames: RulesObject.PrivateDependencyModuleNames,
							InCircularlyReferencedDependentModules: RulesObject.CircularlyReferencedDependentModules,
							InDynamicallyLoadedModuleNames: RulesObject.DynamicallyLoadedModuleNames,
							InPlatformSpecificDynamicallyLoadedModuleNames: RulesObject.PlatformSpecificDynamicallyLoadedModuleNames,
							InOptimizeCode: RulesObject.OptimizeCode,
							InAllowSharedPCH: (RulesObject.PCHUsage == ModuleRules.PCHUsageMode.NoSharedPCHs) ? false : true,
							InSharedPCHHeaderFile: RulesObject.SharedPCHHeaderFile,
							InUseRTTI: RulesObject.bUseRTTI,
							InEnableBufferSecurityChecks: RulesObject.bEnableBufferSecurityChecks,
							InFasterWithoutUnity: RulesObject.bFasterWithoutUnity,
							InMinFilesUsingPrecompiledHeaderOverride: RulesObject.MinFilesUsingPrecompiledHeaderOverride,
							InEnableExceptions: RulesObject.bEnableExceptions,
							InEnableInlining: RulesObject.bEnableInlining
							);
					}
					break;

				case ModuleRules.ModuleType.CPlusPlusCLR:
					{
						Module = new UEBuildModuleCPPCLR(
							InTarget: this,
							InName: ModuleName,
							InType: ModuleType,
							InModuleDirectory: ModuleDirectory,
							InOutputDirectory: ApplicationOutputPath,
							InIntelliSenseGatherer: IntelliSenseGatherer,
							InSourceFiles: ModuleSourceFiles,
							InDefinitions: RulesObject.Definitions,
							InPublicSystemIncludePaths: RulesObject.PublicSystemIncludePaths,
							InPublicIncludePaths: RulesObject.PublicIncludePaths,
							InPublicIncludePathModuleNames: RulesObject.PublicIncludePathModuleNames,
							InPublicDependencyModuleNames: RulesObject.PublicDependencyModuleNames,
							InPublicDelayLoadDLLs: RulesObject.PublicDelayLoadDLLs,
							InPublicAdditionalLibraries: RulesObject.PublicAdditionalLibraries,
							InPublicFrameworks: RulesObject.PublicFrameworks,
							InPublicAdditionalFrameworks: RulesObject.PublicAdditionalFrameworks,
							InPrivateIncludePaths: RulesObject.PrivateIncludePaths,
							InPrivateIncludePathModuleNames: RulesObject.PrivateIncludePathModuleNames,
							InPrivateDependencyModuleNames: RulesObject.PrivateDependencyModuleNames,
							InPrivateAssemblyReferences: RulesObject.PrivateAssemblyReferences,
							InCircularlyReferencedDependentModules: RulesObject.CircularlyReferencedDependentModules,
							InDynamicallyLoadedModuleNames: RulesObject.DynamicallyLoadedModuleNames,
							InPlatformSpecificDynamicallyLoadedModuleNames: RulesObject.PlatformSpecificDynamicallyLoadedModuleNames,
							InOptimizeCode: RulesObject.OptimizeCode,
							InAllowSharedPCH: (RulesObject.PCHUsage == ModuleRules.PCHUsageMode.NoSharedPCHs) ? false : true,
							InSharedPCHHeaderFile: RulesObject.SharedPCHHeaderFile,
							InUseRTTI: RulesObject.bUseRTTI,
							InEnableBufferSecurityChecks: RulesObject.bEnableBufferSecurityChecks,
							InFasterWithoutUnity: RulesObject.bFasterWithoutUnity,
							InMinFilesUsingPrecompiledHeaderOverride: RulesObject.MinFilesUsingPrecompiledHeaderOverride,
							InEnableExceptions: RulesObject.bEnableExceptions,
							InEnableInlining: RulesObject.bEnableInlining
							);
					}
					break;

				case ModuleRules.ModuleType.External:
					Module = new UEBuildExternalModule(
						InTarget: this,
						InName: ModuleName,
						InType: ModuleType,
						InModuleDirectory: ModuleDirectory,
						InOutputDirectory: ApplicationOutputPath,
						InPublicDefinitions: RulesObject.Definitions,
						InPublicSystemIncludePaths: RulesObject.PublicSystemIncludePaths,
						InPublicIncludePaths: RulesObject.PublicIncludePaths,
						InPublicLibraryPaths: RulesObject.PublicLibraryPaths,
						InPublicAdditionalLibraries: RulesObject.PublicAdditionalLibraries,
						InPublicFrameworks: RulesObject.PublicFrameworks,
						InPublicAdditionalFrameworks: RulesObject.PublicAdditionalFrameworks,
						InPublicAdditionalShadowFiles: RulesObject.PublicAdditionalShadowFiles,
						InPublicDependencyModuleNames: RulesObject.PublicDependencyModuleNames,
						InPublicDelayLoadDLLs: RulesObject.PublicDelayLoadDLLs);
					break;

				default:
					throw new BuildException("Unrecognized module type specified by 'Rules' object {0}", RulesObject.ToString());
			}

			return Module;
		}
	}
}
