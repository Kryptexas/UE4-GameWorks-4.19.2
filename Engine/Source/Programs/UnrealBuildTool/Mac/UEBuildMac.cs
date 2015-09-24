// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	class MacPlatform : UEBuildPlatform
	{
		public override bool CanUseXGE()
		{
			return false;
		}

		public override bool CanUseDistcc()
		{
			return true;
		}

		protected override SDKStatus HasRequiredManualSDKInternal()
		{
			return SDKStatus.Valid;
		}

		/// <summary>
		/// Register the platform with the UEBuildPlatform class
		/// </summary>
		protected override void RegisterBuildPlatformInternal()
		{
			// Register this build platform for Mac
			Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.Mac.ToString());
			UEBuildPlatform.RegisterBuildPlatform(UnrealTargetPlatform.Mac, this);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Mac, UnrealPlatformGroup.Unix);
			UEBuildPlatform.RegisterPlatformWithGroup(UnrealTargetPlatform.Mac, UnrealPlatformGroup.Apple);
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
				case UnrealTargetPlatform.Mac:
					return CPPTargetPlatform.Mac;
			}
			throw new BuildException("MacPlatform::GetCPPTargetPlatform: Invalid request for {0}", InUnrealTargetPlatform.ToString());
		}

		/// <summary>
		/// Get the extension to use for the given binary type
		/// </summary>
		/// <param name="InBinaryType"> The binrary type being built</param>
		/// <returns>string    The binary extenstion (ie 'exe' or 'dll')</returns>
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
			return BuildConfiguration.bGeneratedSYMFile || BuildConfiguration.bUsePDBFiles ? ".dSYM" : "";
		}

		public override void ModifyModuleRules(string ModuleName, ModuleRules Rules, TargetInfo Target)
		{
			if (Target.Platform == UnrealTargetPlatform.Mac)
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
						Rules.DynamicallyLoadedModuleNames.Add("MacTargetPlatform");
						Rules.DynamicallyLoadedModuleNames.Add("MacNoEditorTargetPlatform");
						Rules.DynamicallyLoadedModuleNames.Add("MacClientTargetPlatform");
						Rules.DynamicallyLoadedModuleNames.Add("MacServerTargetPlatform");
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
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_MAC=1");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("PLATFORM_APPLE=1");

			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_TTS=0");
			InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_SPEECH_RECOGNITION=0");
			if (!InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Contains("WITH_DATABASE_SUPPORT=0") && !InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Contains("WITH_DATABASE_SUPPORT=1"))
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("WITH_DATABASE_SUPPORT=0");
			}
			// Needs OS X 10.11 for Metal
			if (MacToolChain.MacOSSDKVersionFloat >= 10.11f && UEBuildConfiguration.bCompileAgainstEngine)
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("HAS_METAL=1");
				InBuildTarget.ExtraModuleNames.Add("MetalRHI");
			}
			else
			{
				InBuildTarget.GlobalCompileEnvironment.Config.Definitions.Add("HAS_METAL=0");
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
			return true;
		}

		public override void ResetBuildConfiguration(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			UEBuildConfiguration.bCompileSimplygon = false;
			UEBuildConfiguration.bCompileICU = true;
		}

		public override void ValidateBuildConfiguration(CPPTargetConfiguration Configuration, CPPTargetPlatform Platform, bool bCreateDebugInfo)
		{
			if (BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Mac)
			{
				// @todo: Temporarily disable precompiled header files when building remotely due to errors
				BuildConfiguration.bUsePCHFiles = false;
			}
			BuildConfiguration.bCheckExternalHeadersForModification = BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Mac;
			BuildConfiguration.bCheckSystemHeadersForModification = BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Mac;
			BuildConfiguration.ProcessorCountMultiplier = MacToolChain.GetAdjustedProcessorCountMultiplier();
			BuildConfiguration.bUseSharedPCHs = false;

			BuildConfiguration.bUsePDBFiles = bCreateDebugInfo && Configuration != CPPTargetConfiguration.Debug && Platform == CPPTargetPlatform.Mac;

			// we always deploy - the build machines need to be able to copy the files back, which needs the full bundle
			BuildConfiguration.bDeployAfterCompile = true;
		}

		public override void ValidateUEBuildConfiguration()
		{
			if (ProjectFileGenerator.bGenerateProjectFiles && !ProjectFileGenerator.bGeneratingRocketProjectFiles)
			{
				// When generating non-Rocket project files we need intellisense generator to include info from all modules, including editor-only third party libs
				UEBuildConfiguration.bCompileLeanAndMeanUE = false;
			}
		}

		/// <summary>
		/// Whether the platform requires the extra UnityCPPWriter
		/// This is used to add an extra file for UBT to get the #include dependencies from
		/// </summary>
		/// <returns>bool true if it is required, false if not</returns>
		public override bool RequiresExtraUnityCPPWriter()
		{
			return true;
		}

		/// <summary>
		/// Return whether we wish to have this platform's binaries in our builds
		/// </summary>
		public override bool IsBuildRequired()
		{
			return false;
		}

		/// <summary>
		/// Return whether we wish to have this platform's binaries in our CIS tests
		/// </summary>
		public override bool IsCISRequired()
		{
			return false;
		}
	}
}
