// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	public abstract class UEBuildPlatform
	{
		public static Dictionary<UnrealTargetPlatform, UEBuildPlatform> BuildPlatformDictionary = new Dictionary<UnrealTargetPlatform, UEBuildPlatform>();

		// a mapping of a group to the platforms in the group (ie, Microsoft contains Win32 and Win64)
		static Dictionary<UnrealPlatformGroup, List<UnrealTargetPlatform>> PlatformGroupDictionary = new Dictionary<UnrealPlatformGroup, List<UnrealTargetPlatform>>();

        /** Name of the file that holds currently install SDK version string */
        static string CurrentlyInstalledSDKStringManifest = "CurrentlyInstalled.txt";

        /** name of the file that holds the last succesfully run SDK setup script version */
        static string LastRunScriptVersionManifest = "CurrentlyInstalled.Version.txt";

        /** Name of the file that holds environment variables of current SDK */
        static string SDKEnvironmentVarsFile = "OutputEnvVars.txt";

        /** Enum describing types of hooks a platform SDK can have */
        public enum SDKHookType
        {
            Install,
            Uninstall
        };

		public enum SDKStatus
		{
			Valid,			// Desired SDK is installed and set up.
			Invalid,		// Could not find the desired SDK, SDK setup failed, etc.
			Unknown,		// Not enough info to determine SDK state. 
		};

		public static readonly string SDKRootEnvVar = "UE_SDKS_ROOT";

        /**
         * Attempt to convert a string to an UnrealTargetPlatform enum entry
         * 
         * @return UnrealTargetPlatform.Unknown on failure (the platform didn't match the enum)
         */
		public static UnrealTargetPlatform ConvertStringToPlatform(string InPlatformName)
		{
			// special case x64, to not break anything
			// @todo: Is it possible to remove this hack?
			if (InPlatformName.Equals("X64", StringComparison.InvariantCultureIgnoreCase))
			{
				return UnrealTargetPlatform.Win64;
			}

			// we can't parse the string into an enum because Enum.Parse is case sensitive, so we loop over the enum
			// looking for matches
			foreach (string PlatformName in Enum.GetNames(typeof(UnrealTargetPlatform)))
			{
				if (InPlatformName.Equals(PlatformName, StringComparison.InvariantCultureIgnoreCase))
				{
					// convert the known good enum string back to the enum value
					return (UnrealTargetPlatform)Enum.Parse(typeof(UnrealTargetPlatform), PlatformName);
				}
			}
			return UnrealTargetPlatform.Unknown;
		}

		/**
		 *	Register the given platforms UEBuildPlatform instance
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform to register with
		 *	@param	InBuildPlatform		The UEBuildPlatform instance to use for the InPlatform
		 */
		public static void RegisterBuildPlatform(UnrealTargetPlatform InPlatform, UEBuildPlatform InBuildPlatform)
		{
			if (BuildPlatformDictionary.ContainsKey(InPlatform) == true)
			{
				Log.TraceWarning("RegisterBuildPlatform Warning: Registering build platform {0} for {1} when it is already set to {2}",
					InBuildPlatform.ToString(), InPlatform.ToString(), BuildPlatformDictionary[InPlatform].ToString());
				BuildPlatformDictionary[InPlatform] = InBuildPlatform;
			}
			else
			{
				BuildPlatformDictionary.Add(InPlatform, InBuildPlatform);
			}
		}

		/**
		 *	Unregister the given platform
		 */
		public static void UnregisterBuildPlatform(UnrealTargetPlatform InPlatform)
		{
			if (BuildPlatformDictionary.ContainsKey(InPlatform) == true)
			{
				BuildPlatformDictionary.Remove(InPlatform);
			}
		}

		/**
		 * Assign a platform as a member of the given group
		 */
		public static void RegisterPlatformWithGroup(UnrealTargetPlatform InPlatform, UnrealPlatformGroup InGroup)
		{
			// find or add the list of groups for this platform
			PlatformGroupDictionary.GetOrAddNew(InGroup).Add(InPlatform);
		}

		/**
		 * Retrieve the list of platforms in this group (if any)
		 */
		public static List<UnrealTargetPlatform> GetPlatformsInGroup(UnrealPlatformGroup InGroup)
		{
			List<UnrealTargetPlatform> PlatformList;
			PlatformGroupDictionary.TryGetValue(InGroup, out PlatformList);
			return PlatformList;
		}

		/**
		 *	Retrieve the UEBuildPlatform instance for the given TargetPlatform
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	bInAllowFailure		If true, do not throw an exception and return null
		 *	
		 *	@return	UEBuildPlatform		The instance of the build platform
		 */
		public static UEBuildPlatform GetBuildPlatform(UnrealTargetPlatform InPlatform, bool bInAllowFailure = false)
		{
			if (BuildPlatformDictionary.ContainsKey(InPlatform) == true)
			{
				return BuildPlatformDictionary[InPlatform];
			}
			if (bInAllowFailure == true)
			{
				return null;
			}
			throw new BuildException("GetBuildPlatform: No BuildPlatform found for {0}", InPlatform.ToString());
		}

		/**
		 *	Retrieve the UEBuildPlatform instance for the given CPPTargetPlatform
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	bInAllowFailure		If true, do not throw an exception and return null
		 *	
		 *	@return	UEBuildPlatform		The instance of the build platform
		 */
		public static UEBuildPlatform GetBuildPlatformForCPPTargetPlatform(CPPTargetPlatform InPlatform, bool bInAllowFailure = false)
		{
			UnrealTargetPlatform UTPlatform = UEBuildTarget.CPPTargetPlatformToUnrealTargetPlatform(InPlatform);
			if (BuildPlatformDictionary.ContainsKey(UTPlatform) == true)
			{
				return BuildPlatformDictionary[UTPlatform];
			}
			if (bInAllowFailure == true)
			{
				return null;
			}
			throw new BuildException("UEBuildPlatform::GetBuildPlatformForCPPTargetPlatform: No BuildPlatform found for {0}", InPlatform.ToString());
		}

        /**
         *	Allow all registered build platforms to modify the newly created module 
         *	passed in for the given platform.
         *	This is not required - but allows for hiding details of a particular platform.
         *	
         *	@param	InModule		The newly loaded module
         *	@param	Target			The target being build
         *	@param	Only			If this is not unknown, then only run that platform
         */
        public static void PlatformModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target, UnrealTargetPlatform Only = UnrealTargetPlatform.Unknown)
		{
			foreach (KeyValuePair<UnrealTargetPlatform, UEBuildPlatform> PlatformEntry in BuildPlatformDictionary)
			{
                if (Only == UnrealTargetPlatform.Unknown || PlatformEntry.Key == Only || PlatformEntry.Key == Target.Platform)
                {
                    PlatformEntry.Value.ModifyNewlyLoadedModule(InModule, Target);
                }
			}
		}

        /** 
         * Whether platform supports switching SDKs during runtime
         * 
         * @return true if supports
         */
        public virtual bool PlatformSupportsSDKSwitching()
        {
            return false;
        }

        /** 
         * Returns platform-specific name used in SDK repository
         * 
         * @return path to SDK Repository
         */
        public virtual string GetSDKTargetPlatformName()
        {
            return "";
        }

        /** 
         * Returns SDK string as required by the platform 
         * 
         * @return Valid SDK string
         */
        public virtual string GetRequiredSDKString()
        {
            return "";
        }

        /**
        * Gets the version number of the SDK setup script itself.  The version in the base should ALWAYS be 1.0.  If you need to force a rebuild for a given platform, override this for the given platform.
        * @return Setup script version
        */
        public virtual String GetRequiredScriptVersionString()
        {
            return "1.0";
        }

        /** 
         * Returns path to platform SDKs
         * 
         * @return Valid SDK string
         */
        public virtual string GetPathToPlatformSDKs(out bool SDKRootSet)
        {
            string SDKPath = "";
            SDKRootSet = false;

            string SDKRoot = Environment.GetEnvironmentVariable(SDKRootEnvVar);
            if (SDKRoot != null)
            {
                SDKRootSet = true;
                if (SDKRoot != "")
                {
                    SDKPath = Path.Combine(SDKRoot, "Host" + ExternalExecution.GetRuntimePlatform(), GetSDKTargetPlatformName());
                }
            }
            return SDKPath;
        }

        /**
         * Gets currently installed version
         * 
         * @param PlatformSDKRoot absolute path to platform SDK root
         * @param OutInstalledSDKVersionString version string as currently installed
         * 
         * @return true if was able to read it
         */
        public virtual bool GetCurrentlyInstalledSDKString(string PlatformSDKRoot, out string OutInstalledSDKVersionString)
        {
            if (Directory.Exists(PlatformSDKRoot))
            {
                string VersionFilename = Path.Combine(PlatformSDKRoot, CurrentlyInstalledSDKStringManifest);
                if (File.Exists(VersionFilename))
                {
                    using (StreamReader Reader = new StreamReader(VersionFilename))
                    {
                        string Version = Reader.ReadLine();
                        if (Version != null)
                        {
                            OutInstalledSDKVersionString = Version;
                            return true;
                        }
                    }
                }
            }

            OutInstalledSDKVersionString = "";
            return false;
        }

        /**
         * Gets the version of the last successfully run setup script.
         * 
         * @param PlatformSDKRoot absolute path to platform SDK root
         * @param OutLastRunScriptVersion version string
         * 
         * @return true if was able to read it
         */
        public virtual bool GetLastRunScriptVersionString(string PlatformSDKRoot, out string OutLastRunScriptVersion)
        {
            if (Directory.Exists(PlatformSDKRoot))
            {
                string VersionFilename = Path.Combine(PlatformSDKRoot, LastRunScriptVersionManifest);
                if (File.Exists(VersionFilename))
                {
                    using (StreamReader Reader = new StreamReader(VersionFilename))
                    {
                        string Version = Reader.ReadLine();
                        if (Version != null)
                        {
                            OutLastRunScriptVersion = Version;
                            return true;
                        }
                    }
                }
            }

            OutLastRunScriptVersion = "";
            return false;
        }
       

        /**
         * Sets currently installed version
         * 
         * @param PlatformSDKRoot absolute path to platform SDK root
         * @param InstalledSDKVersionString SDK version string to set
         * 
         * @return true if was able to set it
         */
        public virtual bool SetCurrentlyInstalledSDKString(string PlatformSDKRoot, string InstalledSDKVersionString)
        {
            if (Directory.Exists(PlatformSDKRoot))
            {
                string VersionFilename = Path.Combine(PlatformSDKRoot, CurrentlyInstalledSDKStringManifest);
                if (File.Exists(VersionFilename))
                {
                    File.Delete(VersionFilename);
                }

                using (StreamWriter Writer = File.CreateText(VersionFilename))
                {
                    Writer.WriteLine(InstalledSDKVersionString);
                    return true;
                }
            }

            return false;
        }

        public virtual bool SetLastRunScriptVersion(string PlatformSDKRoot, string LastRunScriptVersion)
        {
            if (Directory.Exists(PlatformSDKRoot))
            {
                string VersionFilename = Path.Combine(PlatformSDKRoot, LastRunScriptVersionManifest);
                if (File.Exists(VersionFilename))
                {
                    File.Delete(VersionFilename);
                }

                using (StreamWriter Writer = File.CreateText(VersionFilename))
                {
                    Writer.WriteLine(LastRunScriptVersion);
                    return true;
                }
            }

            return false;
        }

        /**
         * Returns Hook names as needed by the platform
         * (e.g. can be overriden with custom executables or scripts)
         *
         * @param Hook Hook type
         */
        public virtual string GetHookExecutableName(SDKHookType Hook)
        {
            if (Hook == SDKHookType.Uninstall)
            {
                return "unsetup.bat";
            }

            return "setup.bat";
        }

        /**
         * Runs install/uninstall hooks for SDK
         * 
         * @param PlatformSDKRoot absolute path to platform SDK root
         * @param SDKVersionString version string to run for (can be empty!)
         * @param Hook which one of hooks to run
         * @param bHookCanBeNonExistent whether a non-existing hook means failure
         * 
         * @return true if succeeded
         */
        public virtual bool RunSDKHooks(string PlatformSDKRoot, string SDKVersionString, SDKHookType Hook, bool bHookCanBeNonExistent = true)
        {
            if (SDKVersionString != "")
            {
                string SDKDirectory = Path.Combine(PlatformSDKRoot, SDKVersionString);
                string HookExe = Path.Combine(SDKDirectory, GetHookExecutableName(Hook));

                if (File.Exists(HookExe))
                {
                    Console.WriteLine("Running {0} hook {1}", Hook, HookExe);

                    // run it
                    Process HookProcess = new Process();
                    HookProcess.StartInfo.WorkingDirectory = SDKDirectory;
                    HookProcess.StartInfo.FileName = HookExe;
                    HookProcess.StartInfo.Arguments = "";

                    //installers may require administrator access to succeed. so run as an admmin.
					HookProcess.StartInfo.Verb = "runas";
                    HookProcess.Start();
                    HookProcess.WaitForExit();

                    if (HookProcess.ExitCode != 0)
                    {
                        Console.WriteLine("Hook exited uncleanly (returned {0}), considering it failed.", HookProcess.ExitCode);
                        return false;
                    }

                    return true;
                }
            }

            return bHookCanBeNonExistent;
        }

        /**
         * Returns the delimiter used to separate paths in the PATH environment variable for the platform we are executing on.
         */
        public String GetPathVarDelimiter()
        {
            switch (ExternalExecution.GetRuntimePlatform())
            {
                case UnrealTargetPlatform.Linux:
                case UnrealTargetPlatform.Mac:
                    return ":";
                case UnrealTargetPlatform.Win32:
                case UnrealTargetPlatform.Win64:
                    return ";";
                default:
                    Log.TraceWarning("PATH var delimiter unknown for platform " + ExternalExecution.GetRuntimePlatform().ToString() + " using ';'");
                    return ";";
            }
        }

        /**
         * Loads environment variables from SDK
         * 
         * @param PlatformSDKRoot absolute path to platform SDK
         * @param SDKVersionString SDK version string (cannot be empty!)
         * 
         * @return true if succeeded
         */
        public virtual bool SetupEnvironmentFromSDK(string PlatformSDKRoot, string SDKVersionString)
        {
            string EnvVarFile = Path.Combine(PlatformSDKRoot, SDKVersionString, SDKEnvironmentVarsFile);
            if (File.Exists(EnvVarFile))
            {
                using (StreamReader Reader = new StreamReader(EnvVarFile))
                {
                    List<string> PathAdds = new List<string>();
                    List<string> PathRemoves = new List<string>();

                    List<string> EnvVarNames = new List<string>();
                    List<string> EnvVarValues = new List<string>();
                    for (; ;)
                    {
                        string VariableString = Reader.ReadLine();
                        if (VariableString == null)
                        {
                            break;
                        }
						
                        string[] Parts = VariableString.Split('=');
                        if (Parts.Length != 2)
                        {
                            Console.WriteLine("Incorrect environment variable declaration:");
                            Console.WriteLine(VariableString);
                            return false;
                        }

                        if (String.Compare(Parts[0], "strippath", true) == 0)
                        {
                            PathRemoves.Add(Parts[1]);
                        }
                        else if (String.Compare(Parts[0], "addpath", true) == 0)
                        {
                            PathAdds.Add(Parts[1]);
                        }
                        else
                        {
                            // convenience for setup.bat writers.  Trim any accidental whitespace from var names/values.
                            EnvVarNames.Add(Parts[0].Trim());
                            EnvVarValues.Add(Parts[1].Trim());
                        }
                    }

                    // don't actually set anything until we successfully validate and read all values in.
                    // we don't want to set a few vars, return a failure, and then have a platform try to
                    // build against a manually installed SDK with half-set env vars.
                    for (int i = 0; i < EnvVarNames.Count; ++i)
                    {
                        string EnvVarName = EnvVarNames[i];
                        string EnvVarValue = EnvVarValues[i];                        
                        if (BuildConfiguration.bPrintDebugInfo)
                        {
                            Console.WriteLine("Setting variable '{0}' to '{1}'", EnvVarName, EnvVarValue);
                        }
                        Environment.SetEnvironmentVariable(EnvVarName, EnvVarValue);
                    }
                       
                    
                    // actually perform the PATH stripping / adding.
                    String OrigPathVar = Environment.GetEnvironmentVariable("PATH");
                    String PathDelimiter = GetPathVarDelimiter();
                    String[] PathVars = OrigPathVar.Split(PathDelimiter.ToCharArray());

                    List<String> ModifiedPathVars = new List<string>();
                    ModifiedPathVars.AddRange(PathVars);

                    // perform removes first, in case they overlap with any adds.
                    foreach (String PathRemove in PathRemoves)
                    {
                        foreach (String PathVar in PathVars)
                        {
                            if (PathVar.IndexOf(PathRemove, StringComparison.OrdinalIgnoreCase) >= 0)
                            {
                                if (BuildConfiguration.bPrintDebugInfo)
                                {
                                    Console.WriteLine("Removing Path: '{0}'", PathVar);
                                }
                                ModifiedPathVars.Remove(PathVar);
                            }
                        }
                    }

                    // remove all the of ADDs so that if this function is executed multiple times, the paths will be guarateed to be in the same order after each run.
                    // If we did not do this, a 'remove' that matched some, but not all, of our 'adds' would cause the order to change.
                    foreach (String PathAdd in PathAdds)
                    {
                        foreach (String PathVar in PathVars)
                        {
                            if (String.Compare(PathAdd, PathVar, true) == 0)
                            {
                                if (BuildConfiguration.bPrintDebugInfo)
                                {
                                    Console.WriteLine("Removing Path: '{0}'", PathVar);
                                }
                                ModifiedPathVars.Remove(PathVar);
                            }
                        }
                    }

                    // perform adds, but don't add duplicates
                    foreach (String PathAdd in PathAdds)
                    {
                        if (!ModifiedPathVars.Contains(PathAdd))
                        {
                            Console.WriteLine("Adding Path: '{0}'", PathAdd);
                            ModifiedPathVars.Add(PathAdd);
                        }
                    }

                    String ModifiedPath = String.Join(PathDelimiter, ModifiedPathVars);
                    Environment.SetEnvironmentVariable("PATH", ModifiedPath);

                    return true;
                }
            }

            return false;
        }

        /**
         *	Whether the required external SDKs are installed for this platform
         */
		public virtual SDKStatus HasRequiredSDKsInstalled()
		{			
            if (PlatformSupportsSDKSwitching())
            {				
                // if we don't have a path to platform SDK's, take no action
                bool bSDKPathSet;
                string PlatformSDKRoot = GetPathToPlatformSDKs(out bSDKPathSet);				
                if (bSDKPathSet)
                {					
                    if (PlatformSDKRoot != "")
                    {
                        // check script version so script fixes can be propagated without touching every build machine's CurrentlyInstalled file manually.
                        bool bScriptVersionMatches = false;
                        string CurrentScriptVersionString;
                        if (GetLastRunScriptVersionString(PlatformSDKRoot, out CurrentScriptVersionString) && CurrentScriptVersionString == GetRequiredScriptVersionString())
                        {
                            bScriptVersionMatches = true;
                        }

                        string CurrentSDKString;
                        if (!GetCurrentlyInstalledSDKString(PlatformSDKRoot, out CurrentSDKString) || CurrentSDKString != GetRequiredSDKString() || !bScriptVersionMatches)
                        {
                        
                            // switch over (note that version string can be empty)
                            if (!RunSDKHooks(PlatformSDKRoot, CurrentSDKString, SDKHookType.Uninstall))
                            {
                                Console.WriteLine("Failed to uninstall currently installed SDK {0}", CurrentSDKString);
                                return SDKStatus.Invalid;
                            }
                            // delete Manifest file to avoid multiple uninstalls
                            SetCurrentlyInstalledSDKString(PlatformSDKRoot, "");
                            SetLastRunScriptVersion(PlatformSDKRoot, "");

                            if (!RunSDKHooks(PlatformSDKRoot, GetRequiredSDKString(), SDKHookType.Install, false))
                            {								
                                Console.WriteLine("Failed to install required SDK {0}.  Attemping to uninstall", GetRequiredSDKString());
                                RunSDKHooks(PlatformSDKRoot, GetRequiredSDKString(), SDKHookType.Uninstall, false);

                                return SDKStatus.Invalid;
                            }
                            SetCurrentlyInstalledSDKString(PlatformSDKRoot, GetRequiredSDKString());
                            SetLastRunScriptVersion(PlatformSDKRoot, GetRequiredScriptVersionString());
                        }

                        // load environment variables from current SDK
                        if (!SetupEnvironmentFromSDK(PlatformSDKRoot, GetRequiredSDKString()))
                        {
                            Console.WriteLine("Failed to load environment from required SDK {0}", GetRequiredSDKString());
                            return SDKStatus.Invalid;
                        }                        
                        return SDKStatus.Valid;
                    }
                    else
                    {
                        Console.WriteLine("Failed to install required SDK {0}. {1} is blank.", GetRequiredSDKString(), SDKRootEnvVar);
                        return SDKStatus.Invalid;
                    }
                }
            }
            return SDKStatus.Unknown;
        }

        /**
         *	If this platform can be compiled with XGE
         */
        public virtual bool CanUseXGE()
        {
            return true;
        }

		/**
		 *	Register the platform with the UEBuildPlatform class
		 */
		public abstract void RegisterBuildPlatform();

		/**
		 *	Retrieve the CPPTargetPlatform for the given UnrealTargetPlatform
		 *
		 *	@param	InUnrealTargetPlatform		The UnrealTargetPlatform being build
		 *	
		 *	@return	CPPTargetPlatform			The CPPTargetPlatform to compile for
		 */
		public abstract CPPTargetPlatform GetCPPTargetPlatform(UnrealTargetPlatform InUnrealTargetPlatform);

		/// <summary>
		/// Return whether the given platform requires a monolithic build
		/// </summary>
		/// <param name="InPlatform">The platform of interest</param>
		/// <param name="InConfiguration">The configuration of interest</param>
		/// <returns></returns>
		public static bool PlatformRequiresMonolithicBuilds(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			// Some platforms require monolithic builds...
			UEBuildPlatform BuildPlatform = GetBuildPlatform(InPlatform, true);
			if (BuildPlatform != null)
			{
				return BuildPlatform.ShouldCompileMonolithicBinary(InPlatform);
			}

			// We assume it does not
			return false;
		}

		/**
		 *	Get the extension to use for the given binary type
		 *	
		 *	@param	InBinaryType		The binary type being built
		 *	
		 *	@return	string				The binary extension (i.e. 'exe' or 'dll')
		 */
		public virtual string GetBinaryExtension(UEBuildBinaryType InBinaryType)
		{
			throw new BuildException("GetBinaryExtensiton for {0} not handled in {1}", InBinaryType.ToString(), this.ToString());
		}

		/**
		 *	Get the extension to use for debug info for the given binary type
		 *	
		 *	@param	InBinaryType		The binary type being built
		 *	
		 *	@return	string				The debug info extension (i.e. 'pdb')
		 */
		public virtual string GetDebugInfoExtension( UEBuildBinaryType InBinaryType )
		{
			throw new BuildException( "GetDebugInfoExtension for {0} not handled in {1}", InBinaryType.ToString(), this.ToString() );
		}

		/**
		 *	Whether incremental linking should be used
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	InConfiguration		The CPPTargetConfiguration being built
		 *	
		 *	@return	bool	true if incremental linking should be used, false if not
		 */
		public virtual bool ShouldUseIncrementalLinking(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration)
		{
			return false;
		}

		/**
		 *	Whether PDB files should be used
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	InConfiguration		The CPPTargetConfiguration being built
		 *	@param	bInCreateDebugInfo	true if debug info is getting create, false if not
		 *	
		 *	@return	bool	true if PDB files should be used, false if not
		 */
		public virtual bool ShouldUsePDBFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration, bool bCreateDebugInfo)
		{
			return false;
		}

		/**
		 *	Whether PCH files should be used
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	InConfiguration		The CPPTargetConfiguration being built
		 *	
		 *	@return	bool				true if PCH files should be used, false if not
		 */
		public virtual bool ShouldUsePCHFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration)
		{
			return BuildConfiguration.bUsePCHFiles;
		}

		/**
		 *	Whether the editor should be built for this platform or not
		 *	
		 *	@param	InPlatform		The UnrealTargetPlatform being built
		 *	@param	InConfiguration	The UnrealTargetConfiguration being built
		 *	@return	bool			true if the editor should be built, false if not
		 */
		public virtual bool ShouldNotBuildEditor(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return false;
		}

		/**
		 *	Whether this build should support ONLY cooked data or not
		 *	
		 *	@param	InPlatform		The UnrealTargetPlatform being built
		 *	@param	InConfiguration	The UnrealTargetConfiguration being built
		 *	@return	bool			true if the editor should be built, false if not
		 */
		public virtual bool BuildRequiresCookedData(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return false;
		}

		/**
		 *	Whether the platform requires the extra UnityCPPWriter
		 *	This is used to add an extra file for UBT to get the #include dependencies from
		 *	
		 *	@return	bool	true if it is required, false if not
		 */
		public virtual bool RequiresExtraUnityCPPWriter()
		{
			return false;
		}

		/**
		 * Whether this platform should build a monolithic binary
		 */
		public virtual bool ShouldCompileMonolithicBinary(UnrealTargetPlatform InPlatform)
		{
			return false;
		}

		/**
		 *	Get a list of extra modules the platform requires.
		 *	This is to allow undisclosed platforms to add modules they need without exposing information about the platform.
		 *	
		 *	@param	Target						The target being build
		 *	@param	BuildTarget					The UEBuildTarget getting build
		 *	@param	PlatformExtraModules		OUTPUT the list of extra modules the platform needs to add to the target
		 */
		public virtual void GetExtraModules(TargetInfo Target, UEBuildTarget BuildTarget, ref List<string> PlatformExtraModules)
		{
		}

		/**
		 *	Modify the newly created module passed in for this platform.
		 *	This is not required - but allows for hiding details of a
		 *	particular platform.
		 *	
		 *	@param	InModule		The newly loaded module
		 *	@param	Target			The target being build
		 */
		public virtual void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
		{
		}

		/**
		 *	Setup the target environment for building
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public abstract void SetUpEnvironment(UEBuildTarget InBuildTarget);

		/**
		 * Allow the platform to set an optional architecture
		 */
		public virtual string GetActiveArchitecture()
		{
			// by default, use an empty architecture (which is really just a modifer to the platform for some paths/names)
			return "";
		}

		/**
		 * Allow the platform to override the NMake output name
		 */
		public virtual string ModifyNMakeOutput(string ExeName)
		{
			// by default, use original
			return ExeName;
		}

		/**
		 *	Setup the configuration environment for building
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public virtual void SetUpConfigurationEnvironment(UEBuildTarget InBuildTarget)
		{
			// Determine the C++ compile/link configuration based on the Unreal configuration.
			CPPTargetConfiguration CompileConfiguration;
			UnrealTargetConfiguration CheckConfig = InBuildTarget.Configuration;
			//@todo SAS: Add a true Debug mode!
			if (UnrealBuildTool.RunningRocket())
			{
				if (Utils.IsFileUnderDirectory( InBuildTarget.OutputPath, UnrealBuildTool.GetUProjectPath() ))
				{
					if (CheckConfig == UnrealTargetConfiguration.Debug)
					{
						CheckConfig = UnrealTargetConfiguration.DebugGame;
					}
				}
				else
				{
					// Only Development and Shipping are supported for engine modules
					if( CheckConfig != UnrealTargetConfiguration.Development && CheckConfig != UnrealTargetConfiguration.Shipping )
					{
						CheckConfig = UnrealTargetConfiguration.Development;
					}
				}
			}
			switch (CheckConfig)
			{
				default:
				case UnrealTargetConfiguration.Debug:
					CompileConfiguration = CPPTargetConfiguration.Debug;
					if( BuildConfiguration.bDebugBuildsActuallyUseDebugCRT )
					{
						InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_DEBUG=1"); // the engine doesn't use this, but lots of 3rd party stuff does
					}
					else
					{
						InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("NDEBUG=1"); // the engine doesn't use this, but lots of 3rd party stuff does
					}
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UE_BUILD_DEBUG=1");
					break;
				case UnrealTargetConfiguration.DebugGame:
					// Individual game modules can be switched to be compiled in debug as necessary. By default, everything is compiled in development.
				case UnrealTargetConfiguration.Development:
					CompileConfiguration = CPPTargetConfiguration.Development;
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("NDEBUG=1"); // the engine doesn't use this, but lots of 3rd party stuff does
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UE_BUILD_DEVELOPMENT=1");
					break;
				case UnrealTargetConfiguration.Shipping:
					CompileConfiguration = CPPTargetConfiguration.Shipping;
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("NDEBUG=1"); // the engine doesn't use this, but lots of 3rd party stuff does
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UE_BUILD_SHIPPING=1");
					break;
				case UnrealTargetConfiguration.Test:
					CompileConfiguration = CPPTargetConfiguration.Shipping;
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("NDEBUG=1"); // the engine doesn't use this, but lots of 3rd party stuff does
					InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UE_BUILD_TEST=1");
					break;
			}

			// Set up the global C++ compilation and link environment.
			InBuildTarget.GlobalCompileEnvironment.Config.TargetConfiguration = CompileConfiguration;
			InBuildTarget.GlobalLinkEnvironment.Config.TargetConfiguration = CompileConfiguration;

			// Create debug info based on the heuristics specified by the user.
			InBuildTarget.GlobalCompileEnvironment.Config.bCreateDebugInfo =
				!BuildConfiguration.bDisableDebugInfo && ShouldCreateDebugInfo(InBuildTarget.Platform, CheckConfig);
			InBuildTarget.GlobalLinkEnvironment.Config.bCreateDebugInfo = InBuildTarget.GlobalCompileEnvironment.Config.bCreateDebugInfo;
		}

		/**
		 *	Whether this platform should create debug information or not
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	InConfiguration		The UnrealTargetConfiguration being built
		 *	
		 *	@return	bool				true if debug info should be generated, false if not
		 */
		public abstract bool ShouldCreateDebugInfo(UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration);

		/**
		 *	Gives the platform a chance to 'override' the configuration settings 
		 *	that are overridden on calls to RunUBT.
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	InConfiguration		The UnrealTargetConfiguration being built
		 *	
		 *	@return	bool				true if debug info should be generated, false if not
		 */
		public virtual void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
		}

		/**
		 *	Validate the UEBuildConfiguration for this platform
		 *	This is called BEFORE calling UEBuildConfiguration to allow setting 
		 *	various fields used in that function such as CompileLeanAndMean...
		 */
		public virtual void ValidateUEBuildConfiguration()
		{
		}

		/**
		 *	Validate configuration for this platform
		 *	NOTE: This function can/will modify BuildConfiguration!
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	InConfiguration		The CPPTargetConfiguration being built
		 *	@param	bInCreateDebugInfo	true if debug info is getting create, false if not
		 */
		public virtual void ValidateBuildConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
		{
		}

		/**
		 *	Return whether this platform has uniquely named binaries across multiple games
		 */
		public virtual bool HasUniqueBinaries()
		{
			return true;
		}

		/**
		 *	Return whether we wish to have this platform's binaries in our builds
		 */
		public virtual bool IsBuildRequired()
		{
			return true;
		}

		/**
		 *	Return whether we wish to have this platform's binaries in our CIS tests
		 */
		public virtual bool IsCISRequired()
		{
			return true;
		}

		/// <summary>
		/// Whether the build platform requires deployment prep
		/// </summary>
		/// <returns></returns>
		public virtual bool RequiresDeployPrepAfterCompile()
		{
			return false;
		}

		/**
		 *	Return all valid configurations for this platform
		 *	
		 *  Typically, this is always Debug, Development, and Shipping - but Test is a likely future addition for some platforms
		 */
		public virtual List<UnrealTargetConfiguration> GetConfigurations( UnrealTargetPlatform InUnrealTargetPlatform, bool bIncludeDebug )
		{
			List<UnrealTargetConfiguration> Configurations = new List<UnrealTargetConfiguration>()
			{
				UnrealTargetConfiguration.Development, 
			};

			if( bIncludeDebug )
			{
				Configurations.Insert( 0, UnrealTargetConfiguration.Debug );
			}

			return Configurations;
		}

		/**
		 *	Setup the binaries for this specific platform.
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public virtual void SetupBinaries(UEBuildTarget InBuildTarget)
		{
		}
	}
}
