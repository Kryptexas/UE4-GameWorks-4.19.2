// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using System.Xml;

namespace UnrealBuildTool
{
	class IOSPlatform : UEBuildPlatform
	{
		// The current architecture - affects everything about how UBT operates on IOS
		private static string ActiveArchitecture = Utils.GetStringEnvironmentVariable("ue.IOSArchitecture", "");//"-simulator");
		public override string GetActiveArchitecture()
		{
			// by default, use an empty architecture (which is really just a modifer to the platform for some paths/names)
			return ActiveArchitecture;
		}

		/**
		 *	Register the platform with the UEBuildPlatform class
		 */
		public override void RegisterBuildPlatform()
		{
			//@todo.Rocket: Add platform support
			if ((UnrealBuildTool.RunningRocket() || UnrealBuildTool.BuildingRocket()) && ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				return;
			}

			// Register this build platform for IOS
			Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.IOS.ToString());
			UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.IOS, this);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.IOS, UnrealPlatformGroup.Unix);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.IOS, UnrealPlatformGroup.Apple);

            if (GetActiveArchitecture() == "-simulator")
            {
                UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.IOS, UnrealPlatformGroup.Simulator);
            }
            else
            {
                UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.IOS, UnrealPlatformGroup.Device);
            }
		}

		/**
		 *	Retrieve the CPPTargetPlatform for the given UnrealTargetPlatform
		 *
		 *	@param	InUnrealTargetPlatform		The UnrealTargetPlatform being build
		 *	
		 *	@return	CPPTargetPlatform			The CPPTargetPlatform to compile for
		 */
		public override CPPTargetPlatform GetCPPTargetPlatform(UnrealTargetPlatform InUnrealTargetPlatform)
		{
			switch (InUnrealTargetPlatform)
			{
				case UnrealTargetPlatform.IOS:
					return CPPTargetPlatform.IOS;
			}
			throw new BuildException("IOSPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
		}

		/**
		 *	Get the extension to use for the given binary type
		 *	
		 *	@param	InBinaryType		The binary type being built
		 *	
		 *	@return	string				The binary extenstion (ie 'exe' or 'dll')
		 */
		public override string GetBinaryExtension(UEBuildBinaryType InBinaryType)
		{
			switch (InBinaryType)
			{
				case UEBuildBinaryType.DynamicLinkLibrary:
					return ".dylib";
				case UEBuildBinaryType.Executable:
					return "";
				case UEBuildBinaryType.StaticLibrary:
					return ".a";
			}
			return base.GetBinaryExtension(InBinaryType);
		}

		public override string GetDebugInfoExtension(UEBuildBinaryType InBinaryType)
		{
			return BuildConfiguration.bGeneratedSYMFile ? ".dsym" : "";
		}

        public override bool CanUseXGE()
        {
            return false;
        }

		/**
		 *	Setup the target environment for building
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public override void SetUpEnvironment(UEBuildTarget InBuildTarget)
		{
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_IOS=1");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_APPLE=1");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_TTS=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_SPEECH_RECOGNITION=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_DATABASE_SUPPORT=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_EDITOR=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("USE_NULL_RHI=0");

			if (GetActiveArchitecture() == "-simulator")
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_SIMULATOR=1");
			}

            InBuildTarget.GlobalLinkEnvironment.Config.AdditionalFrameworks.Add("GameKit");
            InBuildTarget.GlobalLinkEnvironment.Config.AdditionalFrameworks.Add("StoreKit");
		}

		/**
		*	Whether the editor should be built for this platform or not
		*	
		*	@param	InPlatform		The UnrealTargetPlatform being built
		*	@param	InConfiguration	The UnrealTargetConfiguration being built
		*	@return	bool			true if the editor should be built, false if not
		*/
		public override bool ShouldNotBuildEditor(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return true;
		}

		public override bool BuildRequiresCookedData(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return true; // for iOS can only run cooked. this is mostly for testing console code paths.
		}

		/**
		 *	Whether this platform should create debug information or not
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	InConfiguration		The UnrealTargetConfiguration being built
		 *	
		 *	@return	bool				true if debug info should be generated, false if not
		 */
		public override bool ShouldCreateDebugInfo(UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration)
		{
			return true;
		}

		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			UEBuildConfiguration.bBuildEditor = false;
			UEBuildConfiguration.bBuildDeveloperTools = false;
			UEBuildConfiguration.bCompileAPEX = false;
			UEBuildConfiguration.bCompileSimplygon = false;
			UEBuildConfiguration.bCompileNetworkProfiler = false;
			UEBuildConfiguration.bBuildDeveloperTools = false;

			// we currently don't have any simulator libs for PhysX
			if (GetActiveArchitecture() == "-simulator")
			{
				UEBuildConfiguration.bCompilePhysX = false;
			}
		}

		public override bool ShouldCompileMonolithicBinary(UnrealTargetPlatform InPlatform)
		{
			// This platform currently always compiles monolithic
			return true;
		}

		public override void ValidateBuildConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
		{
			BuildConfiguration.bUsePCHFiles = false;
			BuildConfiguration.bUseSharedPCHs = false;
			BuildConfiguration.bCheckExternalHeadersForModification = false;
			BuildConfiguration.bCheckSystemHeadersForModification = false;
			BuildConfiguration.ProcessorCountMultiplier = IOSToolChain.GetAdjustedProcessorCountMultiplier();
			BuildConfiguration.bDeployAfterCompile = true;
		}

		/**
		 *	Whether the platform requires the extra UnityCPPWriter
		 *	This is used to add an extra file for UBT to get the #include dependencies from
		 *	
		 *	@return	bool	true if it is required, false if not
		 */
		public override bool RequiresExtraUnityCPPWriter()
		{
			return true;
		}

		/**
		 *     Modify the newly created module passed in for this platform.
		 *     This is not required - but allows for hiding details of a
		 *     particular platform.
		 *     
		 *     @param InModule             The newly loaded module
		 *     @param GameName             The game being build
		 *     @param Target               The target being build
		 */
		public override void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
		{
			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64) || (Target.Platform == UnrealTargetPlatform.Mac))
			{
				bool bBuildShaderFormats = UEBuildConfiguration.bForceBuildShaderFormats;
				if (!UEBuildConfiguration.bBuildRequiresCookedData)
				{
					if (InModule.ToString() == "Engine")
					{
						if (UEBuildConfiguration.bBuildDeveloperTools)
						{
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("IOSTargetPlatform");
						}
					}
					else if (InModule.ToString() == "UnrealEd")
					{
						InModule.AddPlatformSpecificDynamicallyLoadedModule("IOSPlatformEditor");
					}
					else if (InModule.ToString() == "TargetPlatform")
					{
						bBuildShaderFormats = true;
                        InModule.AddDynamicallyLoadedModule("TextureFormatPVR");
						if (UEBuildConfiguration.bBuildDeveloperTools)
						{
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("AudioFormatADPCM");
						}
					}
				}

				// allow standalone tools to use targetplatform modules, without needing Engine
				if (UEBuildConfiguration.bForceBuildTargetPlatforms)
				{
                    InModule.AddPlatformSpecificDynamicallyLoadedModule("IOSTargetPlatform");
				}

				if (bBuildShaderFormats)
				{
                    //InModule.AddPlatformSpecificDynamicallyLoadedModule("ShaderFormatIOS");
				}
			}
		}

		public override void SetupBinaries(UEBuildTarget InBuildTarget)
		{
			if (ExternalExecution.GetRuntimePlatform () != UnrealTargetPlatform.Mac)
			{
				// dangerously fast mode doesn't generate stub files
				if (!IOSToolChain.bUseDangerouslyFastMode)
				{
					List<UEBuildBinary> NewBinaries = new List<UEBuildBinary> ();
					// add the .stub to the binaries
					foreach (var Binary in InBuildTarget.AppBinaries)
					{
						// make a binary that just points to the .stub of this executable
						UEBuildBinaryConfiguration NewConfig = new UEBuildBinaryConfiguration(
							                                      InType: Binary.Config.Type,
							                                      InOutputFilePath: Binary.Config.OutputFilePath + ".stub",
							                                      InIntermediateDirectory: Binary.Config.IntermediateDirectory,
							                                      bInCreateImportLibrarySeparately: Binary.Config.bCreateImportLibrarySeparately,
							                                      bInAllowExports: Binary.Config.bAllowExports,
							                                      InModuleNames: Binary.Config.ModuleNames);

						NewBinaries.Add (new UEStubDummyBinary (InBuildTarget, NewConfig));
					}

					InBuildTarget.AppBinaries.AddRange (NewBinaries);
				}
			}
		}
	}

	/// <summary>
	/// A .stub that has the executable and metadata included
	/// Note that this doesn't actually build anything. It could potentially be used to perform the IPhonePackage stuff that happens in IOSToolChain.PostBuildSync()
	/// </summary>
	class UEStubDummyBinary : UEBuildBinary
	{
		/// <summary>
		/// Create an instance initialized to the given configuration
		/// </summary>
		/// <param name="InConfig">The build binary configuration to initialize the instance to</param>
		public UEStubDummyBinary(UEBuildTarget InTarget, UEBuildBinaryConfiguration InConfig)
			: base(InTarget, InConfig)
		{
		}

		/// <summary>
		/// Builds the binary.
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public override IEnumerable<FileItem> Build(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment)
		{
			// generate the .stub!
			return new FileItem[] { FileItem.GetItemByPath(Config.OutputFilePath) };
		}

		/// <summary>
		/// Writes an XML summary of the build environment for this binary
		/// </summary>
		/// <param name="CompileEnvironment">The environment to compile the binary in</param>
		/// <param name="LinkEnvironment">The environment to link the binary in</param>
		/// <returns></returns>
		public override void WriteBuildEnvironment(CPPEnvironment CompileEnvironment, LinkEnvironment LinkEnvironment, XmlWriter Writer)
		{
		}
	}

}

