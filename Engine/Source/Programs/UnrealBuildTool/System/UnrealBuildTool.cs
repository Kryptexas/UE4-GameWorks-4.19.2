// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Threading;
using System.Reflection;
using System.Linq;

namespace UnrealBuildTool
{
	public partial class UnrealBuildTool
	{
		/** Time at which control fist passes to UBT. */
		static public DateTime StartTime;

		/** Time we waited for the mutex. Tracked for proper active execution time collection. */
		static public TimeSpan MutexWaitTime = TimeSpan.Zero;

		static public string BuildGuid = Guid.NewGuid().ToString();

		/** Total time spent building projects. */
		static public double TotalBuildProjectTime = 0;

		/** Total time spent compiling. */
		static public double TotalCompileTime = 0;

		/** Total time spent creating app bundles. */
		static public double TotalCreateAppBundleTime = 0;

		/** Total time spent generating debug information. */
		static public double TotalGenerateDebugInfoTime = 0;

		/** Total time spent linking. */
		static public double TotalLinkTime = 0;

		/** Total time spent on other actions. */
		static public double TotalOtherActionsTime = 0;

		/** The command line */
		static public List<string> CmdLine = new List<string>();

        /** The environment at boot time. */
        static public System.Collections.IDictionary InitialEnvironment;

        /** Should we skip all extra modules for platforms? */
        static bool bSkipNonHostPlatforms = false;

        static UnrealTargetPlatform OnlyPlatformSpecificFor = UnrealTargetPlatform.Unknown;

        /** Are we running for Rocket */
		static bool bRunningRocket = false;

		/** Are we building Rocket */
		static bool bBuildingRocket = false;

		/** The project file */
		static string UProjectFile = "";

		/** The Unreal project file path.  This should always be valid when compiling game projects.  Engine targets and program targets may not have a UProject set. */
		static bool bHasUProjectFile = false;

		/** The Unreal project directory.  This should always be valid when compiling game projects.  Engine targets and program targets may not have a UProject set. */
		static string UProjectPath = "";

		/** The configuration to build rocket modules in */
		static UnrealTargetConfiguration RocketConfiguration = UnrealTargetConfiguration.Development;

        /// <summary>
        /// Returns true if UnrealBuildTool should skip everything other than the host platform as far as finding other modules that need to be compiled.
        /// </summary>
        /// <returns>True if running with "-skipnonhostplatforms"</returns>
        static public bool SkipNonHostPlatforms()
        {
            return bSkipNonHostPlatforms;
        }

        /// <summary>
        /// Returns the platform we should compile extra modules for, in addition to the host
        /// </summary>
        /// <returns>The extra platfrom to compile for with "-onlyhostplatformand="</returns>
        static public UnrealTargetPlatform GetOnlyPlatformSpecificFor()
        {
            return OnlyPlatformSpecificFor;
        }

		/// <summary>
		/// Returns true if UnrealBuildTool is running with the "-rocket" option (Rocket mode)
		/// </summary>
		/// <returns>True if running with "-rocket"</returns>
		static public bool RunningRocket()
		{
			return bRunningRocket;
		}

		/// <summary>
		/// Checks to see if we're building a "Rocket" target
		/// </summary>
		/// <returns>True if building a "Rocket" target, such as RocketEditor (not running with "-rocket" though)</returns>
		static public bool BuildingRocket()
		{
			return bBuildingRocket;
		}

		/// <summary>
		/// Returns true if a UProject file is available.  This should always be the case when working with game projects.  For engine or program targets, a UProject will not be available.
		/// </summary>
		/// <returns></returns>
		static public bool HasUProjectFile()
		{
			return bHasUProjectFile;
		}

		/// <summary>
		/// The Unreal project file path.  This should always be valid when compiling game projects.  Engine targets and program targets may not have a UProject set.
		/// </summary>
		/// <returns>The path to the file</returns>
		static public string GetUProjectFile()
		{
			return UProjectFile;
		}

		/// <summary>
		/// The Unreal project directory.  This should always be valid when compiling game projects.  Engine targets and program targets may not have a UProject set. */
		/// </summary>
		/// <returns>The directory path</returns>
		static public string GetUProjectPath()
		{
			return UProjectPath;
		}

		/// <summary>
		/// The Unreal project source directory.  This should always be valid when compiling game projects.  Engine targets and program targets may not have a UProject set. */
		/// </summary>
		/// <returns>Relative path to the project's source directory</returns>
		static public string GetUProjectSourcePath()
		{
			string ProjectPath = GetUProjectPath();
			if (ProjectPath != null)
			{
				ProjectPath = Path.Combine(ProjectPath, "Source");
			}
			return ProjectPath;
		}

		static public string GetUBTPath()
		{
			string UnrealBuildToolPath = Path.Combine(Utils.GetExecutingAssemblyDirectory(), "UnrealBuildTool.exe");
			return UnrealBuildToolPath;
		}

		// @todo projectfiles: Move this into the ProjectPlatformGeneration class?
		/// <summary>
		/// IsDesktopPlatform
		/// </summary>
		/// <param name="InPlatform">The platform of interest</param>
		/// <returns>True if the given platform is a 'desktop' platform</returns>
		static public bool IsDesktopPlatform(UnrealTargetPlatform InPlatform)
		{
			// Windows and Mac are desktop platforms.
			return (
				(InPlatform == UnrealTargetPlatform.Win64) ||
				(InPlatform == UnrealTargetPlatform.Win32) ||
				(InPlatform == UnrealTargetPlatform.Mac)
				);
		}

        // @todo projectfiles: Move this into the ProjectPlatformGeneration class?
        /// <summary>
        /// IsServerPlatform
        /// </summary>
        /// <param name="InPlatform">The platform of interest</param>
        /// <returns>True if the given platform is a 'server' platform</returns>
        static public bool IsServerPlatform(UnrealTargetPlatform InPlatform)
        {
            // Windows, Mac, and Linux are desktop platforms.
            return (
                (InPlatform == UnrealTargetPlatform.Win64) ||
                (InPlatform == UnrealTargetPlatform.Win32) ||
                (InPlatform == UnrealTargetPlatform.Mac) ||
                (InPlatform == UnrealTargetPlatform.WinRT) ||
                (InPlatform == UnrealTargetPlatform.Linux)
                );
        }

        // @todo projectfiles: Move this into the ProjectPlatformGeneration class?
        /// <summary>
        /// PlatformSupportsCrashReporter
        /// </summary>
        /// <param name="InPlatform">The platform of interest</param>
        /// <returns>True if the given platform supports a crash reporter client (i.e. it can be built for it)</returns>
        static public bool PlatformSupportsCrashReporter(UnrealTargetPlatform InPlatform)
        {
            return (
                (InPlatform == UnrealTargetPlatform.Win64) ||
                (InPlatform == UnrealTargetPlatform.Win32) ||
                (InPlatform == UnrealTargetPlatform.Linux) ||
                (InPlatform == UnrealTargetPlatform.Mac)
                );
        }

        /// <summary>
		/// Get all platforms
		/// </summary>
		/// <param name="OutPlatforms">The list of platform to fill in</param>
		/// <param name="bCheckValidity">If true, ensure it is a valid platform</param>
		/// <returns>true if successful and platforms are found, false if not</returns>
		static public bool GetAllPlatforms(ref List<UnrealTargetPlatform> OutPlatforms, bool bCheckValidity = true)
		{
			OutPlatforms.Clear();
			foreach (UnrealTargetPlatform platform in Enum.GetValues(typeof(UnrealTargetPlatform)))
			{
				UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(platform, true);
				if ((bCheckValidity == false) || (BuildPlatform != null))
				{
					OutPlatforms.Add(platform);
				}
			}

			return (OutPlatforms.Count > 0) ? true : false;
		}

		/// <summary>
		/// Get all desktop platforms
		/// </summary>
		/// <param name="OutPlatforms">The list of platform to fill in</param>
		/// <param name="bCheckValidity">If true, ensure it is a valid platform</param>
		/// <returns>true if successful and platforms are found, false if not</returns>
		static public bool GetAllDesktopPlatforms(ref List<UnrealTargetPlatform> OutPlatforms, bool bCheckValidity = true)
		{
			OutPlatforms.Clear();
			foreach (UnrealTargetPlatform platform in Enum.GetValues(typeof(UnrealTargetPlatform)))
			{
				if (UnrealBuildTool.IsDesktopPlatform(platform))
				{
					UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(platform, true);
					if ((bCheckValidity == false) || (BuildPlatform != null))
					{
						OutPlatforms.Add(platform);
					}
				}
			}

			return (OutPlatforms.Count > 0) ? true : false;
		}

        /// <summary>
        /// Get all server platforms
        /// </summary>
        /// <param name="OutPlatforms">The list of platform to fill in</param>
        /// <param name="bCheckValidity">If true, ensure it is a valid platform</param>
        /// <returns>true if successful and platforms are found, false if not</returns>
        static public bool GetAllServerPlatforms(ref List<UnrealTargetPlatform> OutPlatforms, bool bCheckValidity = true)
        {
            OutPlatforms.Clear();
            foreach (UnrealTargetPlatform platform in Enum.GetValues(typeof(UnrealTargetPlatform)))
            {
                if (UnrealBuildTool.IsServerPlatform(platform))
                {
                    UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(platform, true);
                    if ((bCheckValidity == false) || (BuildPlatform != null))
                    {
                        OutPlatforms.Add(platform);
                    }
                }
            }

            return (OutPlatforms.Count > 0) ? true : false;
        }

		/// <summary>
		/// Is this a valid configuration
		/// Used primarily for Rocket vs non-Rocket
		/// </summary>
		/// <param name="InConfiguration"></param>
		/// <returns>true if valid, false if not</returns>
		static public bool IsValidConfiguration(UnrealTargetConfiguration InConfiguration)
		{
			if (RunningRocket())
			{
				if (
					(InConfiguration != UnrealTargetConfiguration.Development)
					&& (InConfiguration != UnrealTargetConfiguration.DebugGame)
					&& (InConfiguration != UnrealTargetConfiguration.Shipping)
					)
				{
					return false;
				}
			}
			return true;
		}

		/// <summary>
		/// Is this a valid platform
		/// Used primarily for Rocket vs non-Rocket
		/// </summary>
		/// <param name="InPlatform"></param>
		/// <returns>true if valid, false if not</returns>
		static public bool IsValidPlatform(UnrealTargetPlatform InPlatform)
		{
			if (RunningRocket())
			{
				if(Utils.IsRunningOnMono)
				{
					if(InPlatform != UnrealTargetPlatform.Mac && InPlatform != UnrealTargetPlatform.IOS)
					{
						return false;
					}
				}
				else
				{
					if(InPlatform != UnrealTargetPlatform.Win32 && InPlatform != UnrealTargetPlatform.Win64 && InPlatform != UnrealTargetPlatform.Android)
					{
						return false;
					}
				}
			}
			return true;
		}


		/// <summary>
		/// Is this a valid platform
		/// Used primarily for Rocket vs non-Rocket
		/// </summary>
		/// <param name="InPlatform"></param>
		/// <returns>true if valid, false if not</returns>
		static public bool IsValidDesktopPlatform(UnrealTargetPlatform InPlatform)
		{
			if (Utils.IsRunningOnMono)
			{
				return InPlatform == UnrealTargetPlatform.Mac;
			}
			else
			{
				if (
					(InPlatform != UnrealTargetPlatform.Win32) &&
					(InPlatform != UnrealTargetPlatform.Win64)
					)
				{
					return false;
				}
			}
			return true;
		}

		/// <summary>
		/// See if the given string was pass in on the command line
		/// </summary>
		/// <param name="InString">The argument to check for (must include '-' if expected)</param>
		/// <returns>true if the argument is found, false if not</returns>
		static public bool CommandLineContains(string InString)
		{
			string LowercaseInString = InString.ToLowerInvariant();
			foreach (string Arg in CmdLine)
			{
				string LowercaseArg = Arg.ToLowerInvariant();
				if (LowercaseArg == LowercaseInString)
				{
					return true;
				}
			}

			return false;
		}

		public static void RegisterAllUBTClasses()
		{
			// Find and register all tool chains and build platforms that are present
			Assembly UBTAssembly = Assembly.GetExecutingAssembly();
			if (UBTAssembly != null)
			{
				Log.TraceVerbose("Searching for ToolChains, BuildPlatforms, BuildDeploys and ProjectGenerators...");

				List<System.Type> ProjectGeneratorList = new List<System.Type>();
				var AllTypes = UBTAssembly.GetTypes();
				foreach (var CheckType in AllTypes)
				{
					if (CheckType.IsClass && !CheckType.IsAbstract)
					{
						if (CheckType.IsSubclassOf(typeof(UEToolChain)))
						{
							Log.TraceVerbose("    Registering tool chain    : {0}", CheckType.ToString());
							UEToolChain TempInst = (UEToolChain)(UBTAssembly.CreateInstance(CheckType.FullName, true));
							TempInst.RegisterToolChain();
						}
						else if (CheckType.IsSubclassOf(typeof(UEBuildPlatform)))
						{
							Log.TraceVerbose("    Registering build platform: {0}", CheckType.ToString());
							UEBuildPlatform TempInst = (UEBuildPlatform)(UBTAssembly.CreateInstance(CheckType.FullName, true));
							TempInst.RegisterBuildPlatform();
						}
						else if (CheckType.IsSubclassOf(typeof(UEBuildDeploy)))
						{
							Log.TraceVerbose("    Registering build deploy: {0}", CheckType.ToString());
							UEBuildDeploy TempInst = (UEBuildDeploy)(UBTAssembly.CreateInstance(CheckType.FullName, true));
							TempInst.RegisterBuildDeploy();
						}
						else if (CheckType.IsSubclassOf(typeof(UEPlatformProjectGenerator)))
						{
							ProjectGeneratorList.Add(CheckType);
						}
					}
				}

				// We need to process the ProjectGenerators AFTER all BuildPlatforms have been 
				// registered as they use the BuildPlatform to verify they are legitimate.
				foreach (var ProjType in ProjectGeneratorList)
				{
					Log.TraceVerbose("    Registering project generator: {0}", ProjType.ToString());
					UEPlatformProjectGenerator TempInst = (UEPlatformProjectGenerator)(UBTAssembly.CreateInstance(ProjType.FullName, true));
					TempInst.RegisterPlatformProjectGenerator();
				}
			}
		}

		public static void SetProjectFile(string InProjectFile)
		{
			if (HasUProjectFile() == false)
			{
				UProjectFile = InProjectFile;
				if (UProjectFile.Length > 0)
				{
					bHasUProjectFile = true;
					UProjectPath = Path.GetDirectoryName(UProjectFile);
				}
				else
				{
					bHasUProjectFile = false;
				}
			}
			else
			{
				throw new BuildException("Trying to set the project file to {0} when it is already set to {1}", InProjectFile, UProjectFile);
			}
		}

		private static bool ParseRocketCommandlineArg(string InArg, ref string OutGameName)
		{
			string LowercaseArg = InArg.ToLowerInvariant();
			bool bSetupProject = false;
			
			if (LowercaseArg == "-rocket")
			{
				bRunningRocket = true;
			}
			else if (LowercaseArg.StartsWith("-project="))
			{
				UProjectFile = InArg.Substring(9);
				bSetupProject = true;
			}
			else if (LowercaseArg.EndsWith(".uproject"))
			{
				UProjectFile = InArg;
				bSetupProject = true;
			}
			else if (LowercaseArg == "-buildrocket")
			{
				bBuildingRocket = true;
			}
			else
			{
				return false;
			}

			if (bSetupProject && (UProjectFile.Length > 0))
			{
				bHasUProjectFile = true;
	
				UProjectPath = Path.GetDirectoryName(UProjectFile);
				OutGameName = Path.GetFileNameWithoutExtension(UProjectFile);

				if (Path.IsPathRooted(UProjectPath) == false)
				{
					if (Directory.Exists(UProjectPath) == false)
					{
						// If it is *not* rooted, then we potentially received a path that is 
						// relative to Engine/Binaries/[PLATFORM] rather than Engine/Source
						if (UProjectPath.StartsWith("../") || UProjectPath.StartsWith("..\\"))
						{
							string AlternativeProjectpath = UProjectPath.Substring(3);
							if (Directory.Exists(AlternativeProjectpath))
							{
								UProjectPath = AlternativeProjectpath;
								UProjectFile = UProjectFile.Substring(3);
							}
						}
					}
				}
			}

			return true;
		}

		/// <summary>
		/// Sets up UBT Trace listeners and filters.
		/// </summary>
		private static void InitLogging()
		{
			Trace.Listeners.Add(new ConsoleListener());
			var Filter = new VerbosityFilter();
			foreach (TraceListener Listener in Trace.Listeners)
			{
				Listener.Filter = Filter;
			}
		}

		private static int Main(string[] Arguments)
		{
			InitLogging();

			InitialEnvironment = Environment.GetEnvironmentVariables();
			if (InitialEnvironment.Count < 1)
			{
				throw new BuildException("Environment could not be read");
			}
			// Helpers used for stats tracking.
			StartTime = DateTime.UtcNow;

			Telemetry.Initialize();

			// Copy off the arguments to allow checking for command-line arguments elsewhere
			CmdLine = new List<string>(Arguments);

			foreach (string Arg in Arguments)
			{
				string TempGameName = null;
				if (ParseRocketCommandlineArg(Arg, ref TempGameName) == true)
				{
					// This is to allow relative paths for the project file
					Log.TraceVerbose("UBT Running for Rocket: " + UProjectFile);
				}
			}

			// Change the working directory to be the Engine/Source folder. We are running from Engine/Binaries/DotNET
			string EngineSourceDirectory = Path.Combine(Utils.GetExecutingAssemblyDirectory(), "..", "..", "..", "Engine", "Source");

			//@todo.Rocket: This is a workaround for recompiling game code in editor
			// The working directory when launching is *not* what we would expect
			if (Directory.Exists(EngineSourceDirectory) == false)
			{
				// We are assuming UBT always runs from <>/Engine/...
				EngineSourceDirectory = Path.GetDirectoryName(Utils.GetExecutingAssemblyLocation());
				EngineSourceDirectory = EngineSourceDirectory.Replace("\\", "/");
				Int32 EngineIdx = EngineSourceDirectory.IndexOf("/Engine/");
				if (EngineIdx != 0)
				{
					EngineSourceDirectory = EngineSourceDirectory.Substring(0, EngineIdx + 8);
					EngineSourceDirectory += "Source";
				}
			}
			Directory.SetCurrentDirectory(EngineSourceDirectory);

			// Build the list of game projects that we know about. When building from the editor (for hot-reload) or for Rocket projects, we require the 
			// project file to be passed in. Otherwise we scan for projects in directories named in UE4Games.uprojectdirs.
			if (HasUProjectFile())
			{
				string SourceFolder = Path.Combine(Path.GetDirectoryName(GetUProjectFile()), "Source");
				UProjectInfo.AddProject(GetUProjectFile(), Directory.Exists(SourceFolder));
			}
			else if(!RunningRocket())
			{
				UProjectInfo.FillProjectInfo();
			}

			bool bSuccess = true;

			// Reset early so we can access BuildConfiguration even before RunUBT() is called
			BuildConfiguration.Reset();
			UEBuildConfiguration.Reset();

			Log.TraceVerbose("UnrealBuildTool (DEBUG OUTPUT MODE)");
			Log.TraceVerbose("Command-line: {0}", String.Join(" ", Arguments));

			bool bRunCopyrightVerification = false;
			bool bDumpToFile = false;
			bool bCheckThirdPartyHeaders = false;
			// List of all platforms to skip when bBuildAll is true.
			List<UnrealTargetPlatform> PlatformsToExclude = new List<UnrealTargetPlatform>();


			// @todo: Ideally we never need to Mutex unless we are invoked with the same target project,
			// in the same branch/path!  This would allow two clientspecs to build at the same time (even though we need
			// to make sure that XGE is only used once at a time)
			bool bUseMutex = true;
			{
				int NoMutexArgumentIndex;
				if (Utils.ParseCommandLineFlag(Arguments, "-NoMutex", out NoMutexArgumentIndex))
				{
					bUseMutex = false;
				}
			}

			// Don't allow simultaneous execution of Unreal Built Tool. Multi-selection in the UI e.g. causes this and you want serial
			// execution in this case to avoid contention issues with shared produced items.
			bool bCreatedMutex = false;
			{
				Mutex SingleInstanceMutex = null;
				if (bUseMutex)
				{
					int LocationHash = Utils.GetExecutingAssemblyLocation().GetHashCode();
					SingleInstanceMutex = new Mutex(true, "Global/UnrealBuildTool_Mutex_" + LocationHash.ToString(), out bCreatedMutex);
					if (!bCreatedMutex)
					{
						// If this instance didn't create the mutex, wait for the existing mutex to be released by the mutex's creator.
						DateTime MutexWaitStartTime = DateTime.UtcNow;
						SingleInstanceMutex.WaitOne();
						MutexWaitTime = DateTime.UtcNow - MutexWaitStartTime;
					}
				}

				try
				{
					string GameName = null;
					string ConfigName = null;
					string PlatformName = null;
					var bGenerateVCProjectFiles = false;
					var bGenerateXcodeProjectFiles = false;
					var bValidPlatformsOnly = false;
					var bSpecificModulesOnly = false;

					// We need to be able to identify if one of the arguments is the platform...
					// Leverage the existing parser function in UEBuildTarget to get this information.
					UnrealTargetPlatform CheckPlatform;
					UnrealTargetConfiguration CheckConfiguration;
					UEBuildTarget.ParsePlatformAndConfiguration(Arguments, out CheckPlatform, out CheckConfiguration, false);
					//@todo SAS: Add a true Debug mode!
					if (RunningRocket())
					{
						RocketConfiguration = CheckConfiguration;
						
						// Only Development and Shipping are supported for engine modules
						if( CheckConfiguration != UnrealTargetConfiguration.Development && CheckConfiguration != UnrealTargetConfiguration.Shipping )
						{
							CheckConfiguration = UnrealTargetConfiguration.Development;
						}
					}

					foreach (var Arg in Arguments)
					{
						string LowercaseArg = Arg.ToLowerInvariant();
			            const string OnlyPlatformSpecificForArg = "-onlyplatformspecificfor=";

						if (ParseRocketCommandlineArg(Arg, ref GameName))
						{
							// Already handled at startup. Calling now just to properly set the game name
							continue;
						}
                        else if (LowercaseArg == "-skipnonhostplatforms")
                        {
                            bSkipNonHostPlatforms = true;
                        }
                        else if (LowercaseArg.StartsWith(OnlyPlatformSpecificForArg))
                        {
                            string OtherPlatform = LowercaseArg.Substring(OnlyPlatformSpecificForArg.Length);
                            foreach (UnrealTargetPlatform platform in Enum.GetValues(typeof(UnrealTargetPlatform)))
                            {
                                if (OtherPlatform == platform.ToString().ToLowerInvariant())
                                {
                                    OnlyPlatformSpecificFor = platform;
                                }
                            }
                            if (OnlyPlatformSpecificFor == UnrealTargetPlatform.Unknown)
                            {
                                throw new BuildException("Could not parse {0}", LowercaseArg);
                            }
                        }
                        else if (LowercaseArg.StartsWith("-projectfile"))
						{
							bGenerateVCProjectFiles = true;
							ProjectFileGenerator.bGenerateProjectFiles = true;
						}
						else if (LowercaseArg.StartsWith("-xcodeprojectfile"))
						{
							bGenerateXcodeProjectFiles = true;
							ProjectFileGenerator.bGenerateProjectFiles = true;
						}
						else if (LowercaseArg == "-validplatformsonly")
						{
							bValidPlatformsOnly = true;
						}
						else if (LowercaseArg == "development" || LowercaseArg == "debug" || LowercaseArg == "shipping" || LowercaseArg == "test" || LowercaseArg == "debuggame")
						{
							ConfigName = Arg;
						}
						else if (LowercaseArg == "-modulewithsuffix")
						{
							bSpecificModulesOnly = true;
							continue;
						}
						else if (LowercaseArg == "-forceheadergeneration")
						{
							// Don't reset this as we don't recheck the args in RunUBT
							UEBuildConfiguration.bForceHeaderGeneration = true;
						}
						else if (LowercaseArg == "-nobuilduht")
						{
							UEBuildConfiguration.bDoNotBuildUHT = true;
						}
						else if (LowercaseArg == "-failifgeneratedcodechanges")
						{
							UEBuildConfiguration.bFailIfGeneratedCodeChanges = true;
						}
						else if (LowercaseArg == "-copyrightverification")
						{
							bRunCopyrightVerification = true;
						}
						else if (LowercaseArg == "-dumptofile")
						{
							bDumpToFile = true;
						}
						else if (LowercaseArg == "-check3rdpartyheaders")
						{
							bCheckThirdPartyHeaders = true;
						}
						else if (LowercaseArg == "-clean")
						{
							UEBuildConfiguration.bCleanProject = true;
						}
						else if (LowercaseArg == "-prepfordeploy")
						{
							UEBuildConfiguration.bPrepForDeployment = true;
						}
						else if (LowercaseArg == "-generatemanifest")
						{
							// Generate a manifest file containing all the files required to be in Perforce
							UEBuildConfiguration.bGenerateManifest = true;
						}
						else if (LowercaseArg == "-mergemanifests")
						{
							// Whether to add to the existing manifest (if it exists), or start afresh
							UEBuildConfiguration.bMergeManifests = true;
						}
						else if (LowercaseArg == "-noxge")
						{
							BuildConfiguration.bAllowXGE = false;
						}
						else if (LowercaseArg == "-xgeexport")
						{
							BuildConfiguration.bXGEExport = true;
							BuildConfiguration.bAllowXGE = true;
						}
						else if (LowercaseArg == "-nosimplygon")
						{
							UEBuildConfiguration.bCompileSimplygon = false;
						}
						else if (LowercaseArg == "-nospeedtree")
						{
							UEBuildConfiguration.bCompileSpeedTree = false;
						}
						else if (CheckPlatform.ToString().ToLowerInvariant() == LowercaseArg)
						{
							// It's the platform set...
							PlatformName = Arg;
						}
						else
						{
							bool bPlatformExclusionArg = false;
							// Check if this is one of the platforms to exclude params: -nowin32, -nomac, etc.
							foreach (UnrealTargetPlatform platform in Enum.GetValues(typeof(UnrealTargetPlatform)))
							{
								string ExcludePlatformArg = "-no" + platform.ToString().ToLowerInvariant();
								if (LowercaseArg == ExcludePlatformArg)
								{
									PlatformsToExclude.Add(platform);
									bPlatformExclusionArg = true;
									break;
								}
							}

							if (!bPlatformExclusionArg)
							{
								// This arg may be a game name. Check for the existence of a game folder with this name.
								// "Engine" is not a valid game name.
								if (LowercaseArg != "engine" && Directory.Exists(Path.Combine(ProjectFileGenerator.RootRelativePath, Arg, "Config")))
								{
									GameName = Arg;
									Log.TraceVerbose("CommandLine: Found game name '{0}'", GameName);
								}
								else if (LowercaseArg == "rocket")
								{
									GameName = Arg;
									Log.TraceVerbose("CommandLine: Found game name '{0}'", GameName);
								}
							}
						}
					}

					// Send an event with basic usage dimensions
					Telemetry.SendEvent("CommonAttributes.2",
						// @todo No platform independent way to do processor speed and count. Defer for now. Mono actually has ways to do this but needs to be tested.
						"ProcessorCount", Environment.ProcessorCount.ToString(),
						"ComputerName", Environment.MachineName,
						"User", Environment.UserName,
						"Domain", Environment.UserDomainName,
						"CommandLine", string.Join(" ", Arguments),
						"UBT Action", bGenerateVCProjectFiles 
							? "GenerateVCProjectFiles" 
							: bGenerateXcodeProjectFiles 
								? "GenerateXcodeProjectFiles" 
								: bRunCopyrightVerification
									? "RunCopyrightVerification"
									: "Build",
						"Platform", CheckPlatform.ToString(),
						"Configuration", CheckConfiguration.ToString(),
						"IsRocket", bRunningRocket.ToString(),
						"EngineVersion", Utils.GetEngineVersionFromObjVersionCPP().ToString()
						);



					if (bValidPlatformsOnly == true)
					{
						// We turned the flag on if generating projects so that the
						// build platforms would know we are doing so... In this case,
						// turn it off to only generate projects for valid platforms.
						ProjectFileGenerator.bGenerateProjectFiles = false;
					}

					// Find and register all tool chains, build platforms, etc. that are present
					RegisterAllUBTClasses();
					ProjectFileGenerator.bGenerateProjectFiles = false;

					// now that we know the available platforms, we can delete other platforms' junk. if we're only building specific modules from the editor, don't touch anything else (it may be in use).
					if (!bSpecificModulesOnly)
					{
						JunkDeleter.DeleteJunk();
					}
	
					if (bGenerateVCProjectFiles || bGenerateXcodeProjectFiles)
					{
						bSuccess = true;
						if (bGenerateVCProjectFiles)
						{
							bSuccess &= GenerateProjectFiles(new VCProjectFileGenerator(), Arguments);
						}
						if (bGenerateXcodeProjectFiles)
						{
							bSuccess &= GenerateProjectFiles(new XcodeProjectFileGenerator(), Arguments);
						}
					}
					else if (bRunCopyrightVerification)
					{
						CopyrightVerify.RunCopyrightVerification(EngineSourceDirectory, GameName, bDumpToFile);
						Log.TraceInformation("Completed... exiting.");
					}
					else
					{
						// Check if any third party headers are included from public engine headers.
						if (bCheckThirdPartyHeaders)
						{
							ThirdPartyHeaderFinder.FindThirdPartyIncludes(CheckPlatform, CheckConfiguration);
						}

						// Build our project
						if (bSuccess)
						{
							if (UEBuildConfiguration.bPrepForDeployment == false)
							{
								// If we are only prepping for deployment, assume the build already occurred.
								bSuccess = RunUBT(Arguments);
							}
							else
							{
								UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(CheckPlatform, true);
								if (BuildPlatform != null)
								{
									// Setup environment wasn't called, so set the flag
									BuildConfiguration.bDeployAfterCompile = BuildPlatform.RequiresDeployPrepAfterCompile();
									BuildConfiguration.PlatformIntermediateFolder = Path.Combine(BuildConfiguration.BaseIntermediateFolder, CheckPlatform.ToString() + BuildPlatform.GetActiveArchitecture());
								}
							}

							// If we build w/ bXGEExport true, we didn't REALLY build at this point, 
							// so don't bother with doing the PrepTargetForDeployment call. 
							if ((bSuccess == true) && (BuildConfiguration.bDeployAfterCompile == true) && (BuildConfiguration.bXGEExport == false) &&
								(UEBuildConfiguration.bGenerateManifest == false) && (UEBuildConfiguration.bCleanProject == false))
							{
								UEBuildDeploy DeployHandler = UEBuildDeploy.GetBuildDeploy(CheckPlatform);
								if (DeployHandler != null)
								{
									// We need to be able to identify the Target.Type we can derive it from the Arguments.
									BuildConfiguration.bFlushBuildDirOnRemoteMac = false;
									UEBuildTarget CheckTarget = UEBuildTarget.CreateTarget(Arguments);
									CheckTarget.SetupGlobalEnvironment();
									string AppName = CheckTarget.AppName;
									if ((CheckTarget.Rules.Type == TargetRules.TargetType.Game) || 
										(CheckTarget.Rules.Type == TargetRules.TargetType.Server) || 
										(CheckTarget.Rules.Type == TargetRules.TargetType.Client))
									{
										CheckTarget.AppName = CheckTarget.GameName;
									}
									DeployHandler.PrepTargetForDeployment(CheckTarget);
								}
							}
						}
					}
					// Print some performance info
					var BuildDuration = (DateTime.UtcNow - StartTime - MutexWaitTime).TotalSeconds;
					if (BuildConfiguration.bPrintPerformanceInfo)
					{
						Log.TraceInformation("GetIncludes time: " + CPPEnvironment.TotalTimeSpentGettingIncludes + "s (" + CPPEnvironment.TotalIncludesRequested + " includes)" );
						Log.TraceInformation("DirectIncludes cache miss time: " + CPPEnvironment.DirectIncludeCacheMissesTotalTime + "s (" + CPPEnvironment.TotalDirectIncludeCacheMisses + " misses)" );
						Log.TraceInformation("FindIncludePaths calls: " + CPPEnvironment.TotalFindIncludedFileCalls + " (" + CPPEnvironment.IncludePathSearchAttempts + " searches)" );
						Log.TraceInformation("Total FileItems: {0} ({1} missing)", FileItem.TotalFileItemCount, FileItem.MissingFileItemCount);
						Log.TraceInformation("Include Resolves: {0} ({1} misses, {2:0.00}%)", CPPEnvironment.TotalDirectIncludeResolves, CPPEnvironment.TotalDirectIncludeResolveCacheMisses, (float)CPPEnvironment.TotalDirectIncludeResolveCacheMisses / (float)CPPEnvironment.TotalDirectIncludeResolves * 100);
						Log.TraceInformation("Total FileItems: {0} ({1} missing)", FileItem.TotalFileItemCount, FileItem.MissingFileItemCount);

						Log.TraceInformation("Execution time: {0}", BuildDuration);
					}

					Telemetry.SendEvent("PerformanceInfo.2",
						"TotalExecutionTimeSec", BuildDuration.ToString("0.00"),
						"MutexWaitTimeSec", MutexWaitTime.TotalSeconds.ToString("0.00"),
						"TotalTimeSpentGettingIncludesSec", CPPEnvironment.TotalTimeSpentGettingIncludes.ToString("0.00"),
						"TotalIncludesRequested", CPPEnvironment.TotalIncludesRequested.ToString(),
						"DirectIncludeCacheMissesTotalTimeSec", CPPEnvironment.DirectIncludeCacheMissesTotalTime.ToString("0.00"),
						"TotalDirectIncludeCacheMisses", CPPEnvironment.TotalDirectIncludeCacheMisses.ToString(),
						"TotalFindIncludedFileCalls", CPPEnvironment.TotalFindIncludedFileCalls.ToString(),
						"IncludePathSearchAttempts", CPPEnvironment.IncludePathSearchAttempts.ToString(),
						"TotalFileItemCount", FileItem.TotalFileItemCount.ToString(),
						"MissingFileItemCount", FileItem.MissingFileItemCount.ToString()
						);
				}
				catch (Exception Exception)
				{
					Log.TraceError("UnrealBuildTool Exception: " + Exception);
					bSuccess = false;
				}

				if (bUseMutex)
				{
					// Release the mutex to avoid the abandoned mutex timeout.
					SingleInstanceMutex.ReleaseMutex();
					SingleInstanceMutex.Dispose();
					SingleInstanceMutex = null;
				}
			}

			Telemetry.Shutdown();

			// Print some performance info
			Log.TraceVerbose("Execution time: {0}", (DateTime.UtcNow - StartTime - MutexWaitTime).TotalSeconds);

			return bSuccess ? 0 : 1;
		}


		/// <summary>
		/// Generates project files.  Can also be used to generate projects "in memory", to allow for building
		/// targets without even having project files at all.
		/// </summary>
		/// <param name="Generator">Project generator to use</param>
		/// <param name="Arguments">Command-line arguments</param>
		/// <returns>True if successful</returns>
		public static bool GenerateProjectFiles(ProjectFileGenerator Generator, string[] Arguments)
		{
			bool bSuccess;
			ProjectFileGenerator.bGenerateProjectFiles = true;
			Generator.GenerateProjectFiles(Arguments, out bSuccess);
			ProjectFileGenerator.bGenerateProjectFiles = false;
			return bSuccess;
		}




		// @todo: Ideally get rid of RunUBT() and all of the Clear/Reset stuff!
		public static bool RunUBT(string[] Arguments)
		{
			bool bSuccess = true;

			// Reset global configurations
			ResetAllActions();
			CPPEnvironment.Reset();
			BuildConfiguration.Reset();
			UEBuildConfiguration.Reset();

			ParseBuildConfigurationFlags(Arguments);

			// We need to allow the target platform to perform the 'reset' as well...
			UnrealTargetPlatform ResetPlatform = UnrealTargetPlatform.Unknown;
			UnrealTargetConfiguration ResetConfiguration;
			UEBuildTarget.ParsePlatformAndConfiguration(Arguments, out ResetPlatform, out ResetConfiguration);
			if (RunningRocket())
			{
				RocketConfiguration = ResetConfiguration;

				// Only Development and Shipping are supported for engine modules
				if( ResetConfiguration != UnrealTargetConfiguration.Development && ResetConfiguration != UnrealTargetConfiguration.Shipping )
				{
					ResetConfiguration = UnrealTargetConfiguration.Development;
				}
			}
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(ResetPlatform);

			// now that we have the platform, we can set the intermediate path to include the platform/architecture name
			BuildConfiguration.PlatformIntermediateFolder = Path.Combine(BuildConfiguration.BaseIntermediateFolder, ResetPlatform.ToString() + BuildPlatform.GetActiveArchitecture());

			BuildPlatform.ResetBuildConfiguration(ResetPlatform, ResetConfiguration);

			var Targets = new List<UEBuildTarget>();
			string ExecutorName = "Unknown";

			try
			{
				List<string[]> TargetSettings = ParseCommandLineFlags(Arguments);

				int ArgumentIndex;
				// action graph implies using the dependency resolve cache
				bool GeneratingActionGraph = Utils.ParseCommandLineFlag(Arguments, "-graph", out ArgumentIndex );
				if (GeneratingActionGraph)
				{
					BuildConfiguration.bUseIncludeDependencyResolveCache = true;
				}

				// Build action lists for all passed in targets.
				var OutputItemsForAllTargets = new List<FileItem>();
				foreach (string[] TargetSetting in TargetSettings)
				{
					var TargetStartTime = DateTime.UtcNow;
					var Target = UEBuildTarget.CreateTarget(TargetSetting);
					if ((Target == null) && (UEBuildConfiguration.bCleanProject))
					{
						continue;
					}
					Targets.Add(Target);

					// Load the include dependency cache.
					if (CPPEnvironment.IncludeDependencyCache == null)
					{
						CPPEnvironment.IncludeDependencyCache = DependencyCache.Create(
							DependencyCache.GetDependencyCachePathForTarget(Target)
							);
					}
				
					var TargetOutputItems = Target.Build();
					OutputItemsForAllTargets.AddRange( TargetOutputItems );

					if ( (BuildConfiguration.bXGEExport && UEBuildConfiguration.bGenerateManifest) || (!ProjectFileGenerator.bGenerateProjectFiles && !UEBuildConfiguration.bGenerateManifest && !UEBuildConfiguration.bCleanProject))
					{
						// We don't currently support building game targets in rocket.
						if (UnrealBuildTool.RunningRocket() && Target.Rules != null && Target.Rules.Type == TargetRules.TargetType.Game)
						{
							throw new BuildException(
								"You currently can not build a game target in Rocket.\nTry again in a future release.\nFor now, build and run your editor project."
							);
						}

						// Generate an action graph if we were asked to do that.  The graph generation needs access to the include dependency cache, so
						// we generate it before saving and cleaning that up.
						if( GeneratingActionGraph )
						{
							// The graph generation feature currently only works with a single target at a time.  This is because of how we need access
							// to include dependencies for the target, but those aren't kept around as we process multiple targets
							if( TargetSettings.Count != 1 )
							{
								throw new BuildException( "ERROR: The '-graph' option only works with a single target at a time" );
							}
							LinkActionsAndItems();
							var ActionsToExecute = AllActions;

							var VisualizationType = ActionGraphVisualizationType.OnlyCPlusPlusFilesAndHeaders;
							SaveActionGraphVisualization( Path.Combine( BuildConfiguration.BaseIntermediatePath, Target.GetTargetName() + ".gexf" ), Target.GetTargetName(), VisualizationType, ActionsToExecute );
						}

						// Save the include dependency cache.
						if (CPPEnvironment.IncludeDependencyCache != null)
						{
							CPPEnvironment.IncludeDependencyCache.Save();
							CPPEnvironment.IncludeDependencyCache = null;
						}
					}

					var TargetBuildTime = (DateTime.UtcNow - TargetStartTime).TotalSeconds;

					// Send out telemetry for this target
					Telemetry.SendEvent("TargetBuildStats.2",
						"AppName", Target.AppName,
						"GameName", Target.GameName,
						"Platform", Target.Platform.ToString(),
						"Configuration", Target.Configuration.ToString(),
						"CleanTarget", UEBuildConfiguration.bCleanProject.ToString(),
						"Monolithic", Target.ShouldCompileMonolithic().ToString(),
						"CreateDebugInfo", Target.IsCreatingDebugInfo().ToString(),
						"TargetType", Target.Rules.Type.ToString(),
						"TargetCreateTimeSec", TargetBuildTime.ToString("0.00")
						);
				}

				if ((BuildConfiguration.bXGEExport && UEBuildConfiguration.bGenerateManifest) || (!GeneratingActionGraph && !ProjectFileGenerator.bGenerateProjectFiles && !UEBuildConfiguration.bGenerateManifest && !UEBuildConfiguration.bCleanProject))
				{
					// Plan the actions to execute for the build.
					List<Action> ActionsToExecute = GetActionsToExecute(OutputItemsForAllTargets, Targets);

					// Display some stats to the user.
					Log.TraceVerbose(
							"{0} actions, {1} outdated and requested actions",
							AllActions.Count,
							ActionsToExecute.Count
							);

					UEToolChain ToolChain = UEToolChain.GetPlatformToolChain(BuildPlatform.GetCPPTargetPlatform(ResetPlatform));
					ToolChain.PreBuildSync();

					// Execute the actions.
					bSuccess = ExecuteActions(ActionsToExecute, out ExecutorName);

					// if the build succeeded, do any needed syncing
					if (bSuccess)
					{
						foreach (UEBuildTarget Target in Targets)
						{
							ToolChain.PostBuildSync(Target);
						}
					}
				}
			}
			catch (BuildException Exception)
			{
				// Output the message only, without the call stack
				Log.TraceInformation(Exception.Message);
				bSuccess = false;
			}
			catch (Exception Exception)
			{
				Log.TraceInformation("ERROR: {0}", Exception);
				bSuccess = false;
			}

			// Figure out how long we took to execute.
			double BuildDuration = (DateTime.UtcNow - StartTime - MutexWaitTime).TotalSeconds;
			if (ExecutorName == "Local")
			{
				Log.TraceInformation("Cumulative action seconds ({0} processors): {1:0.00} building projects, {2:0.00} compiling, {3:0.00} creating app bundles, {4:0.00} generating debug info, {5:0.00} linking, {6:0.00} other",
					System.Environment.ProcessorCount,
					TotalBuildProjectTime,
					TotalCompileTime,
					TotalCreateAppBundleTime,
					TotalGenerateDebugInfoTime,
					TotalLinkTime,
					TotalOtherActionsTime
				);
				Telemetry.SendEvent("BuildStatsTotal.2",
					"ExecutorName", ExecutorName,
					"TotalUBTWallClockTimeSec", BuildDuration.ToString("0.00"),
					"TotalBuildProjectThreadTimeSec", TotalBuildProjectTime.ToString("0.00"),
					"TotalCompileThreadTimeSec", TotalCompileTime.ToString("0.00"),
					"TotalCreateAppBundleThreadTimeSec", TotalCreateAppBundleTime.ToString("0.00"),
					"TotalGenerateDebugInfoThreadTimeSec", TotalGenerateDebugInfoTime.ToString("0.00"),
					"TotalLinkThreadTimeSec", TotalLinkTime.ToString("0.00"),
					"TotalOtherActionsThreadTimeSec", TotalOtherActionsTime.ToString("0.00")
					);

				Log.TraceInformation("UBT execution time: {0:0.00} seconds", BuildDuration);

				// reset statistics
				TotalBuildProjectTime = 0;
				TotalCompileTime = 0;
				TotalCreateAppBundleTime = 0;
				TotalGenerateDebugInfoTime = 0;
				TotalLinkTime = 0;
				TotalOtherActionsTime = 0;
			}
			else
			{
				if (ExecutorName == "XGE")
				{
					Log.TraceInformation("XGE execution time: {0:0.00} seconds", BuildDuration);
				}
				Telemetry.SendEvent("BuildStatsTotal.2",
					"ExecutorName", ExecutorName,
					"TotalUBTWallClockTimeSec", BuildDuration.ToString("0.00")
					);
			}

			return bSuccess;
		}

		private static void ParseBuildConfigurationFlags(string[] Arguments)
		{
			int ArgumentIndex = 0;

			// Parse optional command-line flags.
			if (Utils.ParseCommandLineFlag(Arguments, "-verbose", out ArgumentIndex))
			{
				BuildConfiguration.bPrintDebugInfo = true;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-noxge", out ArgumentIndex))
			{
				BuildConfiguration.bAllowXGE = false;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-noxgemonitor", out ArgumentIndex))
			{
				BuildConfiguration.bShowXGEMonitor = false;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-stresstestunity", out ArgumentIndex))
			{
				BuildConfiguration.bStressTestUnity = true;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-disableunity", out ArgumentIndex))
			{
				BuildConfiguration.bUseUnityBuild = false;
			}
            if (Utils.ParseCommandLineFlag(Arguments, "-forceunity", out ArgumentIndex))
            {
                BuildConfiguration.bForceUnityBuild = true;
            }

            // New Monolithic Graphics drivers replace D3D and are *mostly* API compatible
            if (Utils.ParseCommandLineFlag(Arguments, "-monolithicdrivers", out ArgumentIndex))
            {
				BuildConfiguration.bUseMonolithicGraphicsDrivers = true;
            }
            if (Utils.ParseCommandLineFlag(Arguments, "-nomonolithicdrivers", out ArgumentIndex))
            {
				BuildConfiguration.bUseMonolithicGraphicsDrivers = false;
            }
            // New Monolithic Graphics drivers have optional "fast calls" replacing various D3d functions
            if (Utils.ParseCommandLineFlag(Arguments, "-fastmonocalls", out ArgumentIndex))
            {
                BuildConfiguration.bUseMonolithicGraphicsDrivers = true;    //can't have fast calls without mono !
                BuildConfiguration.bUseFastMonoCalls = true;
            }
            if (Utils.ParseCommandLineFlag(Arguments, "-nofastmonocalls", out ArgumentIndex))
            {
                BuildConfiguration.bUseFastMonoCalls = false;
            }
			if (BuildConfiguration.bUseMonolithicGraphicsDrivers == false)
			{
				BuildConfiguration.bUseFastMonoCalls = false;
			}
            
            if (Utils.ParseCommandLineFlag(Arguments, "-uniqueintermediate", out ArgumentIndex))
			{
				BuildConfiguration.BaseIntermediateFolder = "Intermediate/Build/" + BuildGuid + "/";
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-nopch", out ArgumentIndex))
			{
				BuildConfiguration.bUsePCHFiles = false;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-skipActionHistory", out ArgumentIndex))
			{
				BuildConfiguration.bUseActionHistory = false;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-noLTCG", out ArgumentIndex))
			{
				BuildConfiguration.bAllowLTCG = false;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-nopdb", out ArgumentIndex))
			{
				BuildConfiguration.bUsePDBFiles = false;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-deploy", out ArgumentIndex))
			{
				BuildConfiguration.bDeployAfterCompile = true;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-flushmac", out ArgumentIndex))
			{
				BuildConfiguration.bFlushBuildDirOnRemoteMac = true;
			}

			if (Utils.ParseCommandLineFlag(Arguments, "-nodebuginfo", out ArgumentIndex))
			{
				BuildConfiguration.bDisableDebugInfo = true;
			}
			else if (Utils.ParseCommandLineFlag(Arguments, "-forcedebuginfo", out ArgumentIndex))
			{
				BuildConfiguration.bDisableDebugInfo = false;
				BuildConfiguration.bOmitPCDebugInfoInDevelopment = false;
			}

			foreach(string Argument in Arguments)
			{
				const string WriteTargetInfoArg = "-writetargetinfo=";
				if (Argument.ToLowerInvariant().StartsWith(WriteTargetInfoArg))
				{
					BuildConfiguration.WriteTargetInfoPath = Argument.Substring(WriteTargetInfoArg.Length);
					break;
				}
			}
		}

		/**
		 * Parses the passed in command line for build configuration overrides.
		 * 
		 * @param	Arguments	List of arguments to parse
		 * @return	List of build target settings
		 */
		private static List<string[]> ParseCommandLineFlags(string[] Arguments)
		{
			var TargetSettings = new List<string[]>();
			int ArgumentIndex = 0;

			// Log command-line arguments.
			if (BuildConfiguration.bPrintDebugInfo)
			{
				Console.Write("Command-line arguments: ");
				foreach (string Argument in Arguments)
				{
					Console.Write("{0} ", Argument);
				}
				Log.TraceInformation("");
			}

			ParseBuildConfigurationFlags(Arguments);

			if (Utils.ParseCommandLineFlag(Arguments, "-targets", out ArgumentIndex))
			{
				if (ArgumentIndex + 1 >= Arguments.Length)
				{
					throw new BuildException("Expected filename after -targets argument, but found nothing.");
				}
				// Parse lines from the referenced file into target settings.
				var Lines = File.ReadAllLines(Arguments[ArgumentIndex + 1]);
				foreach (string Line in Lines)
				{
					if (Line != "" && Line[0] != ';')
					{
						TargetSettings.Add(Line.Split(' '));
					}
				}
			}
			// Simply use full command line arguments as target setting if not otherwise overriden.
			else
			{
				TargetSettings.Add(Arguments);
			}

			return TargetSettings;
		}
	}
}

