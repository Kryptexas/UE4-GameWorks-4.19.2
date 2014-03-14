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

		/** Whether we should compile the network profiler or not.*/
		public static bool bCompileNetworkProfiler;

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

		/** Whether we should compile in support for Steam OnlineSubsystem or not. */
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

		/** Sets the configuration back to defaults. */
		public static void Reset()
		{
			//@todo. Allow disabling PhysX/APEX via these values...
			// Currently, WITH_PHYSX is forced to true in Engine.h (as it isn't defined anywhere by the builder)
			bCompilePhysX = true;// Utils.GetEnvironmentVariable("ue.bCompilePhysX", true);
			bCompileAPEX = true;// Utils.GetEnvironmentVariable("ue.bCompileAPEX", true);
			bCompileNetworkProfiler = Utils.GetEnvironmentVariable("ue.bCompileNetworkProfiler", false);
			bBuildDedicatedServer = false;
			bBuildEditor = true;
			bBuildRequiresCookedData = false;
			bBuildWithEditorOnlyData = true;
			bBuildDeveloperTools = true;
			bForceBuildTargetPlatforms = false;
			bForceBuildShaderFormats = false;
            bCompileSimplygon = Utils.GetEnvironmentVariable("ue.bCompileSimplygon", true)
                && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NoRedist") == true
                && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NoRedist/Simplygon") == true
                && Directory.Exists("Developer/SimplygonMeshReduction") == true
                && !(ProjectFileGenerator.bGenerateProjectFiles && ProjectFileGenerator.bGeneratingRocketProjectFiles);
			bCompileLeanAndMeanUE = false;
			bCompileAgainstEngine = true;
			bCompileAgainstCoreUObject = true;
			UEThirdPartyDirectory = "ThirdParty/";
			bCompileRecast = Utils.GetEnvironmentVariable("ue.bCompileRecast", true);
			bForceEnableExceptions = false;
			bWithServerCode = true;
			bCompileSpeedTree = Utils.GetEnvironmentVariable("ue.bCompileSpeedTree", true)
				&& Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NoRedist") == true
				&& Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "NoRedist/SpeedTree") == true
				&& !(ProjectFileGenerator.bGenerateProjectFiles && ProjectFileGenerator.bGeneratingRocketProjectFiles);
			bCompileWithStatsWithoutEngine = false;
			bCompileWithPluginSupport = false;
            bUseLoggingInShipping = false;

			string SteamVersion = "Steamv128";
			bCompileSteamOSS = Utils.GetEnvironmentVariable("ue.bCompileSteamOSS", true)
			   && Directory.Exists(UEBuildConfiguration.UEThirdPartyDirectory + "Steamworks/" + SteamVersion) == true;

			bCompileMcpOSS = Utils.GetEnvironmentVariable("ue.bCompileMcpOSS", true)
			   && Directory.Exists("Runtime/Online/NoRedist/OnlineSubsystemMcp") == true;	
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
				bCompileNetworkProfiler = false;
				bCompileSpeedTree = false;
			}
		}
	}
}
