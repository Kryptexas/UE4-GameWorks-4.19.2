// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	class HTML5Platform : UEBuildPlatform
	{
		// use -win32 for win32 builds. ( build html5 platform as a win32 binary for debugging )
		[XmlConfig]
		public static string HTML5Architecture = "";

		/// <summary>
		/// Whether platform supports switching SDKs during runtime
		/// </summary>
		/// <returns>true if supports</returns>
		protected override bool PlatformSupportsAutoSDKs()
		{
			return false;
		}

		public override string GetSDKTargetPlatformName()
		{
			return "HTML5";
		}

		/// <summary>
		/// Returns SDK string as required by the platform
		/// </summary>
		/// <returns>Valid SDK string</returns>
		protected override string GetRequiredSDKString()
		{
			return HTML5SDKInfo.EmscriptenVersion();
		}

		protected override String GetRequiredScriptVersionString()
		{
			return "3.0";
		}

		// The current architecture - affects everything about how UBT operates on HTML5
		public override string GetActiveArchitecture()
		{
			// by default, use an empty architecture (which is really just a modifier to the platform for some paths/names)
			return HTML5Architecture;
		}

		// F5 should always try to run the Win32 version
		public override FileReference ModifyNMakeOutput(FileReference ExeName)
		{
			// nmake Run should always run the win32 version
			return (ExeName + "-win32").ChangeExtension(".exe");
		}

		/// <summary>
		/// Whether the required external SDKs are installed for this platform
		/// </summary>
		protected override SDKStatus HasRequiredManualSDKInternal()
		{
			if (!HTML5SDKInfo.IsSDKInstalled())
			{
				return SDKStatus.Invalid;
			}
			return SDKStatus.Valid;
		}

		public override bool CanUseXGE()
		{
			return (GetActiveArchitecture() == "-win32");
		}

		/// <summary>
		/// Register the platform with the UEBuildPlatform class
		/// </summary>
		protected override void RegisterBuildPlatformInternal()
		{
			// Make sure the SDK is installed
			if ((ProjectFileGenerator.bGenerateProjectFiles == true) || (HasRequiredSDKsInstalled() == SDKStatus.Valid))
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

		/// <summary>
		/// Retrieve the CPPTargetPlatform for the given UnrealTargetPlatform
		/// </summary>
		/// <param name="InUnrealTargetPlatform"> The UnrealTargetPlatform being build</param>
		/// <returns>CPPTargetPlatform   The CPPTargetPlatform to compile for</returns>
		public override CPPTargetPlatform GetCPPTargetPlatform(UnrealTargetPlatform InUnrealTargetPlatform)
		{
			switch (InUnrealTargetPlatform)
			{
				case UnrealTargetPlatform.HTML5:
					return CPPTargetPlatform.HTML5;
			}
			throw new BuildException("HTML5Platform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
		}

		/// <summary>
		/// Get the extension to use for the given binary type
		/// </summary>
		/// <param name="InBinaryType"> The binary type being built</param>
		/// <returns>string    The binary extension (ie 'exe' or 'dll')</returns>
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
					case UEBuildBinaryType.Object:
						return ".o";
					case UEBuildBinaryType.PrecompiledHeader:
						return ".gch";
				}
				return base.GetBinaryExtension(InBinaryType);
			}
			else
			{
				switch (InBinaryType)
				{
					case UEBuildBinaryType.DynamicLinkLibrary:
						return ".js";
					case UEBuildBinaryType.Executable:
						return ".js";
					case UEBuildBinaryType.StaticLibrary:
						return ".bc";
					case UEBuildBinaryType.Object:
						return ".bc";
					case UEBuildBinaryType.PrecompiledHeader:
						return ".gch";
				}

				return base.GetBinaryExtension(InBinaryType);
			}
		}

		/// <summary>
		/// Get the extension to use for debug info for the given binary type
		/// </summary>
		/// <param name="InBinaryType"> The binary type being built</param>
		/// <returns>string    The debug info extension (i.e. 'pdb')</returns>
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

		/// <summary>
		/// Gives the platform a chance to 'override' the configuration settings
		/// that are overridden on calls to RunUBT.
		/// </summary>
		/// <param name="InPlatform">  The UnrealTargetPlatform being built</param>
		/// <param name="InConfiguration"> The UnrealTargetConfiguration being built</param>
		/// <returns>bool    true if debug info should be generated, false if not</returns>
		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			ValidateUEBuildConfiguration();
		}

		/// <summary>
		/// Validate the UEBuildConfiguration for this platform
		/// This is called BEFORE calling UEBuildConfiguration to allow setting
		/// various fields used in that function such as CompileLeanAndMean...
		/// </summary>
		public override void ValidateUEBuildConfiguration()
		{
			UEBuildConfiguration.bCompileLeanAndMeanUE = true;
			UEBuildConfiguration.bCompileAPEX = false;
			UEBuildConfiguration.bCompilePhysX = true;
			UEBuildConfiguration.bRuntimePhysicsCooking = false;
			UEBuildConfiguration.bCompileSimplygon = false;
			UEBuildConfiguration.bCompileICU = true;
			UEBuildConfiguration.bCompileForSize = true;
		}

		/// <summary>
		/// Whether this platform should build a monolithic binary
		/// </summary>
		public override bool ShouldCompileMonolithicBinary(UnrealTargetPlatform InPlatform)
		{
			// This platform currently always compiles monolithic
			return true;
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
			return bCreateDebugInfo;
		}

		/// <summary>
		/// Whether PCH files should be used
		/// </summary>
		/// <param name="InPlatform">  The CPPTargetPlatform being built</param>
		/// <param name="InConfiguration"> The CPPTargetConfiguration being built</param>
		/// <returns>bool    true if PCH files should be used, false if not</returns>
		public override bool ShouldUsePCHFiles(CPPTargetPlatform Platform, CPPTargetConfiguration Configuration)
		{
			return false;
		}

		/// <summary>
		/// Whether the editor should be built for this platform or not
		/// </summary>
		/// <param name="InPlatform"> The UnrealTargetPlatform being built</param>
		/// <param name="InConfiguration">The UnrealTargetConfiguration being built</param>
		/// <returns>bool   true if the editor should be built, false if not</returns>
		public override bool ShouldNotBuildEditor(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return true;
		}

		public override bool BuildRequiresCookedData(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			return true;
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
			if (Target.Platform == UnrealTargetPlatform.HTML5)
			{
				if (ModuleName == "Core")
				{
					Rules.PublicIncludePaths.Add("Runtime/Core/Public/HTML5");
					Rules.PublicDependencyModuleNames.Add("zlib");
				}
				else if (ModuleName == "Engine")
				{
					Rules.PrivateDependencyModuleNames.Add("zlib");
					Rules.PrivateDependencyModuleNames.Add("UElibPNG");
					Rules.PublicDependencyModuleNames.Add("UEOgg");
					Rules.PublicDependencyModuleNames.Add("Vorbis");
				}
			}
			else if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac)
			{
				// allow standalone tools to use targetplatform modules, without needing Engine
				if ((!UEBuildConfiguration.bBuildRequiresCookedData
					&& ModuleName == "Engine"
					&& UEBuildConfiguration.bBuildDeveloperTools)
					|| (UEBuildConfiguration.bForceBuildTargetPlatforms && ModuleName == "TargetPlatform"))
				{
					Rules.PlatformSpecificDynamicallyLoadedModuleNames.Add("HTML5TargetPlatform");
				}
			}
		}


		/// <summary>
		/// Setup the target environment for building
		/// </summary>
		/// <param name="InBuildTarget"> The target being built</param>
		public override void SetUpEnvironment(UEBuildTarget InBuildTarget)
		{
			if (GetActiveArchitecture() == "-win32")
				InBuildTarget.GlobalLinkEnvironment.Config.ExcludedLibraries.Add("LIBCMT");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_HTML5=1");
			if (InBuildTarget.GlobalCompileEnvironment.Config.Target.Architecture == "-win32")
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_HTML5_WIN32=1");
				InBuildTarget.GlobalLinkEnvironment.Config.AdditionalLibraries.Add("delayimp.lib");
			}
			else
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_HTML5_BROWSER=1");
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
					return !BuildConfiguration.bOmitPCDebugInfoInDevelopment;
				case UnrealTargetConfiguration.Debug:
				default:
					// We don't need debug info for Emscripten, and it causes a bunch of issues with linking
					return true;
			};
		}

		/// <summary>
		/// Setup the binaries for this specific platform.
		/// </summary>
		/// <param name="InBuildTarget"> The target being built</param>
		public override void SetupBinaries(UEBuildTarget InBuildTarget)
		{
			// 			InBuildTarget.ExtraModuleNames.Add("ES2RHI");
		}
	}
}
