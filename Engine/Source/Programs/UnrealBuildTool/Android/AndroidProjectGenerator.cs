// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace UnrealBuildTool
{
	/**
	 *	Base class for platform-specific project generators 
	 */
	class AndroidProjectGenerator : UEPlatformProjectGenerator
	{
		static bool CheckedForNsight = false;		// whether we have checked for a recent enough version of Nsight yet
		static bool NsightInstalled = false;		// true if a recent enough version of Nsight is installed
		static int NsightVersionCode = 0;			// version code matching detected Nsight

		/**
		 *	Check to see if a recent enough version of Nsight is installed.
		 */
		bool IsNsightInstalled()
		{
			// cache the results since this gets called a number of times
			if (CheckedForNsight)
			{
				return NsightInstalled;
			}

			CheckedForNsight = true;

			// NOTE: there is now a registry key that we can use instead at:
			//			HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\NVIDIA Corporation\Nsight Tegra\Version

			string ProgramFilesPath = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);

			string PlatformToolsetVersion = VCProjectFileGenerator.ProjectFilePlatformToolsetVersionString;
			if( String.IsNullOrEmpty( PlatformToolsetVersion ) )
			{
				// future maintainer: add toolset version and verify that the rest of the msbuild path, version, and location in ProgramFiles(x86) is still valid
				Log.TraceInformation("Android project generation needs to be updated for this version of Visual Studio.");
				return false;
			}

			// build the path to where the Nsight DLL we'll be checking should sit
			string NsightDllPath = Path.Combine(ProgramFilesPath, @"MSBuild\Microsoft.Cpp\v4.0", PlatformToolsetVersion, @"Platforms\Tegra-Android\Nvidia.Build.CPPTasks.Tegra-Android.Extensibility.dll");

			if (!File.Exists(NsightDllPath))
			{
				return false;
			}

			// grab the version info from the DLL
			FileVersionInfo NsightVersion = FileVersionInfo.GetVersionInfo(NsightDllPath);

			if (NsightVersion.ProductMajorPart > 3)
			{
				// Mark as Nsight 3.1 (project will be updated)
				NsightVersionCode = 10;
				NsightInstalled = true;
			}
			else if (NsightVersion.ProductMajorPart == 3)
			{
				// Nsight 3.0 supported
				NsightVersionCode = 9;
				NsightInstalled = true;

				if (NsightVersion.ProductMinorPart >= 1)
				{
					// Nsight 3.1+ should be valid (will update project if newer)
					NsightVersionCode = 10;
				}
			}
			else if (NsightVersion.ProductMajorPart == 2)
			{
				// Nsight 2.0+ should be valid
				NsightVersionCode = 6;
				NsightInstalled = true;
			}
			else if ((NsightVersion.ProductMajorPart == 1) && (NsightVersion.ProductMinorPart >= 5))
			{
				// Nsight 1.5+ should be valid
				NsightVersionCode = 6;
				NsightInstalled = true;
			}

			if (!NsightInstalled)
			{
				Log.TraceInformation("\nNsight Tegra {0}.{1} found, but Nsight Tegra 1.5 or higher is required for debugging support.", NsightVersion.ProductMajorPart, NsightVersion.ProductMinorPart);
			}

			return NsightInstalled;
		}

		/**
		 *	Register the platform with the UEPlatformProjectGenerator class
		 */
		public override void RegisterPlatformProjectGenerator()
		{
			Log.TraceVerbose("        Registering for {0}", UnrealTargetPlatform.Android.ToString());
			UEPlatformProjectGenerator.RegisterPlatformProjectGenerator(UnrealTargetPlatform.Android, this);
		}

        /**
         *	Whether this build platform has native support for VisualStudio
         *	
         *	@param	InPlatform			The UnrealTargetPlatform being built
         *	@param	InConfiguration		The UnrealTargetConfiguration being built
         *	
         *	@return	bool				true if native VisualStudio support (or custom VSI) is available
         */
        public override bool HasVisualStudioSupport(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
        {
            // Debugging, etc. are dependent on the TADP being installed
			return IsNsightInstalled();
        }
	
		/**
		 *	Return the VisualStudio platform name for this build platform
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	InConfiguration		The UnrealTargetConfiguration being built
		 *	
		 *	@return	string				The name of the platform that VisualStudio recognizes
		 */
		public override string GetVisualStudioPlatformName(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration)
		{
			if (InPlatform == UnrealTargetPlatform.Android)
			{
				return "Tegra-Android";
			}

			return InPlatform.ToString();
		}

		/**
		 * Return any custom property group lines
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	
		 *	@return	string				The custom property import lines for the project file; Empty string if it doesn't require one
		 */
		public override string GetAdditionalVisualStudioPropertyGroups(UnrealTargetPlatform InPlatform)
		{
			if (!IsNsightInstalled())
			{
				return base.GetAdditionalVisualStudioPropertyGroups(InPlatform);
			}

			return 	"	<PropertyGroup Label=\"NsightTegraProject\">" + ProjectFileGenerator.NewLine +
					"		<NsightTegraProjectRevisionNumber>" + NsightVersionCode.ToString() + "</NsightTegraProjectRevisionNumber>" + ProjectFileGenerator.NewLine +
					"	</PropertyGroup>" + ProjectFileGenerator.NewLine;
		}

		/**
		 * Return any custom property group lines
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	
		 *	@return	string				The custom property import lines for the project file; Empty string if it doesn't require one
		 */
		public override string GetVisualStudioPlatformConfigurationType(UnrealTargetPlatform InPlatform)
		{
			if (!IsNsightInstalled())
			{
				return base.GetVisualStudioPlatformConfigurationType(InPlatform);
			}

			return "ExternalBuildSystem";
		}

		/**
		 *	Return the platform toolset string to write into the project configuration
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	InConfiguration		The UnrealTargetConfiguration being built
		 *	
		 *	@return	string				The custom configuration section for the project file; Empty string if it doesn't require one
		 */
		public override string GetVisualStudioPlatformToolsetString(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration, VCProjectFile InProjectFile)
		{
			if (!IsNsightInstalled())
			{
				return "\t\t<PlatformToolset>" + VCProjectFileGenerator.ProjectFilePlatformToolsetVersionString + "</PlatformToolset>" + ProjectFileGenerator.NewLine;
			}

			return "\t\t<PlatformToolset>" + VCProjectFileGenerator.ProjectFilePlatformToolsetVersionString + "</PlatformToolset>" + ProjectFileGenerator.NewLine
				+ "\t\t<AndroidNativeAPI>UseTarget</AndroidNativeAPI>" + ProjectFileGenerator.NewLine;
		}

		/**
		 * Return any custom paths for VisualStudio this platform requires
		 * This include ReferencePath, LibraryPath, LibraryWPath, IncludePath and ExecutablePath.
		 *	
		 *	@param	InPlatform			The UnrealTargetPlatform being built
		 *	@param	TargetType			The type of target (game or program)
		 *	
		 *	@return	string				The custom path lines for the project file; Empty string if it doesn't require one
		 */
		public override string GetVisualStudioPathsEntries(UnrealTargetPlatform InPlatform, UnrealTargetConfiguration InConfiguration, TargetRules.TargetType TargetType, string TargetRulesPath, string ProjectFilePath, string NMakeOutputPath)
		{
			if (!IsNsightInstalled())
			{
				return base.GetVisualStudioPathsEntries(InPlatform, InConfiguration, TargetType, TargetRulesPath, ProjectFilePath, NMakeOutputPath);
			}

			// NOTE: We are intentionally overriding defaults for these paths with empty strings.  We never want Visual Studio's
			//       defaults for these fields to be propagated, since they are version-sensitive paths that may not reflect
			//       the environment that UBT is building in.  We'll set these environment variables ourselves!
			// NOTE: We don't touch 'ExecutablePath' because that would result in Visual Studio clobbering the system "Path"
			//       environment variable

			//@todo android: clean up debug path generation
			string GameName = Path.GetFileNameWithoutExtension(TargetRulesPath);
			GameName = Path.GetFileNameWithoutExtension(GameName);


			// intermediate path for Engine or Game's intermediate
			string IntermediateDirectoryPath;
			IntermediateDirectoryPath = Path.GetDirectoryName(NMakeOutputPath) + "/../../Intermediate/Android/APK";

			// string for <OverrideAPKPath>
			string APKPath = Path.Combine(
				Path.GetDirectoryName(NMakeOutputPath),
				Path.GetFileNameWithoutExtension(NMakeOutputPath) + "-armv7-es2.apk");

			// string for <BuildXmlPath> and <AndroidManifestPath>
			string BuildXmlPath = IntermediateDirectoryPath;
			string AndroidManifestPath = Path.Combine(IntermediateDirectoryPath, "AndroidManifest.xml");

			// string for <AdditionalLibraryDirectories>
			string AdditionalLibDirs = "";
			AdditionalLibDirs += IntermediateDirectoryPath + @"\obj\local\armeabi-v7a";
			AdditionalLibDirs += ";" + IntermediateDirectoryPath + @"\obj\local\x86";
			AdditionalLibDirs += @";$(AdditionalLibraryDirectories)";

			string PathsLines = 
				"		<IncludePath />" + ProjectFileGenerator.NewLine +
				"		<ReferencePath />" + ProjectFileGenerator.NewLine +
				"		<LibraryPath />" + ProjectFileGenerator.NewLine +
				"		<LibraryWPath />" + ProjectFileGenerator.NewLine +
				"		<SourcePath />" + ProjectFileGenerator.NewLine +
				"		<ExcludePath />" + ProjectFileGenerator.NewLine +
				"		<AndroidAttach>False</AndroidAttach>" + ProjectFileGenerator.NewLine +
				"		<DebuggerFlavor>AndroidDebugger</DebuggerFlavor>" + ProjectFileGenerator.NewLine +
				"		<OverrideAPKPath>" + APKPath + "</OverrideAPKPath>" + ProjectFileGenerator.NewLine +
				"		<AdditionalLibraryDirectories>" + AdditionalLibDirs + "</AdditionalLibraryDirectories>" + ProjectFileGenerator.NewLine +
				"		<BuildXmlPath>" + BuildXmlPath + "</BuildXmlPath>" + ProjectFileGenerator.NewLine +
				"		<AndroidManifestPath>" + AndroidManifestPath + "</AndroidManifestPath>" + ProjectFileGenerator.NewLine;

			return PathsLines;
		}
	}
}
