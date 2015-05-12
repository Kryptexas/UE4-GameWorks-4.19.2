// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool
{
	public class UWPPlatform : UEBuildPlatform
	{
		/// Property caching.
		private static WindowsCompiler? CachedCompiler;

		/// Version of the compiler toolchain to use for Universal Windows Platform apps (UWP)
		public static WindowsCompiler Compiler
		{
			get
			{
				// Cache the result because Compiler is often used.
				if (CachedCompiler.HasValue)
				{
					return CachedCompiler.Value;
				}

				// First, default based on whether there is a command line override.
				// Allows build chain to partially progress even in the absence of installed tools
				if (UnrealBuildTool.CommandLineContains("-2015"))
				{
					CachedCompiler = WindowsCompiler.VisualStudio2015;
				}
				// Second, default based on what's installed
				else if (!String.IsNullOrEmpty(WindowsPlatform.GetVSComnToolsPath(WindowsCompiler.VisualStudio2015)))
				{
					CachedCompiler = WindowsCompiler.VisualStudio2015;
				}
				else
				{
					CachedCompiler = null;
				}

				return CachedCompiler.Value;
			}
		}

		// Enables the UWP platform and project file support in Unreal Build Tool
		// @todo UWP: Remove this variable when UWP support is fully implemented
		public static readonly bool bEnableUWPSupport = UnrealBuildTool.CommandLineContains("-uwp");

		/// True if we should only build against the app-local CRT and /APPCONTAINER linker flag
		public static readonly bool bBuildForStore = true;

		/// True if we should only build against the app-local CRT and set the API-set flags
		public static readonly bool bWinApiFamilyApp = true;

		/** True if VS EnvDTE is available (false when building using Visual Studio Express) */
		public static bool bHasVisualStudioDTE
		{
			get
			{
				return WindowsPlatform.bHasVisualStudioDTE;
			}
		}

		public override bool RequiresDeployPrepAfterCompile()
		{
			return true;
		}

		protected override SDKStatus HasRequiredManualSDKInternal()
		{
			try
			{
				// @todo UWP: wire this up once the rest of cleanup has been resolved. right now, just leaving this to always return valid, like VC does.
				//WinUWPToolChain.FindBaseVSToolPath();
				return SDKStatus.Valid;
			}
			catch (BuildException)
			{
				return SDKStatus.Invalid;
			}
		}

		/**
		 * Returns VisualStudio common tools path for current compiler.
		 * 
		 * @return Common tools path.
		 */
		public static string GetVSComnToolsPath()
		{
			return GetVSComnToolsPath(Compiler);
		}

		/**
		 * Returns VisualStudio common tools path for given compiler.
		 * 
		 * @param Compiler Compiler for which to return tools path.
		 * 
		 * @return Common tools path.
		 */
		public static string GetVSComnToolsPath(WindowsCompiler Compiler)
		{
			int VSVersion;

			switch(Compiler)
			{
				case WindowsCompiler.VisualStudio2015:
					VSVersion = 14;
					break;
				default:
					throw new NotSupportedException("Not supported compiler.");
			}

			string[] PossibleRegPaths = new string[] {
				@"Wow6432Node\Microsoft\VisualStudio",	// Non-express VS on 64-bit machine.
				@"Microsoft\VisualStudio",				// Non-express VS on 32-bit machine.
				@"Wow6432Node\Microsoft\WDExpress",		// Express VS on 64-bit machine.
				@"Microsoft\WDExpress"					// Express VS on 32-bit machine.
			};

			string VSPath = null;

			foreach(var PossibleRegPath in PossibleRegPaths)
			{
				VSPath = (string) Registry.GetValue(string.Format(@"HKEY_LOCAL_MACHINE\SOFTWARE\{0}\{1}.0", PossibleRegPath, VSVersion), "InstallDir", null);

				if(VSPath != null)
				{
					break;
				}
			}

			if(VSPath == null)
			{
				return null;
			}

			return new DirectoryInfo(Path.Combine(VSPath, "..", "Tools")).FullName;
		}


		/**
		 *	Register the platform with the UEBuildPlatform class
		 */
		protected override void RegisterBuildPlatformInternal()
		{
			// Register this build platform for UWP
			Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.UWP.ToString());
			UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.UWP, this);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.UWP, UnrealPlatformGroup.Microsoft);
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
				case UnrealTargetPlatform.UWP:
					return CPPTargetPlatform.UWP;
			}
			throw new BuildException("UWPPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
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
			switch (InBinaryType)
			{
				case UEBuildBinaryType.DynamicLinkLibrary: 
					return ".dll";
				case UEBuildBinaryType.Executable:
					return ".exe";
				case UEBuildBinaryType.StaticLibrary:
					return ".lib";
				case UEBuildBinaryType.Object:
					return ".obj";
				case UEBuildBinaryType.PrecompiledHeader:
					return ".pch";
			}
			return base.GetBinaryExtension(InBinaryType);
		}

		
		/// <summary>
		/// When using a Visual Studio compiler, returns the version name as a string
		/// </summary>
		/// <returns>The Visual Studio compiler version name (e.g. "2012")</returns>
		public static string GetVisualStudioCompilerVersionName()
		{
			switch( Compiler )
			{ 
				case WindowsCompiler.VisualStudio2015:
					return "2015";
				default:
					throw new BuildException("Unexpected WindowsCompiler version for GetVisualStudioCompilerVersionName().  Either not using a Visual Studio compiler or switch block needs to be updated");
			}
		}


		/**
		 *	Get the extension to use for debug info for the given binary type
		 *	
		 *	@param	InBinaryType		The binary type being built
		 *	
		 *	@return	string				The debug info extension (i.e. 'pdb')
		 */
		public override string GetDebugInfoExtension( UEBuildBinaryType InBinaryType )
		{
			switch (InBinaryType)
			{
				case UEBuildBinaryType.DynamicLinkLibrary:
				case UEBuildBinaryType.Executable:
					return ".pdb";
			}
			return "";
		}

		/**
		 *	Whether incremental linking should be used
		 *	
		 *	@param	InPlatform			The CPPTargetPlatform being built
		 *	@param	InConfiguration		The CPPTargetConfiguration being built
		 *	
		 *	@return	bool	true if incremental linking should be used, false if not
		 */
		public override bool ShouldUseIncrementalLinking(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration)
		{
			return (Configuration == CPPTargetConfiguration.Debug);
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
			// Only supported on PC.
			if (bCreateDebugInfo && ShouldUseIncrementalLinking(Platform, Configuration))
			{
				return true;
			}
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
			return false;
		}

		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			UEBuildConfiguration.bCompileICU = true;
		}


		public override void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
		{
			if ((Target.Platform == UnrealTargetPlatform.UWP))
			{
				if (InModule.ToString() == "Core")
				{
					InModule.AddPrivateDependencyModule("UWPSDK");
				}
				else if (InModule.ToString() == "Engine")
				{
					InModule.AddPrivateDependencyModule("zlib");
					InModule.AddPrivateDependencyModule("UElibPNG");
					InModule.AddPublicDependencyModule("UEOgg");
					InModule.AddPublicDependencyModule("Vorbis");
				}
				else if (InModule.ToString() == "D3D11RHI")
				{
					InModule.AddPublicDefinition("D3D11_WITH_DWMAPI=0");
					InModule.AddPublicDefinition("WITH_DX_PERF=0");
				}
				else if (InModule.ToString() == "DX11")
				{
					// Clear out all the Windows include paths and libraries...
					// The UWPSDK module handles proper paths and libs for UWP.
					// However, the D3D11RHI module will include the DX11 module.
					InModule.ClearPublicIncludePaths();
					InModule.ClearPublicLibraryPaths();
					InModule.ClearPublicAdditionalLibraries();
					InModule.RemovePublicDefinition("WITH_D3DX_LIBS=1");
					InModule.AddPublicDefinition("WITH_D3DX_LIBS=0");
					InModule.RemovePublicAdditionalLibrary("X3DAudio.lib");
					InModule.RemovePublicAdditionalLibrary("XAPOFX.lib");
				}
				else if (InModule.ToString() == "XAudio2")
				{
					InModule.AddPublicDefinition("XAUDIO_SUPPORTS_XMA2WAVEFORMATEX=0");
					InModule.AddPublicDefinition("XAUDIO_SUPPORTS_DEVICE_DETAILS=0");
					InModule.AddPublicDefinition("XAUDIO2_SUPPORTS_MUSIC=0");
					InModule.AddPublicDefinition("XAUDIO2_SUPPORTS_SENDLIST=0");
					InModule.AddPublicAdditionalLibrary("XAudio2.lib");
				}
				else if (InModule.ToString() == "DX11Audio")
				{
					InModule.RemovePublicAdditionalLibrary("X3DAudio.lib");
					InModule.RemovePublicAdditionalLibrary("XAPOFX.lib");
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

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_DESKTOP=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_64BITS=1");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("EXCEPTIONS_DISABLED=0");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_WIN32_WINNT=0x0A00");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WINVER=0x0A00");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_UWP=1");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("UWP=1");	// @todo UWP: This used to be "WINUAP=1".  Need to verify that 'UWP' is the correct define that we want here.

			if (bWinApiFamilyApp)
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WINAPI_FAMILY=WINAPI_FAMILY_APP");
			}

			// @todo UWP: UE4 is non-compliant when it comes to use of %s and %S
			// Previously %s meant "the current character set" and %S meant "the other one".
			// Now %s means multibyte and %S means wide. %Ts means "natural width".
			// Reverting this behavior until the UE4 source catches up.
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_CRT_STDIO_LEGACY_WIDE_SPECIFIERS=1");

			// No D3DX on UWP!
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("NO_D3DX_LIBS=1");


			if (InBuildTarget.Rules != null)
			{
				// Explicitly exclude the MS C++ runtime libraries we're not using, to ensure other libraries we link with use the same
				// runtime library as the engine.
				if (InBuildTarget.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT)
				{
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("MSVCRT");
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("MSVCPRT");
				}
				else
				{
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("MSVCRTD");
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("MSVCPRTD");
				}
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBC");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCMT");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCPMT");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCP");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCD");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCMTD");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCPMTD");
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCPD");
			}

			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("mincore.lib");
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("dloadhelper.lib");
		   
			// Disable Simplygon support if compiling against the NULL RHI.
			if (InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Contains("USE_NULL_RHI=1"))
			{
				UEBuildConfiguration.bCompileSimplygon = false;
			}
		}

		/**
		 *	Setup the configuration environment for building
		 *	
		 *	@param	InBuildTarget		The target being built
		 */
		public override void SetUpConfigurationEnvironment(UEBuildTarget InBuildTarget)
		{
			// Determine the C++ compile/link configuration based on the Unreal configuration.
			CPPTargetConfiguration CompileConfiguration;
			UnrealTargetConfiguration CheckConfig = InBuildTarget.Configuration;
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
					// Default to Development; can be overriden by individual modules.
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
			InBuildTarget.GlobalCompileEnvironment.Config.Target.Configuration = CompileConfiguration;
			InBuildTarget.GlobalLinkEnvironment.Config.Target.Configuration    = CompileConfiguration;

			// Create debug info based on the heuristics specified by the user.
			InBuildTarget.GlobalCompileEnvironment.Config.bCreateDebugInfo =
				!BuildConfiguration.bDisableDebugInfo && ShouldCreateDebugInfo(InBuildTarget.Platform, CheckConfig);

			// NOTE: Even when debug info is turned off, we currently force the linker to generate debug info
			//       anyway on Visual C++ platforms.  This will cause a PDB file to be generated with symbols
			//       for most of the classes and function/method names, so that crashes still yield somewhat
			//       useful call stacks, even though compiler-generate debug info may be disabled.  This gives
			//       us much of the build-time savings of fully-disabled debug info, without giving up call
			//       data completely.
			InBuildTarget.GlobalLinkEnvironment.Config.bCreateDebugInfo = true;
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
				case UnrealTargetConfiguration.DebugGame:
				case UnrealTargetConfiguration.Debug:
				default: 
					return true;
			};
		}

		/**
		 *	Return whether this platform has uniquely named binaries across multiple games
		 */
		public override bool HasUniqueBinaries()
		{
			// Windows applications have many shared binaries between games
			return false;
		}
	}
}
