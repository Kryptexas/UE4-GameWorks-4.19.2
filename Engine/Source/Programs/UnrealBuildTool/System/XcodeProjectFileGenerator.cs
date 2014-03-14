// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace UnrealBuildTool
{
	/// <summary>
	/// Represents a folder within the master project (e.g. Visual Studio solution)
	/// </summary>
	class XcodeProjectFolder : MasterProjectFolder
	{
		/// <summary>
		/// Constructor
		/// </summary>
		public XcodeProjectFolder( ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName )
			: base(InitOwnerProjectFileGenerator, InitFolderName )
		{
			// Generate a unique GUID for this folder
			// NOTE: When saving generated project files, we ignore differences in GUIDs if every other part of the file
			//       matches identically with the pre-existing file
			FolderGUID = Guid.NewGuid();
		}


		/// GUID for this folder
		public Guid FolderGUID
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// Represents a build target
	/// </summary>
	public class XcodeProjectTarget
	{
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InName">Name of the target that may be built.</param>
		/// <param name="InType">Specifies the type of the targtet to generate.</param>
		/// <param name="InProductName">Specifies the name of the executable if it differs from the InName</param>
		/// <param name="InTargetPlatform">Name of the target that may be built.</param>
		/// <param name="InDependencies">Name of the target that may be built.</param>
		/// <param name="bHasPlist">Name of the target that may be built.</param>
		public XcodeProjectTarget(string InDisplayName, string InTargetName, string InType, string InProductName = "", UnrealTargetPlatform InTargetPlatform = UnrealTargetPlatform.Mac, List<XcodeTargetDependency> InDependencies = null, bool bHasPlist = false, List<XcodeFrameworkRef> InFrameworks = null)
		{
			DisplayName = InDisplayName;
			TargetName = InTargetName;
			Type = InType;
			ProductName = InProductName;
			TargetPlatform = InTargetPlatform;
			Guid = XcodeProjectFileGenerator.MakeXcodeGuid();
			BuildConfigGuild = XcodeProjectFileGenerator.MakeXcodeGuid();
			SourcesPhaseGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			ResourcesPhaseGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			FrameworksPhaseGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			ShellScriptPhaseGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			ProductGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			DebugConfigGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			DevelopmentConfigGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			ShippingConfigGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			TestConfigGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			DebugGameConfigGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			Dependencies = InDependencies == null ? new List<XcodeTargetDependency>() : InDependencies;
            FrameworkRefs = InFrameworks == null ? new List<XcodeFrameworkRef>() : InFrameworks;
			// Meant to prevent adding plists that don't belong to the target, like in the case of Mac builds.
			if (bHasPlist)
			{
				PlistGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			}
		}

		public string DisplayName;				//Name displayed in Xcode's UI.
		public string TargetName;				//Actual name of the target.
		public string Type;						// "Native", "Legacy" or "Project"
		public UnrealTargetPlatform TargetPlatform;	//Mac, IOS
		public string ProductName;

		public string Guid;
		public string BuildConfigGuild;
		public string SourcesPhaseGuid;
		public string ResourcesPhaseGuid;
		public string FrameworksPhaseGuid;
		public string ShellScriptPhaseGuid;
		public string ProductGuid;
		public string DebugConfigGuid;
		public string DevelopmentConfigGuid;
		public string ShippingConfigGuid;
		public string TestConfigGuid;
		public string DebugGameConfigGuid;
		public string PlistGuid;
		public List<XcodeTargetDependency> Dependencies; // Target dependencies used for chaining Xcode target builds.
        public List<XcodeFrameworkRef> FrameworkRefs;   // Frameworks included in this target.

		public bool HasUI; // true if generates an .app instead of a console exe

		public override string ToString ()
		{
			return string.Format ("[XcodeProjectTarget: {0}, {1}]", DisplayName, Type);
		}
	}

	/// <summary>
	/// Represents a target section.
	/// </summary>
	public class XcodeTargetDependency
	{
		public XcodeTargetDependency(string InLegacyTargetName, string InLegacyTargetGuid, string InContainerItemProxyGuid)
		{
			Guid = XcodeProjectFileGenerator.MakeXcodeGuid();
			LegacyTargetName = InLegacyTargetName;
			LegacyTargetGuid = InLegacyTargetGuid;
			ContainerItemProxyGuid = InContainerItemProxyGuid;
		}
		public string Guid;
		public string LegacyTargetName;
		public string LegacyTargetGuid;
		public string ContainerItemProxyGuid;
	}

	/// <summary>
	/// Represents an item proxy section.
	/// </summary>
	public class XcodeContainerItemProxy
	{
		public XcodeContainerItemProxy(string InProjectGuid, string InLegacyTargetGuid, string InTargetName)
		{
			Guid = XcodeProjectFileGenerator.MakeXcodeGuid();
			ProjectGuid = InProjectGuid;
			LegacyTargetGuid = InLegacyTargetGuid;
			TargetName = InTargetName;
		}
		public string Guid;
		public string ProjectGuid;
		public string LegacyTargetGuid;
		public string TargetName;
	}

    /// <summary>
    /// Represents a Framework.
    /// </summary>
    public class XcodeFramework
    {
        public XcodeFramework(string InName, string InPath, string InSourceTree)
        {
            Guid = XcodeProjectFileGenerator.MakeXcodeGuid();
            Name = InName;
            Path = InPath;
            SourceTree = InSourceTree;
        }
        public string Name;
        public string Path;
        public string SourceTree;
        public string Guid;
    }

    /// <summary>
    /// Represents a reference to a Framework.
    /// </summary>
    public class XcodeFrameworkRef
    {
        public XcodeFrameworkRef(XcodeFramework InFramework)
        {
            Guid = XcodeProjectFileGenerator.MakeXcodeGuid();
            Framework = InFramework;
        }
        public XcodeFramework Framework;
        public string Guid;

    }

	class XcodeProjectFileGenerator : ProjectFileGenerator
	{
		/// File extension for project files we'll be generating (e.g. ".vcxproj")
		override public string ProjectFileExtension
		{
			get
			{
				return ".xcodeproj";
			}
		}

		public override void CleanProjectFiles(string InMasterProjectRelativePath, string InMasterProjectName, string InIntermediateProjectFilesPath)
		{
			//@todo Mac. Implement this function...
		}

		/// <summary>
		/// Allocates a generator-specific project file object
		/// </summary>
		/// <param name="InitFilePath">Path to the project file</param>
		/// <returns>The newly allocated project file object</returns>
		protected override ProjectFile AllocateProjectFile( string InitFilePath )
		{
			return new XcodeProjectFile( InitFilePath );
		}

		/// ProjectFileGenerator interface
		public override MasterProjectFolder AllocateMasterProjectFolder( ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName )
		{
			return new XcodeProjectFolder( InitOwnerProjectFileGenerator, InitFolderName );
		}

		protected override bool WriteMasterProjectFile( ProjectFile UBTProject )
		{
			bool bSuccess = true;
			return bSuccess;
		}

		/// <summary>
		/// Appends the groups section.
		/// </summary>
		/// <param name="Contents">StringBuilder object to append groups string to</param>
		/// <param name="Groups">Dictionary of all project groups</param>
		/// <param name="UE4XcodeHelperTarget">Dummy target required for syntax highlighting etc. work</param>
        /// <param name="Frameworks">Frameworks referenced</param>
		private void AppendGroups(ref StringBuilder Contents, ref Dictionary<string, XcodeFileGroup> Groups, List<XcodeProjectTarget> Targets, XcodeProjectTarget UE4XcodeHelperTarget, List<XcodeFramework> Frameworks)
		{
			Contents.Append("/* Begin PBXGroup section */" + ProjectFileGenerator.NewLine);

			// Append main group
			MainGroupGuid = MakeXcodeGuid();
			Contents.Append(string.Format("\t\t{0} = {{{1}", MainGroupGuid, ProjectFileGenerator.NewLine));
			Contents.Append("\t\t\tisa = PBXGroup;" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t\tchildren = (" + ProjectFileGenerator.NewLine);

			foreach (XcodeFileGroup Group in Groups.Values)
			{
				Contents.Append(string.Format("\t\t\t\t{0} /* {1} */,{2}", Group.GroupGuid, Group.GroupName, ProjectFileGenerator.NewLine));
			}

			foreach (XcodeProjectTarget Target in Targets)
			{
				if (!string.IsNullOrEmpty(Target.PlistGuid) && File.Exists(Path.Combine(RootRelativePath, Target.TargetName + "/Build/IOS/" + Target.TargetName + "-Info.plist")))
				{
					Contents.Append("\t\t\t\t" + Target.PlistGuid + " /* " + Target.TargetName + "-Info.plist */," + ProjectFileGenerator.NewLine);
				}
			}

			ProductRefGroupGuid = MakeXcodeGuid();
            FrameworkGroupGuid = MakeXcodeGuid();
			Contents.Append(string.Format("\t\t\t\t{0} /* Products */,{1}", ProductRefGroupGuid, ProjectFileGenerator.NewLine));
            Contents.Append(string.Format("\t\t\t\t{0} /* Frameworks */,{1}", FrameworkGroupGuid, ProjectFileGenerator.NewLine));

			Contents.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t\tsourceTree = \"<group>\";" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t};" + ProjectFileGenerator.NewLine);

			// Add products group
			Contents.Append(string.Format("\t\t{0} = {{{1}", ProductRefGroupGuid, ProjectFileGenerator.NewLine));
			Contents.Append("\t\t\tisa = PBXGroup;" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t\tchildren = (" + ProjectFileGenerator.NewLine);
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				Contents.Append("\t\t\t\t" + UE4XcodeHelperTarget.ProductGuid + " /* " + UE4XcodeHelperTarget.ProductName + " */," + ProjectFileGenerator.NewLine);
			}
			// Required for iOS. This removes some "Target is not Built to Run on this Device" errors.
			foreach (XcodeProjectTarget Target in Targets)
			{
				if (Target.Type == "Project" || Target == UE4XcodeHelperTarget || Target.ProductName == "")
				{
					continue;
				}
				Contents.Append("\t\t\t\t" + Target.ProductGuid + " /* " + Target.ProductName + " */," + ProjectFileGenerator.NewLine);
			}
			Contents.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t\tname = Products;" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t\tsourceTree = \"<group>\";" + ProjectFileGenerator.NewLine);
			Contents.Append("\t\t};" + ProjectFileGenerator.NewLine);

            // Add Frameworks group
            Contents.Append(string.Format("\t\t{0} = {{{1}", FrameworkGroupGuid, ProjectFileGenerator.NewLine));
            Contents.Append("\t\t\tisa = PBXGroup;" + ProjectFileGenerator.NewLine);
            Contents.Append("\t\t\tchildren = (" + ProjectFileGenerator.NewLine);
            foreach (XcodeFramework Framework in Frameworks)
            {
                Contents.Append("\t\t\t\t" + Framework.Guid + " /* " + Framework.Name + " */," + ProjectFileGenerator.NewLine);
            }
            Contents.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
            Contents.Append("\t\t\tname = Frameworks;" + ProjectFileGenerator.NewLine);
            Contents.Append("\t\t\tsourceTree = \"<group>\";" + ProjectFileGenerator.NewLine);
            Contents.Append("\t\t};" + ProjectFileGenerator.NewLine);

			foreach (XcodeFileGroup Group in Groups.Values)
			{
				Group.Append(ref Contents);
			}

			Contents.Append("/* End PBXGroup section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		/// <summary>
		/// Appends a target to targets section.
		/// </summary>
		/// <param name="Contents">StringBuilder object to append target string to</param>
		/// <param name="Target">Target to append</param>
		private void AppendTarget(ref StringBuilder Contents, XcodeProjectTarget Target)
		{
            string XcodeTargetType = Target.Type == "XCTest" ? "Native" : Target.Type;
			Contents.Append(
				"\t\t" + Target.Guid + " /* " + Target.DisplayName + " */ = {" + ProjectFileGenerator.NewLine +
				"\t\t\tisa = PBX" + XcodeTargetType + "Target;" + ProjectFileGenerator.NewLine);

			if (Target.Type == "Legacy")
			{
				string UProjectPath = "";
				if (UnrealBuildTool.HasUProjectFile() && Target.TargetName.StartsWith(Path.GetFileNameWithoutExtension(UnrealBuildTool.GetUProjectFile())))
				{
					if (MasterProjectRelativePath == UnrealBuildTool.GetUProjectPath())
					{
						UProjectPath = " " + "\\\"$(PROJECT_DIR)/" + Path.GetFileName(UnrealBuildTool.GetUProjectFile()) + "\\\"";
					}
					else
					{
					UProjectPath = " " + "\\\"" + UnrealBuildTool.GetUProjectFile() + "\\\"";
				}
				}
				// Xcode provides $ACTION argument for determining if we are building or cleaning a project
				Contents.Append("\t\t\tbuildArgumentsString = \"$(ACTION) " + Target.TargetName + " $(PLATFORM_NAME) $(CONFIGURATION)" + UProjectPath + "\";" + ProjectFileGenerator.NewLine);
			}

			Contents.Append("\t\t\tbuildConfigurationList = " + Target.BuildConfigGuild + " /* Build configuration list for PBX" + XcodeTargetType + "Target \"" + Target.DisplayName + "\" */;" + ProjectFileGenerator.NewLine);

			Contents.Append("\t\t\tbuildPhases = (" + ProjectFileGenerator.NewLine);

			if (Target.Type == "Native" || Target.Type == "XCTest")
			{
				Contents.Append("\t\t\t\t" + Target.SourcesPhaseGuid + " /* Sources */," + ProjectFileGenerator.NewLine);
				//Contents.Append("\t\t\t\t" + Target.ResourcesPhaseGuid + " /* Resources */," + ProjectFileGenerator.NewLine);
				Contents.Append("\t\t\t\t" + Target.FrameworksPhaseGuid + " /* Frameworks */," + ProjectFileGenerator.NewLine);
				Contents.Append("\t\t\t\t" + Target.ShellScriptPhaseGuid + " /* ShellScript */," + ProjectFileGenerator.NewLine);
			}

			Contents.Append("\t\t\t);" + ProjectFileGenerator.NewLine);

			if (Target.Type == "Legacy")
			{
				string UE4Dir = Path.GetFullPath(Directory.GetCurrentDirectory() + "../../..");
				if (bGeneratingRocketProjectFiles)
				{
					Contents.Append ("\t\t\tbuildToolPath = \"" + UE4Dir + "/Engine/Build/BatchFiles/Mac/RocketBuild.sh\";" + ProjectFileGenerator.NewLine);
				}
				else
				{
					Contents.Append ("\t\t\tbuildToolPath = \"" + UE4Dir + "/Engine/Build/BatchFiles/Mac/Build.sh\";" + ProjectFileGenerator.NewLine);
				}
				Contents.Append("\t\t\tbuildWorkingDirectory = \"" + UE4Dir + "\";" + ProjectFileGenerator.NewLine);
			}

			// This binds the "Run" targets to the "Build" targets.
			Contents.Append("\t\t\tdependencies = (" + ProjectFileGenerator.NewLine);
			foreach (XcodeTargetDependency Dependency in Target.Dependencies)
			{
				Contents.Append("\t\t\t\t" + Dependency.Guid + " /* PBXTargetDependency */" + ProjectFileGenerator.NewLine);
			}
			Contents.Append(
				"\t\t\t);" + ProjectFileGenerator.NewLine +
				"\t\t\tname = \"" + Target.DisplayName + "\";" + ProjectFileGenerator.NewLine);

			if (Target.Type == "Legacy")
			{
				Contents.Append("\t\t\tpassBuildSettingsInEnvironment = 1;" + ProjectFileGenerator.NewLine);
			}

			Contents.Append("\t\t\tproductName = \"" + Target.DisplayName + "\";" + ProjectFileGenerator.NewLine);

			if (Target.Type == "Native")
			{
				if (Target.DisplayName.Contains("(Build)"))
				{
					Contents.Append(
						"\t\t\tproductReference = " + Target.ProductGuid + " /* " + Target.ProductName + " */;" + ProjectFileGenerator.NewLine +
						"\t\t\tproductType = \"com.apple.product-type.library.static\";" + ProjectFileGenerator.NewLine);
				}
				else
				{
					Contents.Append(
						"\t\t\tproductReference = " + Target.ProductGuid + " /* " + Target.ProductName + " */;" + ProjectFileGenerator.NewLine +
						"\t\t\tproductType = \"com.apple.product-type.application\";" + ProjectFileGenerator.NewLine);
				}
			}
			if (Target.Type == "XCTest")
			{
					Contents.Append(
						"\t\t\tproductReference = " + Target.ProductGuid + " /* " + Target.ProductName + " */;" + ProjectFileGenerator.NewLine +
						"\t\t\tproductType = \"com.apple.product-type.bundle.unit-test\";" + ProjectFileGenerator.NewLine);
			}
			Contents.Append("\t\t};" + ProjectFileGenerator.NewLine);
		}

		/// <summary>
		/// Appends a build configuration section for specific target.
		/// </summary>
		/// <param name="Contents">StringBuilder object to append build configuration string to</param>
		/// <param name="Target">Target for which we generate the build configuration</param>
		/// <param name="HeaderSearchPaths">Optional string with header search paths section</param>
		/// <param name="PreprocessorDefinitions">Optional string with preprocessor definitions section</param>
		private void AppendBuildConfig(ref StringBuilder Contents, XcodeProjectTarget Target, string HeaderSearchPaths = "", string PreprocessorDefinitions = "")
		{
			List<string> GameFolders = UEBuildTarget.DiscoverAllGameFolders();

			bool IsAGame = false;
			string GamePath = null;
			bool bIsUE4Game = Target.TargetName.Equals("UE4Game", StringComparison.InvariantCultureIgnoreCase);
			string EngineRelative = "";

			foreach (string GameFolder in GameFolders )
			{
				if (GameFolder.EndsWith(Target.TargetName))
				{
					IsAGame = true;
					if (Target.TargetPlatform == UnrealTargetPlatform.IOS)
					{
						UEToolChain Toolchain = UEToolChain.GetPlatformToolChain(CPPTargetPlatform.IOS);
						GamePath = Toolchain.ConvertPath(Path.GetFullPath(GameFolder));
					}
					break;
				}
			}
			if (bGeneratingGameProjectFiles || bGeneratingRocketProjectFiles)
			{
				EngineRelative = Path.GetFullPath(EngineRelativePath + "/../");
			}

			if (Target.Type == "Project")
			{
				if (!bGeneratingRocketProjectFiles)
				{
					Contents.Append(
						"\t\t" + Target.DebugConfigGuid + " /* Debug */ = {" + ProjectFileGenerator.NewLine +
						"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
						"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
						PreprocessorDefinitions +
						HeaderSearchPaths +
						"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"c++0x\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
						//This should only be needed for packaging for iOS. If this changes it should be moved to the iOS exclusive sections.
						"\t\t\t\tCONFIGURATION_BUILD_DIR = \"Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
						"\t\t\t};" + ProjectFileGenerator.NewLine +
						"\t\t\tname = Debug;" + ProjectFileGenerator.NewLine +
						"\t\t};" + ProjectFileGenerator.NewLine +

						"\t\t" + Target.TestConfigGuid + " /* Test */ = {" + ProjectFileGenerator.NewLine +
						"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
						"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
						PreprocessorDefinitions +
						HeaderSearchPaths +
						"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"c++0x\";" + ProjectFileGenerator.NewLine +
						//This should only be needed for packaging for iOS. If this changes it should be moved to the iOS exclusive sections.
						"\t\t\t\tCONFIGURATION_BUILD_DIR = \"Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
						"\t\t\t};" + ProjectFileGenerator.NewLine +
						"\t\t\tname = Test;" + ProjectFileGenerator.NewLine +
						"\t\t};" + ProjectFileGenerator.NewLine);
				}
				else
				{
					Contents.Append(
						"\t\t" + Target.DebugGameConfigGuid + " /* DebugGame */ = {" + ProjectFileGenerator.NewLine +
						"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
						"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
						PreprocessorDefinitions +
						HeaderSearchPaths +
						"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"c++0x\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
						//This should only be needed for packaging for iOS. If this changes it should be moved to the iOS exclusive sections.
						"\t\t\t\tCONFIGURATION_BUILD_DIR = \"Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
						"\t\t\t};" + ProjectFileGenerator.NewLine +
						"\t\t\tname = DebugGame;" + ProjectFileGenerator.NewLine +
						"\t\t};" + ProjectFileGenerator.NewLine);

				}
				Contents.Append(
					"\t\t" + Target.DevelopmentConfigGuid + " /* Development */ = {" + ProjectFileGenerator.NewLine +
					"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
					"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
					PreprocessorDefinitions +
					HeaderSearchPaths +
					"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
					"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"c++0x\";" + ProjectFileGenerator.NewLine +
									//This should only be needed for packaging for iOS. If this changes it should be moved to the iOS exclusive sections.
					"\t\t\t\tCONFIGURATION_BUILD_DIR = \"Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
					"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
					"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
					"\t\t\t};" + ProjectFileGenerator.NewLine +
					"\t\t\tname = Development;" + ProjectFileGenerator.NewLine +
					"\t\t};" + ProjectFileGenerator.NewLine +

					"\t\t" + Target.ShippingConfigGuid + " /* Shipping */ = {" + ProjectFileGenerator.NewLine +
					"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
					"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
					PreprocessorDefinitions +
					HeaderSearchPaths +
					"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
					"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"c++0x\";" + ProjectFileGenerator.NewLine +
									//This should only be needed for packaging for iOS. If this changes it should be moved to the iOS exclusive sections.
					"\t\t\t\tCONFIGURATION_BUILD_DIR = \"Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
					"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
					"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
					"\t\t\t};" + ProjectFileGenerator.NewLine +
					"\t\t\tname = Shipping;" + ProjectFileGenerator.NewLine +
					"\t\t};" + ProjectFileGenerator.NewLine);
			}
			else
			{
				if (Target.TargetPlatform == UnrealTargetPlatform.Mac)
				{
					if (!bGeneratingRocketProjectFiles)
					{
						// Non-project targets inherit build settings from the project
						Contents.Append(
							"\t\t" + Target.DebugConfigGuid + " /* Debug */ = {" + ProjectFileGenerator.NewLine +
							"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
							"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
							"\t\t\t\tCOMBINE_HIDPI_IMAGES = YES;" + ProjectFileGenerator.NewLine +
							"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
							"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
							"\t\t\t};" + ProjectFileGenerator.NewLine +
							"\t\t\tname = Debug;" + ProjectFileGenerator.NewLine +
							"\t\t};" + ProjectFileGenerator.NewLine +

							"\t\t" + Target.TestConfigGuid + " /* Test */ = {" + ProjectFileGenerator.NewLine +
							"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
							"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
							"\t\t\t\tCOMBINE_HIDPI_IMAGES = YES;" + ProjectFileGenerator.NewLine +
							"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
							"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
							"\t\t\t};" + ProjectFileGenerator.NewLine +
							"\t\t\tname = Test;" + ProjectFileGenerator.NewLine +
							"\t\t};" + ProjectFileGenerator.NewLine);
					}
					else
					{
						// Non-project targets inherit build settings from the project
						Contents.Append(
							"\t\t" + Target.DebugGameConfigGuid + " /* DebugGame */ = {" + ProjectFileGenerator.NewLine +
							"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
							"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
							"\t\t\t\tCOMBINE_HIDPI_IMAGES = YES;" + ProjectFileGenerator.NewLine +
							"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
							"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
							"\t\t\t};" + ProjectFileGenerator.NewLine +
							"\t\t\tname = DebugGame;" + ProjectFileGenerator.NewLine +
							"\t\t};" + ProjectFileGenerator.NewLine);
					}
					// Non-project targets inherit build settings from the project
					Contents.Append(
						"\t\t" + Target.DevelopmentConfigGuid + " /* Development */ = {" + ProjectFileGenerator.NewLine +
						"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
						"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
						"\t\t\t\tCOMBINE_HIDPI_IMAGES = YES;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
						"\t\t\t};" + ProjectFileGenerator.NewLine +
						"\t\t\tname = Development;" + ProjectFileGenerator.NewLine +
						"\t\t};" + ProjectFileGenerator.NewLine +

						"\t\t" + Target.ShippingConfigGuid + " /* Shipping */ = {" + ProjectFileGenerator.NewLine +
						"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
						"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
						"\t\t\t\tCOMBINE_HIDPI_IMAGES = YES;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
						"\t\t\t\tMACOSX_DEPLOYMENT_TARGET = 10.8;" + ProjectFileGenerator.NewLine +
						"\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine +
						"\t\t\t};" + ProjectFileGenerator.NewLine +
						"\t\t\tname = Shipping;" + ProjectFileGenerator.NewLine +
						"\t\t};" + ProjectFileGenerator.NewLine);
				}
				else
				{
					// IOS build target
					if (Target.Type == "Legacy")
					{
						if (!bGeneratingRocketProjectFiles)
						{
							Contents.Append(
								"\t\t" + Target.DebugConfigGuid + " /* Debug */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"$(TARGET_NAME)-simulator\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = Debug;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine +

								"\t\t" + Target.ShippingConfigGuid + " /* Shipping */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"$(TARGET_NAME)-simulator\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = Shipping;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine);
						}
						else
						{
							Contents.Append(
								"\t\t" + Target.DebugGameConfigGuid + " /* DebugGame */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"$(TARGET_NAME)-simulator\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = DebugGame;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine);
						}
						Contents.Append(
							"\t\t" + Target.DevelopmentConfigGuid + " /* Development */ = {" + ProjectFileGenerator.NewLine +
							"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
							"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
							"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"$(TARGET_NAME)-simulator\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
							"\t\t\t};" + ProjectFileGenerator.NewLine +
							"\t\t\tname = Development;" + ProjectFileGenerator.NewLine +
							"\t\t};" + ProjectFileGenerator.NewLine +

							"\t\t" + Target.ShippingConfigGuid + " /* Shipping */ = {" + ProjectFileGenerator.NewLine +
							"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
							"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
							"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"$(TARGET_NAME)-simulator\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
							"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
							"\t\t\t};" + ProjectFileGenerator.NewLine +
							"\t\t\tname = Shipping;" + ProjectFileGenerator.NewLine +
							"\t\t};" + ProjectFileGenerator.NewLine);
					}
					// IOS run target (Native Target)
					else
					{
						if ( Target.Type != "XCTest" )
						{
							if (!bGeneratingRocketProjectFiles)
							{
								Contents.Append(
									"\t\t" + Target.DebugConfigGuid + " /* Debug */ = {" + ProjectFileGenerator.NewLine +
									"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
									"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"" + Target.TargetName + "-simulator\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"CODE_SIGN_IDENTITY[sdk=iphoneos*]\" = \"iPhone Developer\";" + ProjectFileGenerator.NewLine);
								if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
								{
									if (bIsUE4Game)
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else if (IsAGame)
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"" + Target.TargetName + "/Intermediate/IOS/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + GamePath + "/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + GamePath + "/Binaries/IOS\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + GamePath + "/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Source/Programs/" + Target.TargetName + "/Resources/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
								}
								else
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"XcodeSupportFiles/" + Target.TargetName + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"XcodeSupportFiles/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"XcodeSupportFiles/build\";" + ProjectFileGenerator.NewLine);

									// if we're building for PC, set up Xcode to accommodate IphonePackager
									if (bIsUE4Game)
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
									}
									else if (IsAGame)
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
									}
								}
								Contents.Append(
									"\t\t\t\tINFOPLIST_OUTPUT_FORMAT = xml;" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PROVISIONING_PROFILE[sdk=iphoneos*]\" = \"\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
									"\t\t\t};" + ProjectFileGenerator.NewLine +
									"\t\t\tname = Debug;" + ProjectFileGenerator.NewLine +
									"\t\t};" + ProjectFileGenerator.NewLine);
								Contents.Append(
									"\t\t" + Target.TestConfigGuid + " /* Test */ = {" + ProjectFileGenerator.NewLine +
									"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
									"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"" + Target.TargetName + "-simulator\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"CODE_SIGN_IDENTITY[sdk=iphoneos*]\" = \"iPhone Developer\";" + ProjectFileGenerator.NewLine);
								if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
								{
									if (bIsUE4Game)
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else if (IsAGame)
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"" + Target.TargetName + "/Intermediate/IOS/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + GamePath + "/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + GamePath + "/Binaries/IOS\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + GamePath + "/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Source/Programs/" + Target.TargetName + "/Resources/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
								}
								else
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"XcodeSupportFiles/" + Target.TargetName + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"XcodeSupportFiles/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"XcodeSupportFiles/build\";" + ProjectFileGenerator.NewLine);

									// if we're building for PC, set up Xcode to accommodate IphonePackager
									if (bIsUE4Game)
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
									}
									else if (IsAGame)
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
									}
								}
								Contents.Append(
									"\t\t\t\tINFOPLIST_OUTPUT_FORMAT = xml;" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PROVISIONING_PROFILE[sdk=iphoneos*]\" = \"\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
									"\t\t\t};" + ProjectFileGenerator.NewLine +
									"\t\t\tname = Test;" + ProjectFileGenerator.NewLine +
									"\t\t};" + ProjectFileGenerator.NewLine);
							}
							else
							{
								Contents.Append(
									"\t\t" + Target.DebugGameConfigGuid + " /* DebugGame */ = {" + ProjectFileGenerator.NewLine +
									"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
									"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"" + Target.TargetName + "-simulator\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"CODE_SIGN_IDENTITY[sdk=iphoneos*]\" = \"iPhone Developer\";" + ProjectFileGenerator.NewLine);
								if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
								{
									if (bIsUE4Game)
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else if (IsAGame)
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"" + Target.TargetName + "/Intermediate/IOS/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + GamePath + "/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + GamePath + "/Binaries/IOS\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + GamePath + "/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else
									{
										Contents.Append(
											"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
											//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Source/Programs/" + Target.TargetName + "/Resources/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
								}
								else
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"XcodeSupportFiles/" + Target.TargetName + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"XcodeSupportFiles/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"XcodeSupportFiles/build\";" + ProjectFileGenerator.NewLine);

									// if we're building for PC, set up Xcode to accommodate IphonePackager
									if (bIsUE4Game)
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
									}
									else if (IsAGame)
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
									}
									else
									{
										Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
									}
								}
								Contents.Append(
									"\t\t\t\tINFOPLIST_OUTPUT_FORMAT = xml;" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\"PROVISIONING_PROFILE[sdk=iphoneos*]\" = \"\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
									"\t\t\t};" + ProjectFileGenerator.NewLine +
									"\t\t\tname = DebugGame;" + ProjectFileGenerator.NewLine +
									"\t\t};" + ProjectFileGenerator.NewLine);
							}
							Contents.Append(
								"\t\t" + Target.DevelopmentConfigGuid + " /* Development */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"" + Target.TargetName + "-simulator\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"CODE_SIGN_IDENTITY[sdk=iphoneos*]\" = \"iPhone Developer\";" + ProjectFileGenerator.NewLine);
							if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
							{
								if (bIsUE4Game)
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
										//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
								else if (IsAGame)
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
										//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"" + Target.TargetName + "/Intermediate/IOS/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + GamePath + "/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tSYMROOT = \"" + GamePath + "/Binaries/IOS\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"" + GamePath + "/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
								else
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
										//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Source/Programs/" + Target.TargetName + "/Resources/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
							}
							else
							{
								Contents.Append(
									"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"XcodeSupportFiles/" + Target.TargetName + ".entitlements\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tINFOPLIST_FILE = \"XcodeSupportFiles/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOBJROOT = \"XcodeSupportFiles/build\";" + ProjectFileGenerator.NewLine);

								// if we're building for PC, set up Xcode to accommodate IphonePackager
								if (bIsUE4Game)
								{
									Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
								}
								else if (IsAGame)
								{
									Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
								else
								{
									Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
								}
							}

							Contents.Append(
								"\t\t\t\tINFOPLIST_OUTPUT_FORMAT = xml;" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PROVISIONING_PROFILE[sdk=iphoneos*]\" = \"\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = Development;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine);

							Contents.Append(
								"\t\t" + Target.ShippingConfigGuid + " /* Shipping */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PRODUCT_NAME[sdk=iphonesimulator*]\" = \"" + Target.TargetName + "-simulator\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSToolChain.IOSVersion + ";" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"CODE_SIGN_IDENTITY[sdk=iphoneos*]\" = \"iPhone Developer\";" + ProjectFileGenerator.NewLine);
							if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
							{
								if (bIsUE4Game)
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
										//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
								else if (IsAGame)
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
										//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"" + Target.TargetName + "/Intermediate/IOS/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
											"\t\t\t\tINFOPLIST_FILE = \"" + GamePath + "/Intermediate/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tSYMROOT = \"" + GamePath + "/Binaries/IOS\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"" + GamePath + "/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
								else
								{
									Contents.Append(
										"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
										//"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \""Engine/Intermediate/Build/IOS/Debug/" + Target.Name + "/" + Target.Name + ".entitlements\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Source/Programs/" + Target.TargetName + "/Resources/IOS/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tSYMROOT = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tOBJROOT = \"" + EngineRelative + "Engine/Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
										"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineRelative + "Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
							}
							else
							{
								Contents.Append(
									"\t\t\t\tCODE_SIGN_ENTITLEMENTS = \"XcodeSupportFiles/" + Target.TargetName + ".entitlements\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tINFOPLIST_FILE = \"XcodeSupportFiles/" + Target.TargetName + "-Info.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOBJROOT = \"XcodeSupportFiles/build\";" + ProjectFileGenerator.NewLine);

								// if we're building for PC, set up Xcode to accommodate IphonePackager
								if (bIsUE4Game)
								{
									Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
								}
								else if (IsAGame)
								{
									Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
								}
								else
								{
									Contents.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"Payload\";" + ProjectFileGenerator.NewLine);
								}
							}

							Contents.Append(
								"\t\t\t\tINFOPLIST_OUTPUT_FORMAT = xml;" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\"PROVISIONING_PROFILE[sdk=iphoneos*]\" = \"\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTARGETED_DEVICE_FAMILY = \"1,2\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = Shipping;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine);
						}
						else
						{
							string EngineTarget = "";
							if (bIsUE4Game)
							{
								if (!bGeneratingGameProjectFiles && !bGeneratingRocketProjectFiles)
								{
									EngineTarget = "Engine/";
								}
							}

							if (!bGeneratingRocketProjectFiles)
							{
								Contents.Append(
									"\t\t" + Target.DebugConfigGuid + " /* Debug */ = {" + ProjectFileGenerator.NewLine +
									"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
									"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
									"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tBUNDLE_LOADER = \"" + EngineTarget + "Binaries/IOS/Payload/" + Target.TargetName + ".app/" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_ENABLE_MODULES = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_ENABLE_OBJC_ARC = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCODE_SIGN_IDENTITY = \"iPhone Developer\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCOPY_PHASE_STRIP = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tFRAMEWORK_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(SDKROOT)/Developer/Library/Frameworks\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(DEVELOPER_FRAMEWORKS_DIR)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t);" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu99;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tDYNAMIC_NO_PIC = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOPTIMIZATION_LEVEL = 0;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_PRECOMPILE_PREFIX_HEADER = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"DEBUG=1\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t);" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_SYMBOLS_PRIVATE_EXTERN = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Build/IOS/UE4CmdLineRun/UE4CmdLineRun-Info.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSYMROOT = \"" + EngineTarget + "Binaries/IOS\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOBJROOT = \"" + EngineTarget + "Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineTarget + "Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = 7.0;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tTEST_HOST = \"$(BUNDLE_LOADER)\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tWRAPPER_EXTENSION = xctest;" + ProjectFileGenerator.NewLine +
									"\t\t\t};" + ProjectFileGenerator.NewLine +
									"\t\t\tname = Debug;" + ProjectFileGenerator.NewLine +
									"\t\t};" + ProjectFileGenerator.NewLine +

									"\t\t" + Target.TestConfigGuid + " /* Test */ = {" + ProjectFileGenerator.NewLine +
									"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
									"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
									"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tBUNDLE_LOADER = \"" + EngineTarget + "Binaries/IOS/Payload/" + Target.TargetName + ".app/" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_ENABLE_MODULES = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_ENABLE_OBJC_ARC = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCODE_SIGN_IDENTITY = \"iPhone Developer\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCOPY_PHASE_STRIP = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tFRAMEWORK_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(SDKROOT)/Developer/Library/Frameworks\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(DEVELOPER_FRAMEWORKS_DIR)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t);" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu99;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tDYNAMIC_NO_PIC = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOPTIMIZATION_LEVEL = 0;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_PRECOMPILE_PREFIX_HEADER = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"DEBUG=1\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t);" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_SYMBOLS_PRIVATE_EXTERN = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Build/IOS/UE4CmdLineRun/UE4CmdLineRun-Info.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSYMROOT = \"" + EngineTarget + "Binaries/IOS\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOBJROOT = \"" + EngineTarget + "Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineTarget + "Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = 7.0;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tTEST_HOST = \"$(BUNDLE_LOADER)\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tWRAPPER_EXTENSION = xctest;" + ProjectFileGenerator.NewLine +
									"\t\t\t};" + ProjectFileGenerator.NewLine +
									"\t\t\tname = Test;" + ProjectFileGenerator.NewLine +
									"\t\t};" + ProjectFileGenerator.NewLine);
							}
							else
							{
								Contents.Append(
									"\t\t" + Target.DebugGameConfigGuid + " /* DebugGame */ = {" + ProjectFileGenerator.NewLine +
									"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
									"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
									"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tBUNDLE_LOADER = \"" + EngineTarget + "Binaries/IOS/Payload/" + Target.TargetName + ".app/" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_ENABLE_MODULES = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCLANG_ENABLE_OBJC_ARC = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCODE_SIGN_IDENTITY = \"iPhone Developer\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCOPY_PHASE_STRIP = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tFRAMEWORK_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(SDKROOT)/Developer/Library/Frameworks\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(DEVELOPER_FRAMEWORKS_DIR)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t);" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu99;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tDYNAMIC_NO_PIC = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOPTIMIZATION_LEVEL = 0;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_PRECOMPILE_PREFIX_HEADER = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"DEBUG=1\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
									"\t\t\t\t);" + ProjectFileGenerator.NewLine +
									"\t\t\t\tGCC_SYMBOLS_PRIVATE_EXTERN = NO;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Build/IOS/UE4CmdLineRun/UE4CmdLineRun-Info.plist\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSYMROOT = \"" + EngineTarget + "Binaries/IOS\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tOBJROOT = \"" + EngineTarget + "Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineTarget + "Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = 7.0;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
									"\t\t\t\tTEST_HOST = \"$(BUNDLE_LOADER)\";" + ProjectFileGenerator.NewLine +
									"\t\t\t\tWRAPPER_EXTENSION = xctest;" + ProjectFileGenerator.NewLine +
									"\t\t\t};" + ProjectFileGenerator.NewLine +
									"\t\t\tname = DebugGame;" + ProjectFileGenerator.NewLine +
									"\t\t};" + ProjectFileGenerator.NewLine);
							}
							Contents.Append(
								"\t\t" + Target.DevelopmentConfigGuid + " /* Development */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tBUNDLE_LOADER = \"" + EngineTarget + "Binaries/IOS/Payload/" + Target.TargetName + ".app/" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_ENABLE_MODULES = YES;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_ENABLE_OBJC_ARC = YES;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCODE_SIGN_IDENTITY = \"iPhone Developer\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCOPY_PHASE_STRIP = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tFRAMEWORK_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(SDKROOT)/Developer/Library/Frameworks\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(DEVELOPER_FRAMEWORKS_DIR)\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t);" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu99;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tDYNAMIC_NO_PIC = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tOPTIMIZATION_LEVEL = 0;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_PRECOMPILE_PREFIX_HEADER = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"DEBUG=1\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t);" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_SYMBOLS_PRIVATE_EXTERN = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Build/IOS/UE4CmdLineRun/UE4CmdLineRun-Info.plist\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSYMROOT = \"" + EngineTarget + "Binaries/IOS\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tOBJROOT = \"" + EngineTarget + "Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineTarget + "Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = 7.0;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTEST_HOST = \"$(BUNDLE_LOADER)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tWRAPPER_EXTENSION = xctest;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = Development;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine +

								"\t\t" + Target.ShippingConfigGuid + " /* Shipping */ = {" + ProjectFileGenerator.NewLine +
								"\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine +
								"\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine +
								"\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tBUNDLE_LOADER = \"" + EngineTarget + "Binaries/IOS/Payload/" + Target.TargetName + ".app/" + Target.TargetName + "\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"gnu++0x\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_CXX_LIBRARY = \"libc++\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_ENABLE_MODULES = YES;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCLANG_ENABLE_OBJC_ARC = YES;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCODE_SIGN_IDENTITY = \"iPhone Developer\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCODE_SIGN_RESOURCE_RULES_PATH = \"" + EngineRelative + "Engine/Build/iOS/XcodeSupportFiles/CustomResourceRules.plist\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCOPY_PHASE_STRIP = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tFRAMEWORK_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(SDKROOT)/Developer/Library/Frameworks\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(DEVELOPER_FRAMEWORKS_DIR)\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t);" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_C_LANGUAGE_STANDARD = gnu99;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tDYNAMIC_NO_PIC = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tOPTIMIZATION_LEVEL = 0;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_PRECOMPILE_PREFIX_HEADER = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"DEBUG=1\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t\t\"$(inherited)\"," + ProjectFileGenerator.NewLine +
								"\t\t\t\t);" + ProjectFileGenerator.NewLine +
								"\t\t\t\tGCC_SYMBOLS_PRIVATE_EXTERN = NO;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tINFOPLIST_FILE = \"" + EngineRelative + "Engine/Build/IOS/UE4CmdLineRun/UE4CmdLineRun-Info.plist\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSYMROOT = \"" + EngineTarget + "Binaries/IOS\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tOBJROOT = \"" + EngineTarget + "Intermediate/IOS/build\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + EngineTarget + "Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = 7.0;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tPRODUCT_NAME = \"$(TARGET_NAME)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tSDKROOT = iphoneos;" + ProjectFileGenerator.NewLine +
								"\t\t\t\tTEST_HOST = \"$(BUNDLE_LOADER)\";" + ProjectFileGenerator.NewLine +
								"\t\t\t\tWRAPPER_EXTENSION = xctest;" + ProjectFileGenerator.NewLine +
								"\t\t\t};" + ProjectFileGenerator.NewLine +
								"\t\t\tname = Shipping;" + ProjectFileGenerator.NewLine +
								"\t\t};" + ProjectFileGenerator.NewLine);
						}
					}
				}
			}
		}

		/// <summary>
		/// Appends a build configuration list section for specific target.
		/// </summary>
		/// <param name="Contents">StringBuilder object to append build configuration list string to</param>
		/// <param name="Target">Target for which we generate the build configuration list</param>
		private void AppendConfigList(ref StringBuilder Contents, XcodeProjectTarget Target)
		{
			string XcodeTargetType = Target.Type == "XCTest" ? "Native" : Target.Type;
			string TypeName = Target.Type == "Project" ? "PBXProject" : "PBX" + XcodeTargetType + "Target";

			if (!bGeneratingRocketProjectFiles)
			{
				Contents.Append(
					"\t\t" + Target.BuildConfigGuild + " /* Build configuration list for " + TypeName + " \"" + Target.DisplayName + "\" */ = {" + ProjectFileGenerator.NewLine +
					"\t\t\tisa = XCConfigurationList;" + ProjectFileGenerator.NewLine +
					"\t\t\tbuildConfigurations = (" + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.DebugConfigGuid + " /* Debug */," + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.DevelopmentConfigGuid + " /* Development */," + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.ShippingConfigGuid + " /* Shipping */," + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.TestConfigGuid + " /* Test */," + ProjectFileGenerator.NewLine +
					"\t\t\t);" + ProjectFileGenerator.NewLine +
					"\t\t\tdefaultConfigurationIsVisible = 0;" + ProjectFileGenerator.NewLine +
					"\t\t\tdefaultConfigurationName = Development;" + ProjectFileGenerator.NewLine +
					"\t\t};" + ProjectFileGenerator.NewLine);
			}
			else
			{
				Contents.Append(
					"\t\t" + Target.BuildConfigGuild + " /* Build configuration list for " + TypeName + " \"" + Target.DisplayName + "\" */ = {" + ProjectFileGenerator.NewLine +
					"\t\t\tisa = XCConfigurationList;" + ProjectFileGenerator.NewLine +
					"\t\t\tbuildConfigurations = (" + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.DebugGameConfigGuid + " /* Debug */," + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.DevelopmentConfigGuid + " /* Development */," + ProjectFileGenerator.NewLine +
					"\t\t\t\t" + Target.ShippingConfigGuid + " /* Shipping */," + ProjectFileGenerator.NewLine +
					"\t\t\t);" + ProjectFileGenerator.NewLine +
					"\t\t\tdefaultConfigurationIsVisible = 0;" + ProjectFileGenerator.NewLine +
					"\t\t\tdefaultConfigurationName = Development;" + ProjectFileGenerator.NewLine +
					"\t\t};" + ProjectFileGenerator.NewLine);
			}
		}


		/// <summary>
		/// Creates the container item proxy section to be added to the PBX Project.
		/// </summary>
		/// <param name="PBXTargetDependencySection">String to be modified with the container item proxy objects.</param>
		/// <param name="TargetDependencies">List of container item proxies to add.</param>
		private void CreateContainerItemProxySection(ref string PBXContainerItemProxySection, List<XcodeContainerItemProxy> ContainerItemProxies)
		{
			foreach(XcodeContainerItemProxy ContainerItemProxy in ContainerItemProxies)
			{
				PBXContainerItemProxySection += "\t\t" + ContainerItemProxy.Guid + " /* PBXContainerItemProxy */ = {" + ProjectFileGenerator.NewLine +
												"\t\t\tisa = PBXContainerItemProxy;" + ProjectFileGenerator.NewLine +
												"\t\t\tcontainerPortal = " + ContainerItemProxy.ProjectGuid + " /* Project object */;" + ProjectFileGenerator.NewLine +
												"\t\t\tproxyType = 1;" + ProjectFileGenerator.NewLine +
												"\t\t\tremoteGlobalIDString = " + ContainerItemProxy.LegacyTargetGuid + ";" + ProjectFileGenerator.NewLine +
												"\t\t\tremoteInfo = \"" + ContainerItemProxy.TargetName + "\";" + ProjectFileGenerator.NewLine +
												"\t\t};" + ProjectFileGenerator.NewLine;
			}
		}

		/// <summary>
		/// Creates the target dependency section to be added to the PBX Project.
		/// </summary>
		/// <param name="PBXTargetDependencySection">String to be modified with the target dependency objects.</param>
		/// <param name="TargetDependencies">List of target dependencies to add.</param>
		private void CreateTargetDependencySection(ref string PBXTargetDependencySection, List<XcodeTargetDependency> TargetDependencies)
		{
			foreach(XcodeTargetDependency TargetDependency in TargetDependencies)
			{
				PBXTargetDependencySection +=	"\t\t" + TargetDependency.Guid + " /* PBXTargetDependency */ = {" + ProjectFileGenerator.NewLine +
												"\t\t\tisa = PBXTargetDependency;" + ProjectFileGenerator.NewLine +
												"\t\t\ttarget = " + TargetDependency.LegacyTargetGuid + " /* " + TargetDependency.LegacyTargetName + " */;" + ProjectFileGenerator.NewLine +
												"\t\t\ttargetProxy = " + TargetDependency.ContainerItemProxyGuid + " /* PBXContainerItemProxy */;" + ProjectFileGenerator.NewLine +
												"\t\t};" + ProjectFileGenerator.NewLine;
			}
		}

		/// <summary>
		/// Writes a scheme file that holds user-specific info needed for debugging.
		/// </summary>
		/// <param name="Target">Target for which we write the scheme file</param>
		/// <param name="ExeExtension">Extension of the executable used to run and debug this target (".app" for bundles, "" for command line apps</param>
		/// <param name="ExeBaseName">Base name of the executable used to run and debug this target</param>
		/// <param name="Args">List of command line arguments for running this target</param>
		private void WriteSchemeFile(string XcodeProjectPath, XcodeProjectTarget Target, string ExeExtension = "", string ExeBaseName = "", List<string> Args = null)
		{
			if (ExeBaseName == "")
			{
				ExeBaseName = Target.TargetName;
			}

			string TargetBinariesFolder;
			if (ExeBaseName.StartsWith("/"))
			{
				TargetBinariesFolder = Path.GetDirectoryName(ExeBaseName);
				ExeBaseName = Path.GetFileName(ExeBaseName);
			}
			else
			{
				TargetBinariesFolder = Path.GetDirectoryName(Directory.GetCurrentDirectory());

			if( ExeBaseName == Target.TargetName && ExeBaseName.EndsWith("Game") && ExeExtension == ".app" )
			{
				TargetBinariesFolder = TargetBinariesFolder.Replace("/Engine", "/") + ExeBaseName + "/Binaries/";
			}
			else
			{
				TargetBinariesFolder += "/Binaries/";
			}

			// append the platform directory
			TargetBinariesFolder += Target.TargetPlatform.ToString();
			}

			string SchemeFileContent =
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + ProjectFileGenerator.NewLine +
				"<Scheme" + ProjectFileGenerator.NewLine +
				"  LastUpgradeVersion = \"0500\"" + ProjectFileGenerator.NewLine +
				"  version = \"1.3\">" + ProjectFileGenerator.NewLine +
				"  <BuildAction" + ProjectFileGenerator.NewLine +
				"     parallelizeBuildables = \"YES\"" + ProjectFileGenerator.NewLine +
				"     buildImplicitDependencies = \"YES\">" + ProjectFileGenerator.NewLine +
				"     <BuildActionEntries>" + ProjectFileGenerator.NewLine +
				"        <BuildActionEntry" + ProjectFileGenerator.NewLine +
				"           buildForTesting = \"YES\"" + ProjectFileGenerator.NewLine +
				"           buildForRunning = \"YES\"" + ProjectFileGenerator.NewLine +
				"           buildForProfiling = \"YES\"" + ProjectFileGenerator.NewLine +
				"           buildForArchiving = \"YES\"" + ProjectFileGenerator.NewLine +
				"           buildForAnalyzing = \"YES\">" + ProjectFileGenerator.NewLine +
				"           <BuildableReference" + ProjectFileGenerator.NewLine +
				"              BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine +
				"              BlueprintIdentifier = \"" + Target.Guid + "\"" + ProjectFileGenerator.NewLine +
				"              BuildableName = \"" + Target.ProductName + "\"" + ProjectFileGenerator.NewLine +
				"              BlueprintName = \"" + Target.DisplayName + "\"" + ProjectFileGenerator.NewLine +
				"              ReferencedContainer = \"container:" + MasterProjectName + ".xcodeproj\">" + ProjectFileGenerator.NewLine +
				"           </BuildableReference>" + ProjectFileGenerator.NewLine +
				"        </BuildActionEntry>" + ProjectFileGenerator.NewLine +
				"     </BuildActionEntries>" + ProjectFileGenerator.NewLine +
				"  </BuildAction>" + ProjectFileGenerator.NewLine +
				"  <TestAction" + ProjectFileGenerator.NewLine +
				"      selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\"" + ProjectFileGenerator.NewLine +
				"     selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\"" + ProjectFileGenerator.NewLine +
				"     shouldUseLaunchSchemeArgsEnv = \"YES\"" + ProjectFileGenerator.NewLine +
				"      buildConfiguration = \"Debug" + (bGeneratingRocketProjectFiles ? "Game" : "") + "\">" + ProjectFileGenerator.NewLine +
				"     <Testables>" + ProjectFileGenerator.NewLine +
				"        <TestableReference" + ProjectFileGenerator.NewLine +
				"           skipped = \"NO\">" + ProjectFileGenerator.NewLine +
				"           <BuildableReference" + ProjectFileGenerator.NewLine +
				"              BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine +
				"              BlueprintIdentifier = \"" + UE4CmdLineGuid + "\"" + ProjectFileGenerator.NewLine +
				"              BuildableName = \"UE4CmdLineRun.xctest\"" + ProjectFileGenerator.NewLine +
				"              BlueprintName = \"UE4CmdLineRun\"" + ProjectFileGenerator.NewLine +
				"              ReferencedContainer = \"container:" + MasterProjectName + ".xcodeproj\">" + ProjectFileGenerator.NewLine +
				"           </BuildableReference>" + ProjectFileGenerator.NewLine +
				"        </TestableReference>" + ProjectFileGenerator.NewLine +
				"     </Testables>" + ProjectFileGenerator.NewLine +
				"     <MacroExpansion>" + ProjectFileGenerator.NewLine +
				"        <BuildableReference" + ProjectFileGenerator.NewLine +
				"           BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine +
				"           BlueprintIdentifier = \"" + Target.Guid + "\"" + ProjectFileGenerator.NewLine +
				"           BuildableName = \"" + Target.ProductName + "\"" + ProjectFileGenerator.NewLine +
				"           BlueprintName = \"" + Target.DisplayName + "\"" + ProjectFileGenerator.NewLine +
				"           ReferencedContainer = \"container:" + MasterProjectName + ".xcodeproj\">" + ProjectFileGenerator.NewLine +
				"        </BuildableReference>" + ProjectFileGenerator.NewLine +
				"     </MacroExpansion>" + ProjectFileGenerator.NewLine +
				"  </TestAction>" + ProjectFileGenerator.NewLine +
				"  <LaunchAction" + ProjectFileGenerator.NewLine +
				"     selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\"" + ProjectFileGenerator.NewLine +
				"     selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\"" + ProjectFileGenerator.NewLine +
				"     launchStyle = \"0\"" + ProjectFileGenerator.NewLine +
				"     useCustomWorkingDirectory = \"NO\"" + ProjectFileGenerator.NewLine +
				"     buildConfiguration = \"Debug" + (bGeneratingRocketProjectFiles ? "Game" : "") + "\"" + ProjectFileGenerator.NewLine +
				"     ignoresPersistentStateOnLaunch = \"NO\"" + ProjectFileGenerator.NewLine +
				"     debugDocumentVersioning = \"YES\"" + ProjectFileGenerator.NewLine +
				"     allowLocationSimulation = \"YES\">" + ProjectFileGenerator.NewLine;
			if (Target.TargetPlatform == UnrealTargetPlatform.Mac)
			{
				// For non-rocket projects the default Run targtet is always in Debug config, so we always add -Mac-Debug suffix.
				// For rocket projects, we have DebugGame config, so the editor runs in Development mode (no suffix), but the game in Debug (so it needs suffix).
				string ExeConfigSuffix = (!bGeneratingRocketProjectFiles || !ExeBaseName.EndsWith("Editor")) ? "-Mac-Debug" : "";
				SchemeFileContent +=
				"     <PathRunnable" + ProjectFileGenerator.NewLine +
					"        FilePath = \"" + TargetBinariesFolder + "/" + ExeBaseName + ExeConfigSuffix + ExeExtension + "\">" + ProjectFileGenerator.NewLine +
				"     </PathRunnable>" + ProjectFileGenerator.NewLine;
			}
			else
			{
				SchemeFileContent +=
					"     <BuildableProductRunnable>" + ProjectFileGenerator.NewLine +
					"        <BuildableReference" + ProjectFileGenerator.NewLine +
					"           BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine +
					"           BlueprintIdentifier = \"" + Target.Guid + "\"" + ProjectFileGenerator.NewLine +
					"           BuildableName = \"" + Target.ProductName + "\"" + ProjectFileGenerator.NewLine +
					"           BlueprintName = \"" + Target.DisplayName + "\"" + ProjectFileGenerator.NewLine +
					"           ReferencedContainer = \"container:" + MasterProjectName + ".xcodeproj\">" + ProjectFileGenerator.NewLine +
					"        </BuildableReference>" + ProjectFileGenerator.NewLine +
					"     </BuildableProductRunnable>" + ProjectFileGenerator.NewLine;
			}

			if (Args != null)
			{
				SchemeFileContent += "     <CommandLineArguments>" + ProjectFileGenerator.NewLine;

				Args.Add("-debug");

				foreach (string Arg in Args)
				{
					SchemeFileContent += 
						"        <CommandLineArgument" + ProjectFileGenerator.NewLine +
						"           argument = \"" + Arg + "\"" + ProjectFileGenerator.NewLine +
						"           isEnabled = \"YES\">" + ProjectFileGenerator.NewLine +
						"        </CommandLineArgument>" + ProjectFileGenerator.NewLine;
				}
				SchemeFileContent += "      </CommandLineArguments>" + ProjectFileGenerator.NewLine;
			}

			string Runnable = TargetBinariesFolder + "/" + ExeBaseName + ExeExtension;
			SchemeFileContent +=
				"     <AdditionalOptions>" + ProjectFileGenerator.NewLine +
				"     </AdditionalOptions>" + ProjectFileGenerator.NewLine +
				"  </LaunchAction>" + ProjectFileGenerator.NewLine +
				"  <ProfileAction" + ProjectFileGenerator.NewLine +
				"     shouldUseLaunchSchemeArgsEnv = \"YES\"" + ProjectFileGenerator.NewLine +
				"     savedToolIdentifier = \"\"" + ProjectFileGenerator.NewLine +
				"     useCustomWorkingDirectory = \"NO\"" + ProjectFileGenerator.NewLine +
				"     buildConfiguration = \"Development\"" + ProjectFileGenerator.NewLine +
				"     debugDocumentVersioning = \"YES\">" + ProjectFileGenerator.NewLine +
				"     <PathRunnable" + ProjectFileGenerator.NewLine +
				"        FilePath = \"" + Runnable + "\">" + ProjectFileGenerator.NewLine +
				"     </PathRunnable>" + ProjectFileGenerator.NewLine +
				"     <BuildableProductRunnable>" + ProjectFileGenerator.NewLine +
				"         <BuildableReference" + ProjectFileGenerator.NewLine +
				"             BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine +
				"	          BlueprintIdentifier = \"" + Target.Guid + "\"" + ProjectFileGenerator.NewLine +
				"             BuildableName = \"" + ExeBaseName + ExeExtension + "\"" + ProjectFileGenerator.NewLine +
				"             BlueprintName = \"" + ExeBaseName + "_RunIOS\"" + ProjectFileGenerator.NewLine +
				"             ReferencedContainer = \"container:UE4.xcodeproj\">" + ProjectFileGenerator.NewLine +
				"         </BuildableReference>" + ProjectFileGenerator.NewLine +
				"     </BuildableProductRunnable>" + ProjectFileGenerator.NewLine +
				"  </ProfileAction>" + ProjectFileGenerator.NewLine +
				"  <AnalyzeAction" + ProjectFileGenerator.NewLine +
				"     buildConfiguration = \"Debug" + (bGeneratingRocketProjectFiles ? "Game" : "") + "\">" + ProjectFileGenerator.NewLine +
				"  </AnalyzeAction>" + ProjectFileGenerator.NewLine +
				"  <ArchiveAction" + ProjectFileGenerator.NewLine +
				"     buildConfiguration = \"Development\"" + ProjectFileGenerator.NewLine +
				"     revealArchiveInOrganizer = \"YES\">" + ProjectFileGenerator.NewLine +
				"  </ArchiveAction>" + ProjectFileGenerator.NewLine +
				"</Scheme>" + ProjectFileGenerator.NewLine;

			string SchemesDir = XcodeProjectPath + "/xcuserdata/" + Environment.UserName + ".xcuserdatad/xcschemes";
			if (!Directory.Exists(SchemesDir))
			{
				Directory.CreateDirectory(SchemesDir);
			}

			string SchemeFilePath = SchemesDir + Path.DirectorySeparatorChar + Target.DisplayName + ".xcscheme";
			File.WriteAllText(SchemeFilePath, SchemeFileContent);
		}

		/// Adds the include directory to the list, after converting it to relative to UE4 root
		private void AddIncludeDirectory(ref List<string> IncludeDirectories, string IncludeDir, string ProjectDir)
		{
			string FullPath = Path.GetFullPath(Path.Combine(ProjectDir, IncludeDir));
			FullPath = Utils.MakePathRelativeTo(FullPath, Path.GetFullPath(Path.Combine(Directory.GetCurrentDirectory(), "../..")));
			FullPath = FullPath.TrimEnd('/');
			if (!IncludeDirectories.Contains(FullPath))
			{
				IncludeDirectories.Add(FullPath);
			}
		}

		private void AppendBuildPhase(ref string Contents, string Guid, string BuildPhaseName, string Files, List<string> FileRefs)
		{
			Contents += "\t\t" + Guid + " /* " + BuildPhaseName + " */ = {" + ProjectFileGenerator.NewLine +
				"\t\t\tisa = PBX" + BuildPhaseName + "BuildPhase;" + ProjectFileGenerator.NewLine +
				"\t\t\tbuildActionMask = 2147483647;" + ProjectFileGenerator.NewLine +
				"\t\t\tfiles = (" + ProjectFileGenerator.NewLine;

			if (Files != null)
			{
				Contents += Files;
			}

			Contents += "\t\t\t);" + ProjectFileGenerator.NewLine +
				"\t\t\trunOnlyForDeploymentPostprocessing = 0;" + ProjectFileGenerator.NewLine +
				"\t\t};" + ProjectFileGenerator.NewLine;
		}

		string TrimTargetCs (string path)
		{
			int period = path.IndexOf('.');
			if(period > -1)
				return path.Substring(0, period);
			return path;
		}

		/// <summary>
		/// Writes the project files to disk
		/// </summary>
		/// <returns>True if successful</returns>
		protected override bool WriteProjectFiles ()
		{
			Log.TraceInformation ("Generating project file...");

            bool bAddSourceFiles = true;
			bool bAddOnlyTargetSources = true;
			bool bOnlyFolderRefs = (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac);

			// Setup project file content
			var XcodeProjectFileContent = new StringBuilder ();

			XcodeProjectFileContent.Append (
				"// !$*UTF8*$!" + ProjectFileGenerator.NewLine +
				"{" + ProjectFileGenerator.NewLine +
				"\tarchiveVersion = 1;" + ProjectFileGenerator.NewLine +
				"\tclasses = {" + ProjectFileGenerator.NewLine +
				"\t};" + ProjectFileGenerator.NewLine +
				"\tobjectVersion = 46;" + ProjectFileGenerator.NewLine +
				"\tobjects = {" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);

			// attempt to determine targets for the project
			List<XcodeProjectTarget> ProjectTargets = new List<XcodeProjectTarget> ();
			// add mandatory ones
			XcodeProjectTarget UE4ProjectTarget = new XcodeProjectTarget ("UE4", "UE4", "Project");
			XcodeProjectTarget UE4XcodeHelperTarget = new XcodeProjectTarget ("UE4XcodeHelper", "UE4XcodeHelper", "Native", "libUE4XcodeHelper.a");
			ProjectTargets.AddRange(new XcodeProjectTarget[] { UE4ProjectTarget, UE4XcodeHelperTarget });

			if (ProjectFilePlatform.HasFlag(XcodeProjectFilePlatform.iOS))
			{
				XcodeProjectTarget UE4CmdLineRunTarget = new XcodeProjectTarget("UE4CmdLineRun", "UE4CmdLineRun", "XCTest", "UE4CmdLineRun.xctest", UnrealTargetPlatform.IOS);
				ProjectTargets.AddRange(new XcodeProjectTarget[] { UE4ProjectTarget, UE4XcodeHelperTarget, UE4CmdLineRunTarget });
				// This GUID will be referenced by each app's test action.
				UE4CmdLineGuid = UE4CmdLineRunTarget.Guid;
			}

			List<XcodeTargetDependency> TargetDependencies = new List<XcodeTargetDependency>();
			List<XcodeContainerItemProxy> ContainerItemProxies = new List<XcodeContainerItemProxy>();
            List<XcodeFramework> Frameworks = new List<XcodeFramework>();
            Frameworks.Add(new XcodeFramework("OpenGLES.framework", "System/Library/Frameworks/OpenGLES.framework", "SDKROOT"));

			XcodeFramework XCTestFramework = new XcodeFramework("XCTest.framework", "Library/Frameworks/XCTest.framework", "DEVELOPER_DIR");
            Frameworks.Add(XCTestFramework);

			var AllTargets = DiscoverTargets();

			PopulateTargets(AllTargets, ProjectTargets, ContainerItemProxies, TargetDependencies, UE4ProjectTarget, Frameworks);

			Log.TraceInformation (string.Format ("Found {0} targets!", ProjectTargets.Count));

			string PBXBuildFileSection = "/* Begin PBXBuildFile section */" + ProjectFileGenerator.NewLine;
			string PBXFileReferenceSection = "/* Begin PBXFileReference section */" + ProjectFileGenerator.NewLine;
			string PBXResourcesBuildPhaseSection = "/* Begin PBXResourcesBuildPhase section */" + ProjectFileGenerator.NewLine;
			string PBXSourcesBuildPhaseSection = "/* Begin PBXSourcesBuildPhase section */" + ProjectFileGenerator.NewLine;
			string PBXFrameworksBuildPhaseSection = "/* Begin PBXFrameworksBuildPhase section */" + ProjectFileGenerator.NewLine;
			string PBXShellScriptBuildPhaseSection = "/* Begin PBXShellScriptBuildPhase section */" + ProjectFileGenerator.NewLine;
			Dictionary<string, XcodeFileGroup> Groups = new Dictionary<string, XcodeFileGroup>();
			List<string> IncludeDirectories = new List<string>();
			List<string> PreprocessorDefinitions = new List<string>();

            foreach (XcodeFramework Framework in Frameworks)
            {
                // Add file references.
                PBXFileReferenceSection += "\t\t" + Framework.Guid + " /* " + Framework.Name + " */ = {"
                                         + "isa = PBXFileReference; "
                                         + "lastKnownFileType = wrapper.framework; "
                                         + "name = " + Framework.Name + "; "
                                         + "path = " + Framework.Path + "; "
                                         + "sourceTree = " + Framework.SourceTree + "; "
                                         + "};" + ProjectFileGenerator.NewLine;
            }

			// Set up all the test guids that need to be explicitly referenced later
			string UE4CmdLineRunMFileGuid = MakeXcodeGuid();
			UE4CmdLineRunMFileRefGuid = MakeXcodeGuid();
			string XCTestBuildFileGUID = MakeXcodeGuid();

			string TestFrameworkFiles = "";
			{
				PBXBuildFileSection += "\t\t" + XCTestBuildFileGUID + " /* XCTest.framework in Frameworks */ = {isa = PBXBuildFile; "
													+ "fileRef = " + XCTestFramework.Guid + " /* UE4CmdLineRun.m */; };" + ProjectFileGenerator.NewLine;

				TestFrameworkFiles += "\t\t\t\t" + XCTestBuildFileGUID + " /* XCTest.framework in Frameworks */," + ProjectFileGenerator.NewLine;
			}

			foreach (XcodeProjectTarget Target in ProjectTargets)
			{
				if (Target.Type == "Native" || Target.Type == "XCTest")
				{
                    // Generate Build references and Framework references for each Framework.
                    string FrameworkFiles = "";
                    foreach (XcodeFrameworkRef FrameworkRef in Target.FrameworkRefs)
                    {
                        XcodeFramework Framework = FrameworkRef.Framework;

                        PBXBuildFileSection += "\t\t" + FrameworkRef.Guid + " /* " + Framework.Name + " in Frameworks */ = {"
                                            + "isa = PBXBuildFile; "
                                            + "fileRef = " + Framework.Guid
                                            + " /* " + Framework.Name + " */;"
                                            + " };" + ProjectFileGenerator.NewLine;

                        FrameworkFiles += "\t\t\t\t" + FrameworkRef.Guid + " /* " + Framework.Name + " in Frameworks */," + ProjectFileGenerator.NewLine;
                    }

					AppendBuildPhase(ref PBXResourcesBuildPhaseSection, Target.ResourcesPhaseGuid, "Resources", null, null);

					string Sources = null;

					if (Target.Type == "XCTest")
					{
						// Add the xctest framework.
						FrameworkFiles += TestFrameworkFiles;

						// Normally every project either gets all source files compiled into it or none.
						// We just want one file: UE4CmdLineRun.m
						Sources = "\t\t\t\t" + UE4CmdLineRunMFileGuid + " /* UE4CmdLineRun.m in Sources */," + ProjectFileGenerator.NewLine;
					}					

					AppendBuildPhase(ref PBXSourcesBuildPhaseSection, Target.SourcesPhaseGuid, "Sources", Sources, null);

					AppendBuildPhase(ref PBXFrameworksBuildPhaseSection, Target.FrameworksPhaseGuid, "Frameworks", FrameworkFiles, null);

					string PayloadDir = "Engine";
					if (Target.Type == "Native" && Target.TargetName != "UE4Game")
					{
						PayloadDir = Target.TargetName;
					}
					// add a script for preventing errors during Archive builds
					// this copies the contents of the app generated by UBT into the archiving app
					PBXShellScriptBuildPhaseSection += "\t\t" + Target.ShellScriptPhaseGuid + " /* ShellScript */ = {" + ProjectFileGenerator.NewLine +
						"\t\t\tisa = PBXShellScriptBuildPhase;" + ProjectFileGenerator.NewLine +
						"\t\t\tbuildActionMask = 2147483647;" + ProjectFileGenerator.NewLine +
						"\t\t\tfiles = (" + ProjectFileGenerator.NewLine +
						"\t\t\t);" + ProjectFileGenerator.NewLine +
						"\t\t\trunOnlyForDeploymentPostprocessing = 0;" + ProjectFileGenerator.NewLine +
						"\t\t\tshellPath = /bin/sh;" + ProjectFileGenerator.NewLine +
						"\t\t\tshellScript = \"if [ $DEPLOYMENT_LOCATION = \\\"YES\\\" ]\\nthen\\ncp -R " + PayloadDir +"/Binaries/IOS/Payload/" + Target.ProductName + "/ $DSTROOT/$LOCAL_APPS_DIR/" + Target.ProductName + "\\nfi\";" + ProjectFileGenerator.NewLine +
						"\t\t};" + ProjectFileGenerator.NewLine;
				}
			}

            if (bAddSourceFiles)
			{
				PBXSourcesBuildPhaseSection += "\t\t" + UE4XcodeHelperTarget.SourcesPhaseGuid + " /* Sources */ = {" + ProjectFileGenerator.NewLine +
					"\t\t\tisa = PBXSourcesBuildPhase;" + ProjectFileGenerator.NewLine +
					"\t\t\tbuildActionMask = 2147483647;" + ProjectFileGenerator.NewLine +
					"\t\t\tfiles = (" + ProjectFileGenerator.NewLine;

				PBXFileReferenceSection += "\t\t" + UE4XcodeHelperTarget.ProductGuid + " /* " + UE4XcodeHelperTarget.ProductName + " */ = {isa = PBXFileReference; explicitFileType = archive.ar; includeInIndex = 0; path = " + UE4XcodeHelperTarget.ProductName + "; sourceTree = BUILT_PRODUCTS_DIR; };" + ProjectFileGenerator.NewLine;
				foreach (var CurProject in GeneratedProjectFiles)
				{
                    if (bAddOnlyTargetSources)
                    {
                        bool bHasTarget = false;
                        foreach (var TargetFile in CurProject.ProjectTargets)
                        {
                            if (TargetFile.TargetFilePath == null)
                            {
                                continue;
                            }

                            string TargetFileName = Path.GetFileNameWithoutExtension(TargetFile.TargetFilePath);
                            string TargetFileMinusTarget = TargetFileName.Substring(0, TargetFileName.LastIndexOf(".Target"));
                            foreach (XcodeProjectTarget Target in ProjectTargets)
                            {
                                if (TargetFileMinusTarget == Target.TargetName)
                                {
                                    bHasTarget = true;
                                    break;
                                }
                            }
                            if (bHasTarget)
                            {
                                break;
                            }
                        }

                     if (!bHasTarget)
					{
                            continue;
                      }
                    }

					XcodeProjectFile XcodeProject = CurProject as XcodeProjectFile;
					if (XcodeProject == null)
					{
						continue;
					}

					// Necessary so that GenerateSectionContents can use the same GUID instead of
					// auto-generating one for this special-case file.
					XcodeProject.UE4CmdLineRunFileGuid = UE4CmdLineRunMFileGuid;
					XcodeProject.UE4CmdLineRunFileRefGuid = UE4CmdLineRunMFileRefGuid;

                    if (bOnlyFolderRefs)
                    {
                        foreach (var CurSourceFile in XcodeProject.SourceFiles)
                        {
                            XcodeSourceFile SourceFile = CurSourceFile as XcodeSourceFile;
                            string GroupPath = Path.GetFullPath(Path.GetDirectoryName(SourceFile.FilePath));
                            XcodeFileGroup Group = XcodeProject.FindGroupByFullPath(ref Groups, GroupPath);
                            if (Group != null)
                            {
                                Group.bReference = true;
                            }

                        }
                    }
                    else
                    {
					XcodeProject.GenerateSectionsContents (ref PBXBuildFileSection, ref PBXFileReferenceSection, ref PBXSourcesBuildPhaseSection, ref Groups);
                    }

					foreach (var CurPath in XcodeProject.IntelliSenseIncludeSearchPaths)
					{
						AddIncludeDirectory (ref IncludeDirectories, CurPath, Path.GetDirectoryName (XcodeProject.ProjectFilePath));
					}

					foreach (var CurDefinition in XcodeProject.IntelliSensePreprocessorDefinitions)
					{
						string AlternateDefinition = CurDefinition.Contains("=0") ? CurDefinition.Replace("=0", "=1") :  CurDefinition.Replace("=1", "=0");
						if (!PreprocessorDefinitions.Contains(CurDefinition) && !PreprocessorDefinitions.Contains(AlternateDefinition) && !CurDefinition.StartsWith("UE_ENGINE_DIRECTORY") && !CurDefinition.StartsWith("ORIGINAL_FILE_NAME"))
						{
							PreprocessorDefinitions.Add (CurDefinition);
						}
					}
				}
				PBXSourcesBuildPhaseSection += "\t\t\t);" + ProjectFileGenerator.NewLine +
				"\t\t\trunOnlyForDeploymentPostprocessing = 0;" + ProjectFileGenerator.NewLine +
				"\t\t};" + ProjectFileGenerator.NewLine;

				if( !bGeneratingRocketProjectFiles )
				{
					// Add UnrealBuildTool to the master project
					string ProjectPath = System.IO.Path.Combine(System.IO.Path.Combine(EngineRelativePath, "Source"), "Programs", "UnrealBuildTool", "UnrealBuildTool_Mono.csproj");
					AddPreGeneratedProject(ref PBXBuildFileSection, ref PBXFileReferenceSection, ref PBXSourcesBuildPhaseSection, ref Groups, ProjectPath);
				}
			}

			foreach (XcodeProjectTarget Target in ProjectTargets)
			{
				if ((Target.Type == "Native" || Target.Type == "XCTest") && Target != UE4XcodeHelperTarget)
				{
					string FileType = Target.Type == "Native" ? "wrapper.application" : "wrapper.cfbundle";
					string PayloadDir = "Engine";
					if (Target.Type == "Native" && Target.TargetName != "UE4Game")
					{
						PayloadDir = Target.TargetName;
					}

					PBXFileReferenceSection += "\t\t" + Target.ProductGuid + " /* " + Target.ProductName + " */ = {isa = PBXFileReference; explicitFileType = " + FileType + "; includeInIndex = 0; path = " + PayloadDir + "/Binaries/IOS/Payload/" + Target.ProductName + "; sourceTree = \"<group>\"; };" + ProjectFileGenerator.NewLine;
					if (!string.IsNullOrEmpty(Target.PlistGuid))
					{
						if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
						{
							PBXFileReferenceSection += "\t\t" + Target.PlistGuid + " /* " + Target.TargetName + "-Info.plist */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = text.plist.xml; name = \"" + Target.TargetName + "-Info.plist\"; path = " + Target.TargetName + "/Build/IOS/" + Target.TargetName + "-Info.plist; sourceTree = \"<group>\"; };" + ProjectFileGenerator.NewLine;
						}
						else
						{
							PBXFileReferenceSection += "\t\t" + Target.PlistGuid + " /* " + Target.TargetName + "-Info.plist */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = text.plist.xml; name = \"" + Target.TargetName + "-Info.plist\"; path = XcodeSupportFiles/" + Target.TargetName + "-Info.plist; sourceTree = \"<group>\"; };" + ProjectFileGenerator.NewLine;
						}
					}
				}
			}

            if (bOnlyFolderRefs)
            {
                WriteReferenceGroups(ref PBXFileReferenceSection, ref Groups);
            }

			PBXBuildFileSection += "/* End PBXBuildFile section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;
			PBXFileReferenceSection += "/* End PBXFileReference section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;

			PBXResourcesBuildPhaseSection += "/* End PBXResourcesBuildPhase section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;
			PBXSourcesBuildPhaseSection += "/* End PBXSourcesBuildPhase section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;
			PBXFrameworksBuildPhaseSection += "/* End PBXFrameworksBuildPhase section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;
			PBXShellScriptBuildPhaseSection += "/* End PBXShellScriptBuildPhase section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;

			string PBXTargetDependencySection = "/* Begin PBXTargetDependency section */" + ProjectFileGenerator.NewLine;
			CreateTargetDependencySection(ref PBXTargetDependencySection, TargetDependencies);
			PBXTargetDependencySection += "/* End PBXTargetDependency section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;

			string PBXContainerItemProxySection = "/* Begin PBXContainerItemProxy section */" + ProjectFileGenerator.NewLine;
			CreateContainerItemProxySection(ref PBXContainerItemProxySection, ContainerItemProxies);
			PBXContainerItemProxySection += "/* End PBXContainerItemProxy section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine;

			XcodeProjectFileContent.Append (PBXBuildFileSection);
			XcodeProjectFileContent.Append (PBXFileReferenceSection);
			XcodeProjectFileContent.Append (PBXContainerItemProxySection);
			XcodeProjectFileContent.Append(PBXTargetDependencySection);


			AppendGroups(ref XcodeProjectFileContent, ref Groups, ProjectTargets, UE4XcodeHelperTarget, Frameworks);


			XcodeProjectFileContent.Append(PBXShellScriptBuildPhaseSection);
			XcodeProjectFileContent.Append(PBXSourcesBuildPhaseSection);
			XcodeProjectFileContent.Append(PBXFrameworksBuildPhaseSection);

			XcodeProjectFileContent.Append ("/* Begin PBXLegacyTarget section */" + ProjectFileGenerator.NewLine);
			foreach (var target in ProjectTargets)
			{
				if (target.Type == "Legacy")
				{
					AppendTarget (ref XcodeProjectFileContent, target);
				}
			}
			XcodeProjectFileContent.Append ("/* End PBXLegacyTarget section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);

			XcodeProjectFileContent.Append ("/* Begin PBXNativeTarget section */" + ProjectFileGenerator.NewLine);
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				AppendTarget (ref XcodeProjectFileContent, UE4XcodeHelperTarget);
			}
			foreach (XcodeProjectTarget Target in ProjectTargets)
			{
				if ((Target.Type == "Native" || Target.Type == "XCTest") && Target != UE4XcodeHelperTarget)
				{
					AppendTarget(ref XcodeProjectFileContent, Target);
				}
			}
			XcodeProjectFileContent.Append ("/* End PBXNativeTarget section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);

			XcodeProjectFileContent.Append (
				"/* Begin PBXProject section */" + ProjectFileGenerator.NewLine +
				"\t\t" + UE4ProjectTarget.Guid + " /* Project object */ = {" + ProjectFileGenerator.NewLine +
				"\t\t\tisa = PBXProject;" + ProjectFileGenerator.NewLine +
				"\t\t\tattributes = {" + ProjectFileGenerator.NewLine +
				"\t\t\t\tLastUpgradeCheck = 0440;" + ProjectFileGenerator.NewLine +
				"\t\t\t\tORGANIZATIONNAME = EpicGames;" + ProjectFileGenerator.NewLine +
				"\t\t\t};" + ProjectFileGenerator.NewLine +
				"\t\t\tbuildConfigurationList = " + UE4ProjectTarget.BuildConfigGuild + " /* Build configuration list for PBXProject \"" + UE4ProjectTarget.DisplayName + "\" */;" + ProjectFileGenerator.NewLine +
				"\t\t\tcompatibilityVersion = \"Xcode 3.2\";" + ProjectFileGenerator.NewLine +
				"\t\t\tdevelopmentRegion = English;" + ProjectFileGenerator.NewLine +
				"\t\t\thasScannedForEncodings = 0;" + ProjectFileGenerator.NewLine +
				"\t\t\tknownRegions = (" + ProjectFileGenerator.NewLine +
				"\t\t\t\ten," + ProjectFileGenerator.NewLine +
				"\t\t\t);" + ProjectFileGenerator.NewLine +
				"\t\t\tmainGroup = " + MainGroupGuid + ";" + ProjectFileGenerator.NewLine +
				"\t\t\tproductRefGroup = " + ProductRefGroupGuid + " /* Products */;" + ProjectFileGenerator.NewLine +
				"\t\t\tprojectDirPath = \"\";" + ProjectFileGenerator.NewLine +
				"\t\t\tprojectRoot = \"\";" + ProjectFileGenerator.NewLine +
				"\t\t\ttargets = (" + ProjectFileGenerator.NewLine
			);
			foreach (var target in ProjectTargets)
			{
				if (target == UE4ProjectTarget || target == UE4XcodeHelperTarget)
				{
					continue;
				}
				XcodeProjectFileContent.AppendLine (string.Format ("\t\t\t\t{0} /* {1} */,", target.Guid, target.DisplayName));
			}
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				XcodeProjectFileContent.Append ("\t\t\t\t" + UE4XcodeHelperTarget.Guid + " /* " + UE4XcodeHelperTarget.DisplayName + " */" + ProjectFileGenerator.NewLine);
			}
			XcodeProjectFileContent.Append (
				"\t\t\t);" + ProjectFileGenerator.NewLine +
				"\t\t};" + ProjectFileGenerator.NewLine +
				"/* End PBXProject section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine
			);

			string PreprocessorDefinitionsString = "\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine;
			foreach (string Definition in PreprocessorDefinitions)
			{
				PreprocessorDefinitionsString += "\t\t\t\t\t\"" + Definition + "\"," + ProjectFileGenerator.NewLine;
			}
			PreprocessorDefinitionsString += "\t\t\t\t\t\"MONOLITHIC_BUILD=1\"," + ProjectFileGenerator.NewLine;
			PreprocessorDefinitionsString += "\t\t\t\t);" + ProjectFileGenerator.NewLine;

			string HeaderSearchPaths = "\t\t\t\tUSER_HEADER_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine;
			foreach (string Path in IncludeDirectories)
			{
				HeaderSearchPaths += "\t\t\t\t\t\"" + Path + "\"," + ProjectFileGenerator.NewLine;
			}
			HeaderSearchPaths += "\t\t\t\t);" + ProjectFileGenerator.NewLine;

			XcodeProjectFileContent.Append ("/* Begin XCBuildConfiguration section */" + ProjectFileGenerator.NewLine);
			AppendBuildConfig (ref XcodeProjectFileContent, UE4ProjectTarget, HeaderSearchPaths, PreprocessorDefinitionsString);
			foreach (var target in ProjectTargets)
			{
				if(target == UE4ProjectTarget || target == UE4XcodeHelperTarget)
				{
					continue;
				}
				AppendBuildConfig (ref XcodeProjectFileContent, target);
			}
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				AppendBuildConfig (ref XcodeProjectFileContent, UE4XcodeHelperTarget);
			}
			XcodeProjectFileContent.Append ("/* End XCBuildConfiguration section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);

			XcodeProjectFileContent.Append ("/* Begin XCConfigurationList section */" + ProjectFileGenerator.NewLine);
			//AppendConfigList (ref XcodeProjectFileContent, UE4ProjectTarget);
			foreach (var target in ProjectTargets)
			{
				if(target == UE4XcodeHelperTarget)
				{
					continue;
				}
				AppendConfigList (ref XcodeProjectFileContent, target);
			}
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				AppendConfigList (ref XcodeProjectFileContent, UE4XcodeHelperTarget);
			}
			XcodeProjectFileContent.Append ("/* End XCConfigurationList section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);

			XcodeProjectFileContent.Append (
				"\t};" + ProjectFileGenerator.NewLine +
				"\trootObject = " + UE4ProjectTarget.Guid + " /* Project object */;" + ProjectFileGenerator.NewLine +
				"}" + ProjectFileGenerator.NewLine);

			string PathBranch = "";
			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				PathBranch = "/Engine/Intermediate/IOS";
			}
			var XcodeProjectPath = MasterProjectRelativePath + PathBranch + "/" + MasterProjectName + ".xcodeproj";
			if (bGeneratingGameProjectFiles && GameProjectName == "UE4Game" && ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				XcodeProjectPath = UnrealBuildTool.GetUProjectPath () + PathBranch + "/" + MasterProjectName + ".xcodeproj";
			}
			if (!Directory.Exists (XcodeProjectPath))
			{
				Directory.CreateDirectory (XcodeProjectPath);
			}

			// load the existing project
			string InnerProjectFileName = XcodeProjectPath + "/project.pbxproj";
			string ExistingProjectContents = "";
			if (File.Exists(InnerProjectFileName))
			{
				ExistingProjectContents = File.ReadAllText(InnerProjectFileName);
			}

			// compare it to the new project
			string NewProjectContents = XcodeProjectFileContent.ToString();
			if (ExistingProjectContents != NewProjectContents)
			{
				Log.TraceInformation("Saving updated project file...");
				File.WriteAllText(InnerProjectFileName, NewProjectContents);
			}
			else
			{
				Log.TraceInformation("Skipping project file write, as it didn't change...");
			}

			// write scheme files for targets
			foreach (var Target in ProjectTargets) 
			{
				if(Target == UE4ProjectTarget || Target == UE4XcodeHelperTarget)
					continue;

				if (UnrealBuildTool.HasUProjectFile() && Target.TargetName.StartsWith(Path.GetFileNameWithoutExtension(UnrealBuildTool.GetUProjectFile())))
				{
					if (Target.TargetName.EndsWith("Editor"))
					{
						WriteSchemeFile(XcodeProjectPath, Target, ".app", "UE4Editor", new List<string>(new string[] { "&quot;" + UnrealBuildTool.GetUProjectFile() + "&quot;" }));
					}
					else
					{
						string ProjectBinariesDir = UnrealBuildTool.GetUProjectPath() + "/Binaries/Mac/";
						WriteSchemeFile(XcodeProjectPath, Target, ".app", ProjectBinariesDir + Target.TargetName);
					}
				}
				else if(Target.TargetName.EndsWith("Game"))
				{
					string ExeBaseName = Target.TargetName;
					List<string> Args = null;
					if (Target.TargetPlatform == UnrealTargetPlatform.Mac)
					{
						Args = new List<string>(new string[] { Target.TargetName });
						ExeBaseName = "UE4";
					}

					WriteSchemeFile(XcodeProjectPath, Target, ".app", ExeBaseName, Args);
				}
				else if(Target.TargetName.EndsWith("Editor") && Target.TargetName != "UE4Editor")
				{
					string GameName = Target.TargetName.Replace("Editor", "");
					WriteSchemeFile(XcodeProjectPath, Target, ".app", "UE4Editor", new List<string>(new string[] { GameName }));
				}
				else if(TargetsThatNeedApp.Contains(Target.TargetName) || (Target.TargetPlatform == UnrealTargetPlatform.IOS))
				{
					WriteSchemeFile(XcodeProjectPath, Target, ".app");
				}
				else
				{
					WriteSchemeFile(XcodeProjectPath, Target);
				}
			}

			return true;
		}

		// always seed the random number the same, so multiple runs of the generator will generate the same project
		static Random Rand = new Random(0);

		/**
		 * Make a random Guid string usable by Xcode (24 characters exactly)
		 */
		public static string MakeXcodeGuid()
		{
			string Guid = "";

			byte[] Randoms = new byte[12];
			Rand.NextBytes(Randoms);
			for (int i = 0; i < 12; i++)
			{
				Guid += Randoms[i].ToString("X2");
			}

			return Guid;
		}

        private void WriteReferenceGroups(ref string PBXFileReferenceSection, ref Dictionary<string, XcodeFileGroup> Groups)
        {
            string RelativePath = "../../../";

            foreach (XcodeFileGroup Group in Groups.Values)
            {
                Group.bReference = true;
                string GroupPath = RelativePath + Group.GroupPath;

                // Add File reference.
                PBXFileReferenceSection += "\t\t" + Group.GroupGuid + " /* " + Group.GroupName + " */ = {"
                                            + "isa = PBXFileReference; lastKnownFileType = folder; "
                                            + "name = " + Group.GroupName + "; "
																						+ "path = " + Utils.CleanDirectorySeparators(GroupPath, '/') + "; sourceTree = \"<group>\"; };" + ProjectFileGenerator.NewLine;
            }
        }

		private void PopulateTargets(List<string> GamePaths, List<XcodeProjectTarget> ProjectTargets, List<XcodeContainerItemProxy> ContainerItemProxies, List<XcodeTargetDependency> TargetDependencies, XcodeProjectTarget ProjectTarget, List<XcodeFramework> Frameworks)
		{
			foreach (string TargetPath in GamePaths)
			{
				string TargetName = Path.GetFileName(TargetPath);
				TargetName = TrimTargetCs(TargetName);
				if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
				{
					bool WantProjectFileForTarget = true;
					if( bGeneratingGameProjectFiles || bGeneratingRocketProjectFiles )
					{
						bool IsEngineTarget = false;

						// Check to see if this is an Engine target.  That is, the target is located under the "Engine" folder
						string TargetFileRelativeToEngineDirectory = Utils.MakePathRelativeTo( TargetPath, Path.Combine( EngineRelativePath ), AlwaysTreatSourceAsDirectory: false );
						if( !TargetFileRelativeToEngineDirectory.StartsWith( ".." ) && !Path.IsPathRooted( TargetFileRelativeToEngineDirectory ) )
						{
							// This is an engine target
							IsEngineTarget = true;
						}

						if( IsEngineTarget )
						{
							if (!bAlwaysIncludeEngineModules)
							{
								// We were asked to exclude engine modules from the generated projects
								WantProjectFileForTarget = false;
							}
							if (bGeneratingGameProjectFiles && this.GameProjectName == TargetName)
							{
								WantProjectFileForTarget = true;
							}
						}
					}

					if( WantProjectFileForTarget )
					{
						string MacTargetRun = "";
						string IOSTargetRun = " (Run)";
						string IOSTargetBuild = " (Build)";
						if (ProjectFilePlatform.HasFlag(XcodeProjectFilePlatform.All))
						{
							MacTargetRun = " - Mac";
							IOSTargetRun = " - iOS (Run)";
							IOSTargetBuild = " - iOS (Build)";
						}

						string TargetFilePath;
						var Target = new TargetInfo(UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);
						var TargetRulesObject = RulesCompiler.CreateTargetRules( TargetName, Target, false, out TargetFilePath );
						List<UnrealTargetPlatform> SupportedPlatforms = new List<UnrealTargetPlatform>();
						TargetRulesObject.GetSupportedPlatforms(ref SupportedPlatforms);
						LinkEnvironmentConfiguration LinkConfiguration = new LinkEnvironmentConfiguration();
						CPPEnvironmentConfiguration CPPConfiguration = new CPPEnvironmentConfiguration();
						TargetRulesObject.SetupGlobalEnvironment(Target, ref LinkConfiguration, ref CPPConfiguration);

						if (!LinkConfiguration.bIsBuildingConsoleApplication)
						{
							TargetsThatNeedApp.Add(TargetName);
						}

						if (ProjectFilePlatform.HasFlag(XcodeProjectFilePlatform.Mac) && SupportedPlatforms.Contains(UnrealTargetPlatform.Mac))
						{
							XcodeProjectTarget MacBuildTarget = new XcodeProjectTarget(TargetName + MacTargetRun, TargetName, "Legacy");
							ProjectTargets.Add(MacBuildTarget);
						}

						if (ProjectFilePlatform.HasFlag(XcodeProjectFilePlatform.iOS) && SupportedPlatforms.Contains(UnrealTargetPlatform.IOS))
						{
							if (bGeneratingRocketProjectFiles && TargetName == "UE4Game")
							{
								// Generate Framework references.
								List<XcodeFrameworkRef> FrameworkRefs = new List<XcodeFrameworkRef>();
								foreach (XcodeFramework Framework in Frameworks)
								{
									FrameworkRefs.Add(new XcodeFrameworkRef(Framework));
								}

								XcodeProjectTarget IOSDeployTarget = new XcodeProjectTarget(TargetName + IOSTargetRun, TargetName, "Native", TargetName + ".app", UnrealTargetPlatform.IOS, null, true, FrameworkRefs);
								ProjectTargets.Add(IOSDeployTarget);
							}
							else
							{
								XcodeProjectTarget IOSBuildTarget = new XcodeProjectTarget(TargetName + IOSTargetBuild, TargetName, "Legacy", "", UnrealTargetPlatform.IOS);
								XcodeContainerItemProxy ContainerProxy = new XcodeContainerItemProxy(ProjectTarget.Guid, IOSBuildTarget.Guid, IOSBuildTarget.DisplayName);
								XcodeTargetDependency TargetDependency = new XcodeTargetDependency(IOSBuildTarget.DisplayName, IOSBuildTarget.Guid, ContainerProxy.Guid);
								XcodeProjectTarget IOSDeployTarget = new XcodeProjectTarget(TargetName + IOSTargetRun, TargetName, "Native", TargetName + ".app", UnrealTargetPlatform.IOS, new List<XcodeTargetDependency>() { TargetDependency }, true);
								ProjectTargets.Add(IOSBuildTarget);
								ProjectTargets.Add(IOSDeployTarget);
								ContainerItemProxies.Add(ContainerProxy);
								TargetDependencies.Add(TargetDependency);
							}
						}
					}
				}
				else // when building on PC, only the code signing targets are needed
				{
					string TargetFilePath;
					TargetRules TargetRulesObject = RulesCompiler.CreateTargetRules(TargetName, new TargetInfo(UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Development), false, out TargetFilePath);

					if (TargetRulesObject.SupportsPlatform(UnrealTargetPlatform.IOS))
					{
						// Generate Framework references.
						List<XcodeFrameworkRef> FrameworkRefs = new List<XcodeFrameworkRef>();
						foreach (XcodeFramework Framework in Frameworks)
						{
							FrameworkRefs.Add(new XcodeFrameworkRef(Framework));
						}

						XcodeProjectTarget IOSDeployTarget = new XcodeProjectTarget(TargetName + "_RunIOS", TargetName, "Native", TargetName + ".app", UnrealTargetPlatform.IOS, null, true, FrameworkRefs);
						ProjectTargets.Add(IOSDeployTarget);
					}
				}
			}
		}

		private void AddPreGeneratedProject (ref string PBXBuildFileSection, ref string PBXFileReferenceSection,
		                                     ref string PBXSourcesBuildPhaseSection, ref Dictionary<string, XcodeFileGroup> Groups,
		                                     string ProjectPath)
		{
			var ProjectFileName = Utils.MakePathRelativeTo( ProjectPath, MasterProjectRelativePath );
			var XcodeProject = new XcodeProjectFile( ProjectFileName) ;
			string ProjectDirectory = Path.GetDirectoryName(ProjectPath);
			XcodeProject.AddFilesToProject(SourceFileSearch.FindFiles(new List<string>() { ProjectDirectory }, ExcludeNoRedistFiles: bExcludeNoRedistFiles, SubdirectoryNamesToExclude:new List<string>() { "obj" }, SearchSubdirectories:true, IncludePrivateSourceCode:true), null);
			XcodeProject.GenerateSectionsContents(ref PBXBuildFileSection, ref PBXFileReferenceSection, ref PBXSourcesBuildPhaseSection, ref Groups);
		}

		[Flags]
		public enum XcodeProjectFilePlatform
		{
			Mac = 1 << 0,
			iOS = 1 << 1,
			All = Mac | iOS
		}

		/// Which platforms we should generate targets for
		static public XcodeProjectFilePlatform ProjectFilePlatform = XcodeProjectFilePlatform.All;

		/// <summary>
		/// Configures project generator based on command-line options
		/// </summary>
		/// <param name="Arguments">Arguments passed into the program</param>
		/// <param name="IncludeAllPlatforms">True if all platforms should be included</param>
		protected override void ConfigureProjectFileGeneration( String[] Arguments, ref bool IncludeAllPlatforms )
		{
			// Call parent implementation first
			base.ConfigureProjectFileGeneration( Arguments, ref IncludeAllPlatforms );
			ProjectFilePlatform = IncludeAllPlatforms ? XcodeProjectFilePlatform.All : XcodeProjectFilePlatform.Mac;
		}

		private string UE4CmdLineGuid;
		private string MainGroupGuid;
		private string ProductRefGroupGuid;
        private string FrameworkGroupGuid;
		private string UE4CmdLineRunMFileRefGuid;

		// List of targets that have a user interface.
		static List<string> TargetsThatNeedApp = new List<string>();
	}
}
