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

		/**
		 * Attempt to convert a string to an UnrealTargetPlatform enum eentry
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
			List<UnrealTargetPlatform> PlatformList;
			if (PlatformGroupDictionary.TryGetValue(InGroup, out PlatformList) == false)
			{
				PlatformList = new List<UnrealTargetPlatform>();
				PlatformGroupDictionary[InGroup] = PlatformList;
			}

			// add the platform to the list of platforms the group has
			PlatformList.Add(InPlatform);
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
		 *	Whether the required external SDKs are installed for this platform
		 */
		public virtual bool HasRequiredSDKsInstalled()
		{
			return true;
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
