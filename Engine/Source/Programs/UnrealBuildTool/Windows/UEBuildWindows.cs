// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;
using Microsoft.Win32;

namespace UnrealBuildTool
{
	/// <summary>
	/// Available compiler toolchains on Windows platform
	/// </summary>
	public enum WindowsCompiler
	{
		/// Visual Studio 2012 (Visual C++ 11.0)
		VisualStudio2012,

		/// Visual Studio 2013 (Visual C++ 12.0)
		VisualStudio2013,
	}


	public class WindowsPlatform : UEBuildPlatform
	{
		/// Version of the compiler toolchain to use on Windows platform
		public static readonly WindowsCompiler Compiler = WindowsCompiler.VisualStudio2013;

		/// True if we are using "clang-cl" to compile instead of MSVC on Windows platform
		public static readonly bool bCompileWithClang = false;
			// !UnrealBuildTool.CommandLineContains( "UnrealHeaderTool" );	// @todo clang: Avoid using Clang to compile UHT because 64-bit toolchain is not ready yet

		/// True if we're targeting Windows XP as a minimum spec.  In Visual Studio 2012 and higher, this may change how
		/// we compile and link the application (http://blogs.msdn.com/b/vcblog/archive/2012/10/08/10357555.aspx)
		public static readonly bool SupportWindowsXP = false;

		/** True if VS EnvDTE is available (false when building using Visual Studio Express) */
		public static bool bHasVisualStudioDTE
		{
			get
			{
				try
				{
					// Interrogate the Win32 registry
					return RegistryKey.OpenBaseKey(RegistryHive.ClassesRoot, RegistryView.Registry32).OpenSubKey("VisualStudio.DTE") != null;
				}
				catch(Exception) 
				{
					return false;
				}
			}
		}


		/**
		 *	Register the platform with the UEBuildPlatform class
		 */
		public override void RegisterBuildPlatform()
		{
			// Register this build platform for both Win64 and Win32
			Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.Win64.ToString());
			UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.Win64, this);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Win64, UnrealPlatformGroup.Windows);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Win64, UnrealPlatformGroup.Microsoft);

			Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.Win32.ToString());
			UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.Win32, this);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Win32, UnrealPlatformGroup.Windows);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Win32, UnrealPlatformGroup.Microsoft);
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
				case UnrealTargetPlatform.Win32:
					return CPPTargetPlatform.Win32;

				case UnrealTargetPlatform.Win64:
					return CPPTargetPlatform.Win64;
			}
			throw new BuildException("WindowsPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
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
				case WindowsCompiler.VisualStudio2012:
					return "2012";

				case WindowsCompiler.VisualStudio2013:
					return "2013";

				default:
					throw new BuildException( "Unexpected WindowsCompiler version for GetVisualStudioCompilerVersionName().  Either not using a Visual Studio compiler or switch block needs to be updated");
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
                    return ".pdb";
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
			return false;
		}

		public override bool BuildRequiresCookedData(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return false;
		}

		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
		}

        public override void ValidateBuildConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
        {
			// @todo clang: PCH files aren't quite working yet with "clang-cl" (no /Yc support, and "-x c++-header" cannot be specified)
			if( WindowsPlatform.bCompileWithClang )
			{
				BuildConfiguration.bUsePCHFiles = false;
				BuildConfiguration.bUseSharedPCHs = false;
			}
        }

		/**
         *	Validate the UEBuildConfiguration for this platform
         *	This is called BEFORE calling UEBuildConfiguration to allow setting 
         *	various fields used in that function such as CompileLeanAndMean...
         */
        public override void ValidateUEBuildConfiguration()
        {
            UEBuildConfiguration.bCompileICU = !bCompileWithClang;	// @todo clang: ICU causes STL link errors when using Clang on Windows.  Needs debugging.
        }

		public override void ModifyNewlyLoadedModule(UEBuildModule InModule, TargetInfo Target)
		{
			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
			{
                bool bBuildShaderFormats = UEBuildConfiguration.bForceBuildShaderFormats;

                if (!UEBuildConfiguration.bBuildRequiresCookedData)
                {
                    if (InModule.ToString() == "TargetPlatform")
                    {
                        bBuildShaderFormats = true;
                    }
                }

				// allow standalone tools to use target platform modules, without needing Engine
				if (UEBuildConfiguration.bForceBuildTargetPlatforms)
				{
					InModule.AddDynamicallyLoadedModule("WindowsTargetPlatform");
					InModule.AddDynamicallyLoadedModule("WindowsNoEditorTargetPlatform");
					InModule.AddDynamicallyLoadedModule("WindowsServerTargetPlatform");
					InModule.AddDynamicallyLoadedModule("WindowsClientTargetPlatform");
				}

                if (bBuildShaderFormats)
                {
                    InModule.AddDynamicallyLoadedModule("ShaderFormatD3D");
                    InModule.AddDynamicallyLoadedModule("ShaderFormatOpenGL");
                }

				if (InModule.ToString() == "D3D11RHI")
				{
					// To enable platform specific D3D11 RHI Types
					InModule.AddPrivateIncludePath("Runtime/Windows/D3D11RHI/Private/Windows");
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
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WIN32=1");
			if( SupportWindowsXP )
			{
				// Windows XP SP3 or higher required
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_WIN32_WINNT=0x0502");
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WINVER=0x0502");
			}
			else
			{
				// Windows Vista or higher required
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("_WIN32_WINNT=0x0600");
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WINVER=0x0600");
			}
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_WINDOWS=1");

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

				//@todo ATL: Currently, only VSAccessor requires ATL (which is only used in editor builds)
				// When compiling games, we do not want to include ATL - and we can't when compiling Rocket games
				// due to VS 2012 Express not including ATL.
				// If more modules end up requiring ATL, this should be refactored into a BuildTarget flag (bNeedsATL)
				// that is set by the modules the target includes to allow for easier tracking.
				// Alternatively, if VSAccessor is modified to not require ATL than we should always exclude the libraries.
				if (InBuildTarget.ShouldCompileMonolithic() && (InBuildTarget.Rules != null) &&
					(TargetRules.IsGameType(InBuildTarget.Rules.Type)) && (TargetRules.IsEditorType(InBuildTarget.Rules.Type) == false))
				{
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("atl");
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("atls");
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("atlsd");
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("atlsn");
					InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("atlsnd");
				}

				// Add the library used for the delayed loading of DLLs.
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("delayimp.lib");

				//@todo - remove once FB implementation uses Http module
				if (UEBuildConfiguration.bCompileAgainstEngine)
				{
					// link against wininet (used by FBX and Facebook)
					InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("wininet.lib");
				}

				// Compile and link with Win32 API libraries.
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("rpcrt4.lib");
				//InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("wsock32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("ws2_32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("dbghelp.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("comctl32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("Winmm.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("kernel32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("user32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("gdi32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("winspool.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("comdlg32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("advapi32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("shell32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("ole32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("oleaut32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("uuid.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("odbc32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("odbccp32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("netapi32.lib");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("iphlpapi.lib");
                InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("setupapi.lib"); //  Required for access monitor device enumeration

				// Windows Vista/7 Desktop Windows Manager API for Slate Windows Compliance
				if( !SupportWindowsXP )		// Windows XP does not support DWM
				{
					InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("dwmapi.lib");
				}

				// Setup assembly path for .NET modules.  This will only be used when CLR is enabled. (CPlusPlusCLR module types)
				InBuildTarget.GlobalCompileEnvironment.Config.SystemDotNetAssemblyPaths.Add(
					Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86) +
					"/Reference Assemblies/Microsoft/Framework/.NETFramework/v4.0");

				// Setup default assemblies for .NET modules.  These will only be used when CLR is enabled. (CPlusPlusCLR module types)
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("System.dll");
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("System.Data.dll");
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("System.Drawing.dll");
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("System.Xml.dll");
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("System.Management.dll");
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("System.Windows.Forms.dll");
				InBuildTarget.GlobalCompileEnvironment.Config.FrameworkAssemblyDependencies.Add("WindowsBase.dll");
			}

			// Disable Simplygon support if compiling against the NULL RHI.
			if (InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Contains("USE_NULL_RHI=1"))
			{
				UEBuildConfiguration.bCompileSimplygon = false;
			}

			// For 64-bit builds, we'll forcibly ignore a linker warning with DirectInput.  This is
			// Microsoft's recommended solution as they don't have a fixed .lib for us.
			if (InBuildTarget.Platform == UnrealTargetPlatform.Win64)
			{
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalArguments += " /ignore:4078";
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
			//@todo SAS: Add a true Debug mode!
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
			InBuildTarget.GlobalCompileEnvironment.Config.TargetConfiguration = CompileConfiguration;
			InBuildTarget.GlobalLinkEnvironment.Config.TargetConfiguration = CompileConfiguration;

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
