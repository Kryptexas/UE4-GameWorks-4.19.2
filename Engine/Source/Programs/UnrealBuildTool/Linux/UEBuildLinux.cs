// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	class LinuxPlatform : UEBuildPlatform
	{
		/// <summary>
		/// This is the SDK version we support
		/// </summary>
		static private Dictionary<string, string> ExpectedSDKVersions = new Dictionary<string, string>()
		{
			{ "x86_64-unknown-linux-gnu", "v6_clang-3.6.0_ld-2.24_glibc-2.12.2" },
			{ "arm-unknown-linux-gnueabihf", "arm-unknown-linux-gnueabihf_v5_clang-3.5.0-ld-2.23.1-glibc-2.13" },
		};

		/// <summary>
		/// Platform name (embeds architecture for now)
		/// </summary>
		static private string TargetPlatformName = "Linux_x64";

		/// <summary>
		/// Linux architecture (compiler target triplet)
		/// </summary>
		// FIXME: for now switching between architectures is hard-coded
		static private string DefaultArchitecture = "x86_64-unknown-linux-gnu";
		//static private string DefaultArchitecture = "arm-unknown-linux-gnueabihf";

		/// <summary>
		/// The current architecture
		/// </summary>
		public override string GetActiveArchitecture()
		{
			return DefaultArchitecture;
		}

		/// <summary>
		/// Allow the platform to apply architecture-specific name according to its rules
		/// </summary>
		public override string ApplyArchitectureName(string BinaryName)
		{
			// Linux ignores architecture-specific names, although it might be worth it to prepend architecture
			return BinaryName;
		}

		/// <summary>
		/// Whether platform supports switching SDKs during runtime
		/// </summary>
		/// <returns>true if supports</returns>
		protected override bool PlatformSupportsAutoSDKs()
		{
			return true;
		}

		/// <summary>
		/// Returns platform-specific name used in SDK repository
		/// </summary>
		/// <returns>path to SDK Repository</returns>
		public override string GetSDKTargetPlatformName()
		{
			return TargetPlatformName;
		}

		/// <summary>
		/// Returns SDK string as required by the platform
		/// </summary>
		/// <returns>Valid SDK string</returns>
		protected override string GetRequiredSDKString()
		{
			string SDKString;
			if (!ExpectedSDKVersions.TryGetValue(GetActiveArchitecture(), out SDKString))
			{
				throw new BuildException("LinuxPlatform::GetRequiredSDKString: no toolchain set up for architecture '{0}'", GetActiveArchitecture());
			}

			return SDKString;
		}

		protected override String GetRequiredScriptVersionString()
		{
			return "3.0";
		}

		protected override bool PreferAutoSDK()
		{
			// having LINUX_ROOT set (for legacy reasons or for convenience of cross-compiling certain third party libs) should not make UBT skip AutoSDKs
			return true;
		}

		/// <summary>
		/// Whether the required external SDKs are installed for this platform
		/// </summary>
		protected override SDKStatus HasRequiredManualSDKInternal()
		{
			if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Linux)
			{
				return SDKStatus.Valid;
			}

			string BaseLinuxPath = Environment.GetEnvironmentVariable("LINUX_ROOT");

			// we don't have an LINUX_ROOT specified
			if (String.IsNullOrEmpty(BaseLinuxPath))
				return SDKStatus.Invalid;

			// paths to our toolchains
			BaseLinuxPath = BaseLinuxPath.Replace("\"", "");
			string ClangPath = Path.Combine(BaseLinuxPath, @"bin\Clang++.exe");

			if (File.Exists(ClangPath))
				return SDKStatus.Valid;

			return SDKStatus.Invalid;
		}

		public override bool CanUseXGE()
		{
			// [RCL] 2015-08-04 FIXME: modular (cross-)builds (e.g. editor, UT server) fail with XGE as FixDeps step apparently depends on artifacts (object files) which aren't listed among its prerequisites.
			return false;
		}

		/// <summary>
		/// Register the platform with the UEBuildPlatform class
		/// </summary>
		protected override void RegisterBuildPlatformInternal()
		{
			if ((ProjectFileGenerator.bGenerateProjectFiles == true) || (HasRequiredSDKsInstalled() == SDKStatus.Valid))
			{
				bool bRegisterBuildPlatform = true;

				string EngineSourcePath = Path.Combine(ProjectFileGenerator.RootRelativePath, "Engine", "Source");
				string LinuxTargetPlatformFile = Path.Combine(EngineSourcePath, "Developer", "Linux", "LinuxTargetPlatform", "LinuxTargetPlatform.Build.cs");

				if (File.Exists(LinuxTargetPlatformFile) == false)
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

		/// <summary>
		/// Retrieve the CPPTargetPlatform for the given UnrealTargetPlatform
		/// </summary>
		/// <param name="InUnrealTargetPlatform"> The UnrealTargetPlatform being build</param>
		/// <returns>CPPTargetPlatform   The CPPTargetPlatform to compile for</returns>
		public override CPPTargetPlatform GetCPPTargetPlatform(UnrealTargetPlatform InUnrealTargetPlatform)
		{
			switch (InUnrealTargetPlatform)
			{
				case UnrealTargetPlatform.Linux:
					return CPPTargetPlatform.Linux;
			}
			throw new BuildException("LinuxPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
		}

		/// <summary>
		/// Get the extension to use for the given binary type
		/// </summary>
		/// <param name="InBinaryType"> The binary type being built</param>
		/// <returns>string    The binary extension (i.e. 'exe' or 'dll')</returns>
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
				case UEBuildBinaryType.Object:
					return ".o";
				case UEBuildBinaryType.PrecompiledHeader:
					return ".gch";
			}
			return base.GetBinaryExtension(InBinaryType);
		}

		/// <summary>
		/// Get the extension to use for debug info for the given binary type
		/// </summary>
		/// <param name="InBinaryType"> The binary type being built</param>
		/// <returns>string    The debug info extension (i.e. 'pdb')</returns>
		public override string GetDebugInfoExtension(UEBuildBinaryType InBinaryType)
		{
			return "";
		}

		/// <summary>
		/// Gives the platform a chance to 'override' the configuration settings
		/// that are overridden on calls to RunUBT.
		/// </summary>
		/// <param name="InPlatform">  The UnrealTargetPlatform being built</param>
		/// <param name="InConfiguration"> The UnrealTargetConfiguration being built</param>
		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			ValidateUEBuildConfiguration();
		}

		/// <summary>
		/// Validate configuration for this platform
		/// NOTE: This function can/will modify BuildConfiguration!
		/// </summary>
		/// <param name="InPlatform">  The CPPTargetPlatform being built</param>
		/// <param name="InConfiguration"> The CPPTargetConfiguration being built</param>
		/// <param name="bInCreateDebugInfo">true if debug info is getting create, false if not</param>
		public override void ValidateBuildConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
		{
			UEBuildConfiguration.bCompileSimplygon = false;
		}

		/// <summary>
		/// Validate the UEBuildConfiguration for this platform
		/// This is called BEFORE calling UEBuildConfiguration to allow setting
		/// various fields used in that function such as CompileLeanAndMean...
		/// </summary>
		public override void ValidateUEBuildConfiguration()
		{
			if (ProjectFileGenerator.bGenerateProjectFiles && !ProjectFileGenerator.bGeneratingRocketProjectFiles)
			{
				// When generating non-Rocket project files we need intellisense generator to include info from all modules,
				// including editor-only third party libs
				UEBuildConfiguration.bCompileLeanAndMeanUE = false;
			}

			BuildConfiguration.bUseUnityBuild = true;

			// Don't stop compilation at first error...
			BuildConfiguration.bStopXGECompilationAfterErrors = true;

			BuildConfiguration.bUseSharedPCHs = false;
		}

		/// <summary>
		/// Whether PDB files should be used
		/// </summary>
		/// <param name="InPlatform">  The CPPTargetPlatform being built</param>
		/// <param name="InConfiguration"> The CPPTargetConfiguration being built</param>
		/// <param name="bInCreateDebugInfo">true if debug info is getting create, false if not</param>
		/// <returns>bool true if PDB files should be used, false if not</returns>
		public override bool ShouldUsePDBFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration, bool bCreateDebugInfo)
		{
			return true;
		}

		/// <summary>
		/// Get a list of extra modules the platform requires.
		/// This is to allow undisclosed platforms to add modules they need without exposing information about the platfomr.
		/// </summary>
		/// <param name="Target">     The target being build</param>
		/// <param name="BuildTarget">    The UEBuildTarget getting build</param>
		/// <param name="PlatformExtraModules"> OUTPUT the list of extra modules the platform needs to add to the target</param>
		public override void GetExtraModules(TargetInfo Target, UEBuildTarget BuildTarget, ref List<string> PlatformExtraModules)
		{
		}

		/// <summary>
		/// Modify the newly created module passed in for this platform.
		/// This is not required - but allows for hiding details of a
		/// particular platform.
		/// </summary>
		/// <param name="">Name   The name of the module</param>
		/// <param name="Rules">  The module rules</param>
		/// <param name="Target">  The target being build</param>
		public override void ModifyModuleRules(string ModuleName, ModuleRules Rules, TargetInfo Target)
		{
			if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
			{
				if (!UEBuildConfiguration.bBuildRequiresCookedData)
				{
					if (ModuleName == "Engine")
					{
						if (UEBuildConfiguration.bBuildDeveloperTools)
						{
							Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("LinuxTargetPlatform");
							Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("LinuxNoEditorTargetPlatform");
							Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("LinuxServerTargetPlatform");
						}
					}
				}

				// allow standalone tools to use targetplatform modules, without needing Engine
				if (UEBuildConfiguration.bForceBuildTargetPlatforms && ModuleName == "TargetPlatform")
				{
					Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("LinuxTargetPlatform");
					Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("LinuxNoEditorTargetPlatform");
					Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("LinuxServerTargetPlatform");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				bool bBuildShaderFormats = UEBuildConfiguration.bForceBuildShaderFormats;

				if (!UEBuildConfiguration.bBuildRequiresCookedData)
				{
					if (ModuleName == "TargetPlatform")
					{
						bBuildShaderFormats = true;
					}
				}

				// allow standalone tools to use target platform modules, without needing Engine
				if (ModuleName == "TargetPlatform")
				{
					if (UEBuildConfiguration.bForceBuildTargetPlatforms)
					{
						Rules.DynamicallyLoadedModuleNames.Add("LinuxTargetPlatform");
						Rules.DynamicallyLoadedModuleNames.Add("LinuxNoEditorTargetPlatform");
						Rules.DynamicallyLoadedModuleNames.Add("LinuxServerTargetPlatform");
						Rules.DynamicallyLoadedModuleNames.Add("AllDesktopTargetPlatform");
					}

					if (bBuildShaderFormats)
					{
						// Rules.DynamicallyLoadedModuleNames.Add("ShaderFormatD3D");
						Rules.DynamicallyLoadedModuleNames.Add("ShaderFormatOpenGL");
					}
				}
			}
		}

		/// <summary>
		/// Setup the target environment for building
		/// </summary>
		/// <param name="InBuildTarget"> The target being built</param>
		public override void SetUpEnvironment(UEBuildTarget InBuildTarget)
		{
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_LINUX=1");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("LINUX=1");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_DATABASE_SUPPORT=0");		//@todo linux: valid?

			if (GetActiveArchitecture().StartsWith("arm"))
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("REQUIRES_ALIGNED_INT_ACCESS");
			}

			// link with Linux libraries.
			InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("pthread");

			// Disable Simplygon support if compiling against the NULL RHI.
			if (InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Contains("USE_NULL_RHI=1"))
			{
				UEBuildConfiguration.bCompileSimplygon = false;
			}

			if (InBuildTarget.TargetType == TargetRules.TargetType.Server)
			{
				// Localization shouldn't be needed on servers by default, and ICU is pretty heavy
				UEBuildConfiguration.bCompileICU = false;
			}
		}

		/// <summary>
		/// Whether this platform should create debug information or not
		/// </summary>
		/// <param name="InPlatform">  The UnrealTargetPlatform being built</param>
		/// <param name="InConfiguration"> The UnrealTargetConfiguration being built</param>
		/// <returns>bool    true if debug info should be generated, false if not</returns>
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

		/// <summary>
		/// Setup the binaries for this specific platform.
		/// </summary>
		/// <param name="InBuildTarget"> The target being built</param>
		public override void SetupBinaries(UEBuildTarget InBuildTarget)
		{
		}
	}
}
