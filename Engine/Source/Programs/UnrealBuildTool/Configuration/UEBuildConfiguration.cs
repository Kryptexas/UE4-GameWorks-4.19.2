// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;

namespace UnrealBuildTool
{
	public class UEBuildConfiguration
	{
		/** Whether to include PhysX support */
		public static bool bCompilePhysX;

		/** Whether to include PhysX APEX support */
		public static bool bCompileAPEX;

        /** Whether to include ICU unicode/i18n support in core */
        public static bool bCompileICU;

		/** Whether to build a stripped down version of the game specifically for dedicated server. */
		public static bool bBuildDedicatedServer;

		/** Whether to compile the editor or not. Only desktop platforms (Windows or Mac) will use this, other platforms force this to false */
		public static bool bBuildEditor;

		/** Whether to compile code related to building assets. Consoles generally cannot build assets. Desktop platforms generally can. */
		public static bool bBuildRequiresCookedData;

		/** Whether to compile WITH_EDITORONLY_DATA disabled. Only Windows will use this, other platforms force this to false */
		public static bool bBuildWithEditorOnlyData;

		/** Whether to compile the developer tools. */
		public static bool bBuildDeveloperTools;

		/** Whether to force compiling the target platform modules, even if they wouldn't normally be built */
		public static bool bForceBuildTargetPlatforms;

		/** Whether to force compiling shader format modules, even if they wouldn't normally be built. */
		public static bool bForceBuildShaderFormats;

		/** Whether we should compile in support for Simplygon or not. */
		public static bool bCompileSimplygon;

        /** Whether we should compile in support for Steam OnlineSubsystem or not. [RCL] FIXME 2014-Apr-17: bCompileSteamOSS means "bHasSteamworksInstalled" for some code, these meanings need to be untangled */
		public static bool bCompileSteamOSS;

		/** Whether we should compile in support for Mcp OnlineSubsystem or not. */
		public static bool bCompileMcpOSS;

		/** Whether to compile lean and mean version of UE. */
		public static bool bCompileLeanAndMeanUE;

		/** Whether to generate a manifest file that contains the files to add to Perforce */
		public static bool bGenerateManifest;

		/** Whether to add to the existing manifest (if it exists), or start afresh */
		public static bool bMergeManifests;

		/** Whether to 'clean' the given project */
		public static bool bCleanProject;

		/** Whether we are just running the PrepTargetForDeployment step */
		public static bool bPrepForDeployment;

		/** Enabled for all builds that include the engine project.  Disabled only when building standalone apps that only link with Core. */
		public static bool bCompileAgainstEngine;

		/** Enabled for all builds that include the CoreUObject project.  Disabled only when building standalone apps that only link with Core. */
		public static bool bCompileAgainstCoreUObject;

		/** If true, include ADO database support in core */
		public static bool bIncludeADO;

		/** Directory for the third part files/libs */
		public static string UEThirdPartyDirectory;

		/** If true, force header regeneration. Intended for the build machine */
		public static bool bForceHeaderGeneration;

		/** If true, do not build UHT, assume it is already built */
		public static bool bDoNotBuildUHT;

		/** If true, fail if any of the generated header files is out of date. */
		public static bool bFailIfGeneratedCodeChanges;

		/** Whether to compile Recast navmesh generation */
		public static bool bCompileRecast;

		/** Whether to compile SpeedTree support. */
		public static bool bCompileSpeedTree;

		/** Enable exceptions for all modules */
		public static bool bForceEnableExceptions;

		/** Compile server-only code. */
		public static bool bWithServerCode;

		/** Whether to include stats support even without the engine */
		public static bool bCompileWithStatsWithoutEngine;

		/** Whether to include plugin support */
		public static bool bCompileWithPluginSupport;

		/** Whether to turn on logging for test/shipping builds */
		public static bool bUseLoggingInShipping;

		/** True if plugins should be excluded when building target. */
		public static bool bExcludePlugins;

        /** True if we need to package up Android with the OBB in the APK file */
        public static bool bOBBinAPK;

		/** Sets the configuration back to defaults. */
		public static void Reset()
		{
			//@todo. Allow disabling PhysX/APEX via these values...
			// Currently, WITH_PHYSX is forced to true in Engine.h (as it isn't defined anywhere by the builder)
			bCompilePhysX = true;
			bCompileAPEX = true;
			bBuildDedicatedServer = false;
			bBuildEditor = true;
			bBuildRequiresCookedData = false;
			bBuildWithEditorOnlyData = true;
			bBuildDeveloperTools = true;
			bForceBuildTargetPlatforms = false;
			bForceBuildShaderFormats = false;
			bCompileSimplygon = true;
			bCompileLeanAndMeanUE = false;
			bCompileAgainstEngine = true;
			bCompileAgainstCoreUObject = true;
			UEThirdPartyDirectory = "ThirdParty/";
			bCompileRecast = true;
			bForceEnableExceptions = false;
			bWithServerCode = true;
			bCompileSpeedTree = true;
			bCompileWithStatsWithoutEngine = false;
			bCompileWithPluginSupport = false;
            bUseLoggingInShipping = false;
			bCompileSteamOSS = true;
			bCompileMcpOSS = true;
            bOBBinAPK = false;

			XmlConfigLoader.Load(typeof(UEBuildConfiguration));

			// Configuration overrides.
			string SteamVersion = "Steamv129";
			bCompileSteamOSS = bCompileSteamOSS
			   && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "Steamworks/" + SteamVersion) == true;

			bCompileMcpOSS = bCompileMcpOSS
			   && Directory.Exists("Runtime/Online/NotForLicensees/OnlineSubsystemMcp") == true;


			bCompileSimplygon = bCompileSimplygon
				&& Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees") == true
                && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees/Simplygon") == true
				&& Directory.Exists("Developer/SimplygonMeshReduction") == true
				&& !(ProjectFileGenerator.bGenerateProjectFiles && ProjectFileGenerator.bGeneratingRocketProjectFiles);

			bCompileSpeedTree = bCompileSpeedTree
                && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees") == true
                && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NotForLicensees/SpeedTree") == true
				&& !(ProjectFileGenerator.bGenerateProjectFiles && ProjectFileGenerator.bGeneratingRocketProjectFiles);
		}

		/**
		 * Validates the configuration.
		 * 
		 * @warning: the order of validation is important
		 */
		public static void ValidateConfiguration()
		{
			// Lean and mean means no Editor and other frills.
			if (bCompileLeanAndMeanUE)
			{
				bBuildEditor = false;
				bBuildDeveloperTools = false;
				bCompileSimplygon = false;
				bCompileSpeedTree = false;
			}
		}
	}
}
