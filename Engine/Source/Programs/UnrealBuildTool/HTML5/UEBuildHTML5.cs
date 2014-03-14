// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	class HTML5Platform : UEBuildPlatform
	{
		// The current architecture - affects everything about how UBT operates on HTML5
		private static string ActiveArchitecture = Utils.GetStringEnvironmentVariable("ue.HTML5Architecture", "");//"-win32");
		public override string GetActiveArchitecture()
		{
			// by default, use an empty architecture (which is really just a modifier to the platform for some paths/names)
			return ActiveArchitecture;
		}

		// F5 should always try to run the Win32 version
		public override string ModifyNMakeOutput(string ExeName)
		{
			// nmake Run should always run the win32 version
			return Path.ChangeExtension(ExeName, ".exe");
		}

		/**
		 *	Whether the required external SDKs are installed for this platform
		 */
		public override bool HasRequiredSDKsInstalled()
		{
			string BaseSDKPath = Environment.GetEnvironmentVariable("EMSCRIPTEN");
			return string.IsNullOrEmpty(BaseSDKPath) == false;
		}

        public override bool CanUseXGE()
        {
            return (GetActiveArchitecture() == "-win32" );
        }

		/**
		 *	Register the platform with the UEBuildPlatform class
		 */
		public override void RegisterBuildPlatform()
		{
			//@todo.Rocket: Add platform support
			if (UnrealBuildTool.RunningRocket() || UnrealBuildTool.BuildingRocket())
			{
				return;
			}

			// Make sure the SDK is installed
			if ((ProjectFileGenerator.bGenerateProjectFiles == true) || (HasRequiredSDKsInstalled() == true))
			{
				bool bRegisterBuildPlatform = true;

				// make sure we have the HTML5 files; if not, then this user doesn't really have HTML5 access/files, no need to compile HTML5!
				string EngineSourcePath = Path.Combine(ProjectFileGenerator.EngineRelativePath, "Source");
				string HTML5TargetPlatformFile = Path.Combine(EngineSourcePath, "Developer", "HTML5", "HTML5TargetPlatform", "HTML5TargetPlatform.Build.cs");
 				if ((File.Exists(HTML5TargetPlatformFile) == false))
 				{
 					bRegisterBuildPlatform = false;
 					Log.TraceWarning("Missing required components (.... HTML5TargetPlatformFile, others here...). Check source control filtering, or try resyncing.");
 				}

				if (bRegisterBuildPlatform == true)
				{
					// Register this build platform for HTML5
					Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.HTML5.ToString());
					UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.HTML5, this);
                    if (GetActiveArchitecture() == "-win32")
                    {
                        UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.HTML5, UnrealPlatformGroup.Simulator);
                    }
                    else
                    {
                        UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.HTML5, UnrealPlatformGroup.Device);
                    }
				}
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
				case UnrealTargetPlatform.HTML5:
					return CPPTargetPlatform.HTML5;
			}
			throw new BuildException("HTML5Platform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
		}

		/**
		 *	Get the extension to use for the given binary type
		 *	
		 *	@param	InBinaryType		The binrary type being built
		 *	
		 *	@return	string				The binary extenstion (ie 'exe' or 'dll')
		 */
		public override string GetBinaryExtension(UEBuildBinaryType InBinaryType)
		{
			if (GetActiveArchitecture() == "-win32")
			{
				switch (InBinaryType)
				{
					case UEBuildBinaryType.DynamicLinkLibrary:
						return ".dll";
					case UEBuildBinaryType.Executable:
						return ".exe";
					case UEBuildBinaryType.StaticLibrary:
						return ".lib";
				}
				return base.GetBinaryExtension(InBinaryType);
			}
			else
			{
                if (InBinaryType == UEBuildBinaryType.StaticLibrary)
                {
                    return ".bc";
                }
                else 
                {
				    return ".js";
                }
			}
		}

		/**
		 *	Get the extension to use for debug info for the given binary type
		 *	
		 *	@param	InBinaryType		The binary type being built
		 *	
		 *	@return	string				The debug info extension (i.e. 'pdb')
		 */
		public override string GetDebugInfoExtension(UEBuildBinaryType InBinaryType)
		{
			if (GetActiveArchitecture() == "-win32")
			{
				switch (InBinaryType)
				{
					case UEBuildBinaryType.DynamicLinkLibrary:
						return ".pdb";
					case UEBuildBinaryType.Executable:
						return ".pdb";
				}
				return "";
			}
			else
			{
				return "";
			}
		}

		/**
		 *	Gives the platform a chance to 'override' the configuration settings 
		 *	that are overridden on calls to RunUBT.
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	InConfiguration		The UnrealTargetConfiguration being built
		 *	
		 *	@return	bool				true if debug info should be generated, false if not
		 */
		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			ValidateUEBuildConfiguration();
		}

		/**
		 *	Validate the UEBuildConfiguration for this platform
		 *	This is called BEFORE calling UEBuildConfiguration to allow setting 
		 *	various fields used in that function such as CompileLeanAndMean...
		 */
		public override void ValidateUEBuildConfiguration()
		{
			UEBuildConfiguration.bCompileLeanAndMeanUE = true;
			UEBuildConfiguration.bCompileAPEX = false;
			UEBuildConfiguration.bCompilePhysX = true;
			UEBuildConfiguration.bCompileSimplygon = false;
            UEBuildConfiguration.bCompileICU = false;
		}

		/**
		 * Whether this platform should build a monolithic binary
		 */
		public override bool ShouldCompileMonolithicBinary(UnrealTargetPlatform InPlatform)
		{
			// This platform currently always compiles monolithic
			return true;
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
		public override bool ShouldUsePDBFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration, bool bCreateDebugInfo)
		{
			return bCreateDebugInfo;
		}

		/**
		 *	Whether PCH files should be used
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	InConfiguration		The CPPTargetConfiguration being built
		 *	
		 *	@return	bool				true if PCH files should be used, false if not
		 */
		public override bool ShouldUsePCHFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration)
		{
			return false;
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
			return true;
		}

		/**
		 *	Modify the newly created module passed in for this platform.
		 *	This is not required - but allows for hiding details of a
		 *	particular platform.
		 *	
		 *	@param	InModule		The newly loaded module
		 *	@param	Target			The target being build
		 */
		public override void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
		{
			if (Target.Platform == UnrealTargetPlatform.HTML5)
			{
				if (InModule.ToString() == "Core")
				{
					InModule.AddPublicIncludePath("Runtime/Core/Public/HTML5");
                    InModule.AddPublicDependencyModule("zlib");
				}
				else if (InModule.ToString() == "Engine")
				{
					InModule.AddPrivateDependencyModule("zlib");
					InModule.AddPrivateDependencyModule("UElibPNG");
                    InModule.AddPublicDependencyModule("UEOgg");
                    InModule.AddPublicDependencyModule("Vorbis");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Win64)
			{
                bool bBuildShaderFormats = UEBuildConfiguration.bForceBuildShaderFormats;
				if (!UEBuildConfiguration.bBuildRequiresCookedData)
				{
					if (InModule.ToString() == "Engine")
					{
						if (UEBuildConfiguration.bBuildDeveloperTools)
						{
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("HTML5TargetPlatform");
						}
					}
					else if (InModule.ToString() == "TargetPlatform")
					{
                        bBuildShaderFormats = true;
// 							if (UEBuildConfiguration.bBuildDeveloperTools)
// 							{
                        // 								InModule.AddPlatformSpecificDynamicallyLoadedModule("AT9AudioFormat");
                        // 								InModule.AddPlatformSpecificDynamicallyLoadedModule("HTML5TextureFormat");
// 							}
					}
				}

				// allow standalone tools to use target platform modules, without needing Engine
				if (UEBuildConfiguration.bForceBuildTargetPlatforms)
				{
                    InModule.AddPlatformSpecificDynamicallyLoadedModule("HTML5TargetPlatform");
				}

                if (bBuildShaderFormats)
                {
                    // InModule.AddDynamicallyLoadedModule("HTML5ShaderFormat");
                }
			}
		}


		/**
		 *	Setup the target environment for building
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public override void SetUpEnvironment(UEBuildTarget InBuildTarget)
		{
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_HTML5=1");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("HTML5=1");
            if (InBuildTarget.GlobalCompileEnvironment.Config.TargetArchitecture == "-win32")
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_HTML5_WIN32=1");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("delayimp.lib");
			}

			// @todo needed?
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UNICODE");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_UNICODE");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_AUTOMATION_WORKER=0");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("REQUIRES_ALIGNED_INT_ACCESS");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_OGGVORBIS=1");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("USE_SCENE_LOCK=0");
         	BuildConfiguration.bDeployAfterCompile = true;

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
			switch (Configuration)
			{
				case UnrealTargetConfiguration.Development:
				case UnrealTargetConfiguration.Shipping:
				case UnrealTargetConfiguration.Test:
					return !BuildConfiguration.bOmitPCDebugInfoInDevelopment;
				case UnrealTargetConfiguration.Debug:
				default:
					// We don't need debug info for Emscripten, and it causes a bunch of issues with linking
					return true;
			};
		}

		/**
		 *	Setup the binaries for this specific platform.
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public override void SetupBinaries(UEBuildTarget InBuildTarget)
		{
// 			InBuildTarget.ExtraModuleNames.Add("ES2RHI");
		}
	}
}
