// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using System.Collections.Generic;

namespace UnrealBuildTool
{
	public class BuildConfiguration
	{
		/// <summary>
		/// Whether to unify C++ code into larger files for faster compilation.
		/// </summary>
		public static bool bUseUnityBuild;
        /// <summary>
        /// Whether to _force_ unify C++ code into larger files for faster compilation.
        /// </summary>
        public static bool bForceUnityBuild;

		/// <summary>
		/// The number of source files in a game module before unity build will be activated for that module.  This
		/// allows small game modules to have faster iterative compile times for single files, at the expense of slower full
		/// rebuild times.  This setting can be overridden by the bFasterWithoutUnity option in a module's Build.cs file.
		/// </summary>
		public static int MinGameModuleSourceFilesForUnityBuild;

        /// <summary>
        /// Whether to build with Monolithic D3D Graphics drivers
        ///	Eventually this will become mandatory
        /// </summary>
        public static bool bUseMonolithicGraphicsDrivers;

		/// <summary>
		/// New Monolithic Graphics drivers have optional "fast calls" replacing various D3d functions
		/// </summary>
        public static bool bUseFastMonoCalls;

        /// <summary>
		/// An approximate number of bytes of C++ code to target for inclusion in a single unified C++ file.
		/// </summary>
		public static int NumIncludedBytesPerUnityCPP;

		/// <summary>
		/// Whether to stress test the C++ unity build robustness by including all C++ files files in a project from a single unified file.
		/// </summary>
		public static bool bStressTestUnity;

		/// <summary>
		/// Whether headers in system paths should be checked for modification when determining outdated actions.
		/// </summary>
		public static bool bCheckSystemHeadersForModification;

		/// <summary>
		/// Whether headers in the Development\External folder should be checked for modification when determining outdated actions.
		/// </summary>
		public static bool bCheckExternalHeadersForModification;

		/// <summary>
		/// Whether to ignore import library files that are out of date when building targets.  Set this to true to improve iteration time.
		/// </summary>
		public static bool bIgnoreOutdatedImportLibraries;

		/// <summary>
		/// Whether to globally disable debug info generation; see DebugInfoHeuristics.cs for per-config and per-platform options.
		/// </summary>
		public static bool bDisableDebugInfo;

		/// <summary>
		/// Whether to disable debug info on PC in development builds (for faster developer iteration, as link times are extremely fast with debug info disabled.)
		/// </summary>
		public static bool bOmitPCDebugInfoInDevelopment;

		/// <summary>
		/// Whether PDB files should be used for Visual C++ builds.
		/// </summary>
		public static bool bUsePDBFiles;

 		/// <summary>
 		/// Whether PCH files should be used.
		/// </summary>
		public static bool bUsePCHFiles;

		/// <summary>
		/// Whether to generate command line dependencies for compile actions when requested
		/// </summary>
		public static bool bUseActionHistory;

		/// <summary>
		/// The minimum number of files that must use a pre-compiled header before it will be created and used.
		/// </summary>
		public static int MinFilesUsingPrecompiledHeader;

		/// <summary>
		/// When enabled, a precompiled header is always generated for game modules, even if there are only a few source files
		/// in the module.  This greatly improves compile times for iterative changes on a few files in the project, at the expense of slower
		/// full rebuild times for small game projects.  This can be overridden by setting MinFilesUsingPrecompiledHeaderOverride in
		/// a module's Build.cs file
		/// </summary>
		public static bool bForcePrecompiledHeaderForGameModules;

		/// <summary>
		/// Whether debug info should be written to the console.
		/// </summary>
		public static bool bPrintDebugInfo;

		/// <summary>
		/// Prints performance diagnostics about include dependencies and other bits
		/// </summary>
		public static bool bPrintPerformanceInfo;

		/// <summary>
		/// Whether to log detailed action stats. This forces local execution.
		/// </summary>
		public static bool bLogDetailedActionStats;

		/// <summary>
		/// Whether to deploy the executable after compilation on platforms that require deployment.
		/// </summary>
		public static bool bDeployAfterCompile;

		/// <summary>
		/// Whether to clean Builds directory on a remote Mac before building
		/// </summary>
		public static bool bFlushBuildDirOnRemoteMac;

		/// <summary>
		/// Whether XGE may be used.
		/// </summary>
		public static bool bAllowXGE;

		/// <summary>
		/// Whether we should just export the XGE XML and pretend it succeeded
		/// </summary>
		public static bool bXGEExport;

		/// <summary>
		/// Whether to display the XGE build monitor.
		/// </summary>
		public static bool bShowXGEMonitor;

		/// <summary>
		/// When enabled, XGE will stop compiling targets after a compile error occurs.  Recommended, as it saves computing resources for others.
		/// </summary>
		public static bool bStopXGECompilationAfterErrors;

		/// <summary>
		/// When enabled, allows XGE to compile pre-compiled header files on remote machines.  Otherwise, PCHs are always generated locally.
		/// </summary>
		public static bool bAllowRemotelyCompiledPCHs;

		/// <summary>
		/// Whether or not to delete outdated produced items.
		/// </summary>
		public static bool bShouldDeleteAllOutdatedProducedItems;

		/// <summary>
		/// Whether to use incremental linking or not.
		/// </summary>
		public static bool bUseIncrementalLinking;

		/// <summary>
		/// Whether to allow the use of LTCG (link time code generation) 
		/// </summary>
		public static bool bAllowLTCG;

		/// <summary>
		/// Whether to support edit and continue.  Only works on Microsoft compilers in 32-bit compiles.
		/// </summary>
		public static bool bSupportEditAndContinue;

		/// <summary>
		/// Whether to omit frame pointers or not. Disabling is useful for e.g. memory profiling on the PC
		/// </summary>
		public static bool bOmitFramePointers;
        
		/// <summary>
		/// Processor count multiplier for local execution. Can be below 1 to reserve CPU for other tasks.
		/// </summary>
		public static double ProcessorCountMultiplier;

		/// <summary>
		/// The intermediate folder - i.e. Intermediate/Build.
		/// </summary>
		public static string BaseIntermediateFolder;

		/// <summary>
		/// Relative root engine path.
		/// </summary>
		public static string RelativeEnginePath
		{
			get
			{
				return "../../Engine/";
			}
		}

		/// <summary>
		/// The path to the intermediate folder, relative to Engine/Source.
		/// </summary>
		public static string BaseIntermediatePath
		{
			get
			{
				return RelativeEnginePath + BaseIntermediateFolder;
			}
		}

		/// <summary>
		/// The intermediate folder - i.e. Intermediate/Build/Platform[-Architecture] for the current platform
		/// </summary>
		private static string _PlatformIntermediateFolder = null;
		public static string PlatformIntermediateFolder
		{
			get
			{
				if (_PlatformIntermediateFolder == null)
				{
					throw new BuildException("Attempted to use PlatformIntermediateFolder before it was initialized");
				}
				return _PlatformIntermediateFolder;
			}
			set { _PlatformIntermediateFolder = value; }
		}

		/// <summary>
		/// The path to the platform intermediate folder, relative to Engine/Source.
		/// </summary>
		public static string PlatformIntermediatePath
		{
			get
			{
				return RelativeEnginePath + PlatformIntermediateFolder;
			}
		}

		/// <summary>
		/// Whether to generate a dSYM file or not.
		/// </summary>
		public static bool bGeneratedSYMFile;

		/// <summary>
		/// Whether to strip iOS symbols or not (implied by bGeneratedSYMFile).
		/// </summary>
		public static bool bStripSymbolsOnIOS;

		/// <summary>
		/// If true, then a stub IPA will be generated when compiling is done (minimal files needed for a valid IPA)
		/// </summary>
		public static bool bCreateStubIPA;

		/// <summary>
		/// If true, then a stub IPA will be generated when compiling is done (minimal files needed for a valid IPA)
		/// </summary>
		public static bool bCopyAppBundleBackToDevice;

		/// <summary>
		/// If true, then enable memory profiling in the build (defines USE_MALLOC_PROFILER=1 and forces bOmitFramePointers=false)
		/// </summary>
		public static bool bUseMallocProfiler;

		/// <summary>
		/// Enables code analysis mode.  Currently, this has specific requirements.  It only works on Windows
		/// platform with the MSVC compiler.  Also, it requires a version of the compiler that supports the
		/// /analyze option.  The compiler included with the Windows SDK supports this, as well as Team Edition
		/// versions of Visual Studio.
		/// </summary>
		public static bool bEnableCodeAnalysis;

		/// <summary>
		/// Enables "Shared PCHs", an experimental feature which may significantly speed up compile times by attempting to
		/// share certain PCH files between modules that UBT detects is including those PCH's header files
		/// </summary>
		public static bool bUseSharedPCHs;

		/// <summary>
		/// Writes an XML summary of the compile environment for the target to the given path, but does not build it.
		/// </summary>
		public static string WriteTargetInfoPath;

		/// <summary>
		/// Whether the dependency cache includes pre-resolved include locations so UBT doesn't have to re-resolve each include location just to check the timestamp.
		/// This is technically not fully correct because the dependency cache is global and each module could have a different set of include paths that could cause headers
		/// to resolve files differently. In practice this is not the case, and significantly speeds up UBT when nothing is to be done.
		/// </summary>
		public static bool bUseIncludeDependencyResolveCache;

		/// <summary>
		/// Used to test the dependency resolve cache. This will verify the resolve cache has no conflicts by resolving every time and checking against any previous resolve attempts.
		/// </summary>
		public static bool bTestIncludeDependencyResolveCache;

		/// <summary>
		/// True if we should compile Debug builds using Debug CRT libs and Debug third party dependencies.  Otherwise, we'll actually
		/// compile against the Release CRT but leave optimizations turned off, so that it is still easy to debug.  If you enable this
		/// setting, actual Debug versions of third party static libraries will be needed when compiling in Debug.
		/// </summary>
		public static bool bDebugBuildsActuallyUseDebugCRT;

		/// <summary>
		/// Sets the configuration back to defaults.
		/// </summary>
		public static void Reset()
		{
			bAllowLTCG = Utils.GetEnvironmentVariable("ue.bAllowLTCG", false);
			bAllowRemotelyCompiledPCHs = Utils.GetEnvironmentVariable("ue.bAllowRemotelyCompiledPCHs", false);
            bAllowXGE = Utils.GetEnvironmentVariable("ue.bAllowXGE", true);

			// Don't bother to check external (stable) headers for modification.  It slows down UBT's dependency checking.
			bCheckExternalHeadersForModification = Utils.GetEnvironmentVariable("ue.bCheckExternalHeadersForModification", false);
			bCheckSystemHeadersForModification = Utils.GetEnvironmentVariable("ue.bCheckSystemHeadersForModification", false);

			bCopyAppBundleBackToDevice = Utils.GetEnvironmentVariable("ue.bCopyAppBundleBackToDevice", false);
			bCreateStubIPA = Utils.GetEnvironmentVariable("ue.iPhone_CreateStubIPA", true);
			bDeployAfterCompile = false;
			bDisableDebugInfo = Utils.GetEnvironmentVariable("ue.bDisableDebugInfo", false);
			bEnableCodeAnalysis = false;
			bFlushBuildDirOnRemoteMac = false;
			bGeneratedSYMFile = Utils.GetEnvironmentVariable("ue.bGeneratedSYMFile", false);
			bStripSymbolsOnIOS = Utils.GetEnvironmentVariable("ue.bStripSymbolsOnIOS", bGeneratedSYMFile);

			// By default we don't bother relinking targets if only a dependent .lib has changed, as chances are that
			// the import library wasn't actually different unless a dependent header file of this target was also changed,
			// in which case the target would be rebuilt automatically.
			bIgnoreOutdatedImportLibraries = Utils.GetEnvironmentVariable("ue.bIgnoreOutdatedImportLibraries", true);

			bLogDetailedActionStats = Utils.GetEnvironmentVariable("ue.bLogDetailedActionStats", false);
			bOmitFramePointers = Utils.GetEnvironmentVariable("ue.bOmitFramePointers", true);
			bOmitPCDebugInfoInDevelopment = Utils.GetEnvironmentVariable("ue.bOmitPCDebugInfoInDevelopment", false);
			bPrintDebugInfo = Utils.GetEnvironmentVariable("ue.bPrintDebugInfo", false);
			bPrintPerformanceInfo = Utils.GetEnvironmentVariable("ue.bPrintPerformanceInfo", false);
			bShouldDeleteAllOutdatedProducedItems = Utils.GetEnvironmentVariable("ue.bShouldDeleteAllOutdatedProducedItems", false);
			bShowXGEMonitor = Utils.GetEnvironmentVariable("ue.bShowXGEMonitor", false);
			bStopXGECompilationAfterErrors = Utils.GetEnvironmentVariable("ue.bStopXGECompilationAfterErrors", true);
			bStressTestUnity = Utils.GetEnvironmentVariable("ue.bStressTestUnity", false);
			bSupportEditAndContinue = Utils.GetEnvironmentVariable("ue.bSupportEditAndContinue", false);
			bUseActionHistory = true;

			// Incremental linking can yield faster iteration times when making small changes
			// NOTE: We currently don't use incremental linking because it tends to behave a bit buggy on some computers (PDB-related compile errors)
			bUseIncrementalLinking = Utils.GetEnvironmentVariable("ue.bUseIncrementalLinking", false);

			bUseMallocProfiler = false;
			bUsePDBFiles = Utils.GetEnvironmentVariable("ue.bUsePDBFiles", false);
			bUsePCHFiles = Utils.GetEnvironmentVariable("ue.bUsePCHFiles", true);

			// Shared PCHs are necessary to avoid generating tons of private PCH files for every single little module.  While
			// these private PCHs do yield fastest incremental compiles for single files, it causes full rebuilds to
			// take an inordinate amount of time, and intermediates will use up many gigabytes of disk space.
			bUseSharedPCHs = true;
	
			WriteTargetInfoPath = null;

			// Using unity build to coalesce source files allows for much faster full rebuild times.  For fastest iteration times on single-file
			// changes, consider turning this off.  In some cases, unity builds can cause linker errors after iterative changes to files, because
			// the synthesized file names for unity code files may change between builds.
			bUseUnityBuild = Utils.GetEnvironmentVariable("ue.bUseUnityBuild", true);

            bForceUnityBuild = false;

			// For programmers working on gameplay code, it's convenient to have fast iteration times when changing only single source
			// files.  We've set this default such that smaller gameplay modules will not use the unity build feature, allowing them
			// the fastest possible iterative compile times at the expense of slower full rebuild times.  For larger game modules, unity
			// build will automatically be activated.
			MinGameModuleSourceFilesForUnityBuild = 32;

			// This controls how big unity files are.  Use smaller values for faster iterative iteration times at the cost of slower full rebuild times.
			// This setting can greatly affect how effective parallelized compilation can be.
			NumIncludedBytesPerUnityCPP = 256 * 1024;

			// This sets the number of source files (post-unity-combine) in a module before we'll bother to generate a precompiled header
			// for that module.  If you want the fastest iterative builds for non-header changes (at the expense of slower full rebuild times),
			// set this to value to 1
			MinFilesUsingPrecompiledHeader = 6;

			// We want iterative changes to gameplay modules to be very fast!  This makes full rebuilds for small game projects a bit slower, 
			// but changes to individual code files will compile quickly.
			bForcePrecompiledHeaderForGameModules = true;

			// When using the local executor (not XGE), run a single action on each CPU core.  Note that you can set this to a larger value
			// to get slightly faster build times in many cases, but your computer's responsiveness during compiling may be much worse.
			ProcessorCountMultiplier = Utils.GetEnvironmentVariable("ue.ProcessorCountMultiplier", 1.0);

			bTestIncludeDependencyResolveCache = false;
			// if we are testing the resolve cache, we require UBT to use it.
			bUseIncludeDependencyResolveCache = bTestIncludeDependencyResolveCache || Utils.GetEnvironmentVariable("ue.bUseIncludeDependencyResolveCache", true);
            
            //IMPORTANT THIS IS THE MAIN SWITCH FOR MONO.
            // New Monolithic Graphics drivers replace D3D and are *mostly* API compatible
            // Eventually this will become mandatory
            //  if this is set to true, then mono will be on by default on Dingo, and if false it will be off by default on Dingo.
            //  This can be overridden by -monolithicdrivers or -nomonolithicdrivers in the NMAKE params.
			bUseMonolithicGraphicsDrivers = true;
			//IMPORTANT THIS IS THE MAIN SWITCH FOR MONO FAST CALLS
			//  if this is set to true, then fast calls will be on by default on Dingo, and if false it will be off by default on Dingo.
			//  This can be overridden by -fastmonocalls  or -nofastmonocalls in the NMAKE params.
			bUseFastMonoCalls = true;

			// By default we use the Release C++ Runtime (CRT), even when compiling Debug builds.  This is because the Debug C++
			// Runtime isn't very useful when debugging Unreal Engine projects, and linking against the Debug CRT libraries forces
			// our third party library dependencies to also be compiled using the Debug CRT (and often perform more slowly.)  Often
			// it can be inconvenient to require a separate copy of the debug versions of third party static libraries simply
			// so that you can debug your program's code.
			bDebugBuildsActuallyUseDebugCRT = false;

			// set up some paths
			BaseIntermediateFolder = "Intermediate/Build/";
		}

		/// <summary>
		/// Validates the configuration. E.g. some options are mutually exclusive whereof some imply others. Also
		/// some functionality is not available on all platforms.
		/// @warning: the order of validation is important
		/// </summary>
		/// <param name="Configuration">Current configuration (e.g. development, debug, ...)</param>
		/// <param name="Platform">Current platform (e.g. Win32, PS3, ...)</param>
		/// <param name="bCreateDebugInfo">True if debug info should be created</param>
		public static void ValidateConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
		{
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatformForCPPTargetPlatform(Platform);

			// E&C support.
			if( bSupportEditAndContinue )
			{
				bUseIncrementalLinking = BuildPlatform.ShouldUseIncrementalLinking(Platform, Configuration);
			}

			// Incremental linking.
			if (bUseIncrementalLinking)
			{
				bUsePDBFiles = BuildPlatform.ShouldUsePDBFiles(Platform, Configuration, bCreateDebugInfo);
			}

			// Detailed stats
			if( bLogDetailedActionStats )
			{
				// Force local execution as we only have stats for local actions.
				bAllowXGE = false;
			}

			if (UnrealBuildTool.RunningRocket())
			{
				bAllowXGE = false;
			}

			// PDB
			if (bUsePDBFiles)
			{
				// NOTE: Currently we allow XGE to run, even with PDBs, until we notice an issue with this
				bool bDisallowXGEWithPDBFiles = false;
				if (bDisallowXGEWithPDBFiles)
				{
					// Force local execution as we have one PDB for all files using the same PCH. This currently doesn't
					// scale well with XGE due to required networking bandwidth. Xoreax mentioned that this was going to
					// be fixed in a future version of the software.
					bAllowXGE = false;
				}
			}

			// Allow for the build platform to perform custom validation here...
			// NOTE: This CAN modify the static BuildConfiguration settings!!!!
			BuildPlatform.ValidateBuildConfiguration(Configuration, Platform, bCreateDebugInfo);

            if (!BuildPlatform.CanUseXGE())
            {
                bAllowXGE = false;
            }
		}
	}
}
