// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool.Linux
{
    class LinuxPlatform : UEBuildPlatform
    {
        /**
         *	Whether the required external SDKs are installed for this platform
         */
        public override bool HasRequiredSDKsInstalled()
        {
            string BaseLinuxPath = Environment.GetEnvironmentVariable("LINUX_ROOT");

            // we don't have an ANDROID_ROOT specified
            if (String.IsNullOrEmpty(BaseLinuxPath))
                return false;

            // paths to our toolchains
            BaseLinuxPath = BaseLinuxPath.Replace("\"", "");
            string ClangPath = Path.Combine(BaseLinuxPath, @"bin\Clang++.exe");
            
            if (File.Exists(ClangPath))
                return true;

            return false;
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

            if ((ProjectFileGenerator.bGenerateProjectFiles == true) || (HasRequiredSDKsInstalled() == true))
            {
                bool bRegisterBuildPlatform = true;

                string EngineSourcePath = Path.Combine(ProjectFileGenerator.RootRelativePath, "Engine", "Source");
                string LinuxTargetPlatformFile = Path.Combine(EngineSourcePath, "Developer", "Linux", "LinuxTargetPlatform", "LinuxTargetPlatform.Build.cs");

                if (
                    (File.Exists(LinuxTargetPlatformFile) == false)
                    )
                {
                    bRegisterBuildPlatform = false;
                }

                if (bRegisterBuildPlatform == true)
                {
                    // Register this build platform for Linux
                    if (BuildConfiguration.bPrintDebugInfo)
                    {
                        Console.WriteLine("        Registering for {0}", UnrealTargetPlatform.Linux.ToString());
                    }
                    UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.Linux, this);
                    UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Linux, UnrealPlatformGroup.Unix);
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
                case UnrealTargetPlatform.Linux:
                    return CPPTargetPlatform.Linux;
            }
            throw new BuildException("LinuxPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
        }

        /**
         *	Get the extension to use for the given binary type
         *	
         *	@param	InBinaryType		The binary type being built
         *	
         *	@return	string				The binary extension (i.e. 'exe' or 'dll')
         */
        public override string GetBinaryExtension(UEBuildBinaryType InBinaryType)
        {
            switch (InBinaryType)
            {
                case UEBuildBinaryType.DynamicLinkLibrary:
                    return ".so";
                case UEBuildBinaryType.Executable:
                    return "";
                case UEBuildBinaryType.StaticLibrary:
                    return ".a";
            }
            return base.GetBinaryExtension(InBinaryType);
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
            return "";
        }

        /**
         *	Gives the platform a chance to 'override' the configuration settings 
         *	that are overridden on calls to RunUBT.
         *	
         *	@param	InPlatform			The UnrealTargetPlatform being built
         *	@param	InConfiguration		The UnrealTargetConfiguration being built
         */
        public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
        {
            ValidateUEBuildConfiguration();
        }

        /**
         *	Validate configuration for this platform
         *	NOTE: This function can/will modify BuildConfiguration!
         *	
         *	@param	InPlatform			The CPPTargetPlatform being built
         *	@param	InConfiguration		The CPPTargetConfiguration being built
         *	@param	bInCreateDebugInfo	true if debug info is getting create, false if not
         */
        public override void ValidateBuildConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
        {
            // increase Unity size to avoid too long command lines
            BuildConfiguration.NumIncludedBytesPerUnityCPP = 1024 * 1024;
        }

        /**
         *	Validate the UEBuildConfiguration for this platform
         *	This is called BEFORE calling UEBuildConfiguration to allow setting 
         *	various fields used in that function such as CompileLeanAndMean...
         */
        public override void ValidateUEBuildConfiguration()
        {
            BuildConfiguration.bUseUnityBuild = true;

            UEBuildConfiguration.bCompileLeanAndMeanUE = true;
            UEBuildConfiguration.bCompilePhysX = true;
            UEBuildConfiguration.bCompileAPEX = false;

            UEBuildConfiguration.bBuildEditor = false;
            UEBuildConfiguration.bBuildDeveloperTools = false;
            UEBuildConfiguration.bCompileSimplygon = false;
            UEBuildConfiguration.bCompileNetworkProfiler = false;

            // Don't stop compilation at first error...
            BuildConfiguration.bStopXGECompilationAfterErrors = true;

            BuildConfiguration.bUseSharedPCHs = false;
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
            return true;
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

        /**
         *	Whether the platform requires cooked data
         *	
         *	@param	InPlatform		The UnrealTargetPlatform being built
         *	@param	InConfiguration	The UnrealTargetConfiguration being built
         *	@return	bool			true if the platform requires cooked data, false if not
         */
        public override bool BuildRequiresCookedData(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
        {
            return true;
        }

        /**
         *	Get a list of extra modules the platform requires.
         *	This is to allow undisclosed platforms to add modules they need without exposing information about the platfomr.
         *	
         *	@param	Target						The target being build
         *	@param	BuildTarget					The UEBuildTarget getting build
         *	@param	PlatformExtraModules		OUTPUT the list of extra modules the platform needs to add to the target
         */
        public override void GetExtraModules(TargetInfo Target, UEBuildTarget BuildTarget, ref List<string> PlatformExtraModules)
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
        public override void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
        {
            if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
            {
                if (!UEBuildConfiguration.bBuildRequiresCookedData)
                {
                    if (InModule.ToString() == "Engine")
                    {
                        if (UEBuildConfiguration.bBuildDeveloperTools)
                        {
                            InModule.AddPlatformSpecificDynamicallyLoadedModule("LinuxTargetPlatform");
                        }
                    }
                }

                // allow standalone tools to use targetplatform modules, without needing Engine
                if (UEBuildConfiguration.bForceBuildTargetPlatforms)
                {
                    InModule.AddPlatformSpecificDynamicallyLoadedModule("LinuxTargetPlatform");
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
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_OGGVORBIS=0");

            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UNICODE");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_UNICODE");

            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_LINUX=1");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("LINUX=1");

            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_DATABASE_SUPPORT=0");		//@todo android: valid?
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_EDITOR=0");
            InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("USE_NULL_RHI=1");

            // link with Linux libraries.
            InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("pthread");
            InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("z");

            UEBuildConfiguration.bCompileSimplygon = false;
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
                case UnrealTargetConfiguration.Debug:
                default:
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
        }
    }
}
