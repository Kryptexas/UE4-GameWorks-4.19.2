// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Linq;
using System.Linq;
using System.Text;

namespace UnrealBuildTool
{
	/// <summary>
	/// Info needed to make a file a member of specific group
	/// </summary>
	class XcodeSourceFile : ProjectFile.SourceFile
	{
		/// <summary>
		/// Constructor
		/// </summary>
		public XcodeSourceFile(string InitFilePath, string InitRelativeBaseFolder)
			: base(InitFilePath, InitRelativeBaseFolder)
		{
			FileGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			FileRefGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
		}

		/// <summary>
		/// File Guid for use in Xcode project
		/// </summary>
		public string FileGuid
		{
			get;
			private set;
		}

		public void ReplaceGuids(string NewFileGuid, string NewFileRefGuid)
		{
			FileGuid = NewFileGuid;
			FileRefGuid = NewFileRefGuid;
		}

		/// <summary>
		/// File reference Guid for use in Xcode project
		/// </summary>
		public string FileRefGuid
		{
			get;
			private set;
		}
	}

	/// <summary>
	/// Represents a group of files shown in Xcode's project navigator as a folder
	/// </summary>
	class XcodeFileGroup
	{
		public XcodeFileGroup(string InName, string InPath, bool InIsReference = false)
		{
			GroupName = InName;
			GroupPath = InPath;
			GroupGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			bIsReference = InIsReference;
		}

		public string GroupGuid;
		public string GroupName;
		public string GroupPath;
		public Dictionary<string, XcodeFileGroup> Children = new Dictionary<string, XcodeFileGroup>();
		public List<XcodeSourceFile> Files = new List<XcodeSourceFile>();
		public bool bIsReference;
	}

	class XcodeProjectFile : ProjectFile
	{
		Dictionary<string, XcodeFileGroup> Groups = new Dictionary<string, XcodeFileGroup>();

		/// <summary>
		/// Constructs a new project file object
		/// </summary>
		/// <param name="InitFilePath">The path to the project file on disk</param>
		public XcodeProjectFile(string InitFilePath)
			: base(InitFilePath)
		{
		}

		public override string ToString()
		{
			return Path.GetFileNameWithoutExtension(ProjectFilePath);
		}

		/** Gets Xcode file category based on its extension */
		private string GetFileCategory(string Extension)
		{
			// @todo Mac: Handle more categories
			switch (Extension)
			{
			case ".framework":
				return "Frameworks";
			default:
				return "Sources";
			}
		}

		/** Gets Xcode file type based on its extension */
		private string GetFileType(string Extension)
		{
			// @todo Mac: Handle more file types
			switch (Extension)
			{
			case ".c":
			case ".m":
				return "sourcecode.c.objc";
			case ".cc":
			case ".cpp":
			case ".mm":
				return "sourcecode.cpp.objcpp";
			case ".h":
			case ".inl":
			case ".pch":
				return "sourcecode.c.h";
			case ".framework":
				return "wrapper.framework";
			case ".plist":
				return "text.plist.xml";
			case ".png":
				return "image.png";
			case ".icns":
				return "image.icns";
			default:
				return "file.text";
			}
		}

		/** Returns true if Extension is a known extension for files containing source code */
		private bool IsSourceCode(string Extension)
		{
			return Extension == ".c" || Extension == ".cc" || Extension == ".cpp" || Extension == ".m" || Extension == ".mm";
		}

		private bool ShouldIncludeFileInBuildPhaseSection(XcodeSourceFile SourceFile)
		{
			string FileExtension = Path.GetExtension(SourceFile.FilePath);

			if (IsSourceCode(FileExtension))
			{
				foreach (string PlatformName in Enum.GetNames(typeof(UnrealTargetPlatform)))
				{
					string AltName = PlatformName == "Win32" || PlatformName == "Win64" ? "windows" : PlatformName.ToLower();
					if ((SourceFile.FilePath.ToLower().Contains("/" + PlatformName.ToLower() + "/") || SourceFile.FilePath.ToLower().Contains("/" + AltName + "/"))
						&& PlatformName != "Mac" && !SourceFile.FilePath.Contains("MetalRHI"))
					{
						// Build phase is used for indexing only and indexing currently works only with files that can be compiled for Mac, so skip files for other platforms
						return false;
					}
				}

				return true;
			}

			return false;
		}

		/**
		 * Returns a project navigator group to which the file should belong based on its path.
		 * Creates a group tree if it doesn't exist yet.
		 */
		public XcodeFileGroup FindGroupByRelativePath(ref Dictionary<string, XcodeFileGroup> Groups, string RelativePath)
		{
			string[] Parts = RelativePath.Split(Path.DirectorySeparatorChar);
			string CurrentPath = "";
			Dictionary<string, XcodeFileGroup> CurrentParent = Groups;

			foreach (string Part in Parts)
			{
				XcodeFileGroup CurrentGroup;

				if (CurrentPath != "")
				{
					CurrentPath += Path.DirectorySeparatorChar;
				}

				CurrentPath += Part;

				if (!CurrentParent.ContainsKey(CurrentPath))
				{
					CurrentGroup = new XcodeFileGroup(Path.GetFileName(CurrentPath), CurrentPath);
					CurrentParent.Add(CurrentPath, CurrentGroup);
				}
				else
				{
					CurrentGroup = CurrentParent[CurrentPath];
				}

				if (CurrentPath == RelativePath)
				{
					return CurrentGroup;
				}

				CurrentParent = CurrentGroup.Children;
			}

			return null;
		}

		/// <summary>
		/// Allocates a generator-specific source file object
		/// </summary>
		/// <param name="InitFilePath">Path to the source file on disk</param>
		/// <param name="InitProjectSubFolder">Optional sub-folder to put the file in.  If empty, this will be determined automatically from the file's path relative to the project file</param>
		/// <returns>The newly allocated source file object</returns>
		public override SourceFile AllocSourceFile(string InitFilePath, string InitProjectSubFolder)
		{
			if (Path.GetFileName(InitFilePath).StartsWith("."))
			{
				return null;
			}
			return new XcodeSourceFile(InitFilePath, InitProjectSubFolder);
		}

		/**
		 * Generates bodies of all sections that contain a list of source files plus a dictionary of project navigator groups.
		 */
		private void GenerateSectionsWithSourceFiles(StringBuilder PBXBuildFileSection, StringBuilder PBXFileReferenceSection, StringBuilder PBXSourcesBuildPhaseSection, string TargetAppGuid, string TargetName)
		{
			foreach (var CurSourceFile in SourceFiles)
			{
				XcodeSourceFile SourceFile = CurSourceFile as XcodeSourceFile;
				string FileName = Path.GetFileName(SourceFile.FilePath);
				string FileExtension = Path.GetExtension(FileName);
				string FilePath = Utils.MakePathRelativeTo(SourceFile.FilePath, Path.GetDirectoryName(ProjectFilePath));
				string FilePathMac = Utils.CleanDirectorySeparators(FilePath, '/');

				if (IsGeneratedProject)
				{
					PBXBuildFileSection.Append(string.Format("\t\t{0} /* {1} in {2} */ = {{isa = PBXBuildFile; fileRef = {3} /* {1} */; }};" + ProjectFileGenerator.NewLine,
						SourceFile.FileGuid,
						FileName,
						GetFileCategory(FileExtension),
						SourceFile.FileRefGuid));
				}

				PBXFileReferenceSection.Append(string.Format("\t\t{0} /* {1} */ = {{isa = PBXFileReference; lastKnownFileType = {2}; name = \"{1}\"; path = \"{3}\"; sourceTree = SOURCE_ROOT; }};" + ProjectFileGenerator.NewLine,
					SourceFile.FileRefGuid,
					FileName,
					GetFileType(FileExtension),
					FilePathMac));

				if (ShouldIncludeFileInBuildPhaseSection(SourceFile))
				{
					PBXSourcesBuildPhaseSection.Append("\t\t\t\t" + SourceFile.FileGuid + " /* " + FileName + " in Sources */," + ProjectFileGenerator.NewLine);
				}

				var ProjectRelativeSourceFile = Utils.MakePathRelativeTo(CurSourceFile.FilePath, Path.GetDirectoryName(ProjectFilePath));
				string RelativeSourceDirectory = Path.GetDirectoryName(ProjectRelativeSourceFile);
				// Use the specified relative base folder
				if (CurSourceFile.RelativeBaseFolder != null)	// NOTE: We are looking for null strings, not empty strings!
				{
					RelativeSourceDirectory = Path.GetDirectoryName(Utils.MakePathRelativeTo(CurSourceFile.FilePath, CurSourceFile.RelativeBaseFolder));
				}
				XcodeFileGroup Group = FindGroupByRelativePath(ref Groups, RelativeSourceDirectory);
				if (Group != null)
				{
					Group.Files.Add(SourceFile);
				}
			}

			PBXFileReferenceSection.Append(string.Format("\t\t{0} /* {1} */ = {{isa = PBXFileReference; explicitFileType = wrapper.application; path = {1}; sourceTree = BUILT_PRODUCTS_DIR; }};" + ProjectFileGenerator.NewLine, TargetAppGuid, TargetName));
		}

		private void AppendGroup(XcodeFileGroup Group, StringBuilder Content, bool bFilesOnly)
		{
			if (!Group.bIsReference)
			{
				if (!bFilesOnly)
				{
					Content.Append(string.Format("\t\t{0} = {{{1}", Group.GroupGuid, ProjectFileGenerator.NewLine));
					Content.Append("\t\t\tisa = PBXGroup;" + ProjectFileGenerator.NewLine);
					Content.Append("\t\t\tchildren = (" + ProjectFileGenerator.NewLine);

					foreach (XcodeFileGroup ChildGroup in Group.Children.Values)
					{
						Content.Append(string.Format("\t\t\t\t{0} /* {1} */,{2}", ChildGroup.GroupGuid, ChildGroup.GroupName, ProjectFileGenerator.NewLine));
					}
				}

				foreach (XcodeSourceFile File in Group.Files)
				{
					Content.Append(string.Format("\t\t\t\t{0} /* {1} */,{2}", File.FileRefGuid, Path.GetFileName(File.FilePath), ProjectFileGenerator.NewLine));
				}

				if (!bFilesOnly)
				{
					Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
					Content.Append("\t\t\tname = \"" + Group.GroupName + "\";" + ProjectFileGenerator.NewLine);
					Content.Append("\t\t\tpath = \"" + Group.GroupPath + "\";" + ProjectFileGenerator.NewLine);
					Content.Append("\t\t\tsourceTree = \"<group>\";" + ProjectFileGenerator.NewLine);
					Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

					foreach (XcodeFileGroup ChildGroup in Group.Children.Values)
					{
						AppendGroup(ChildGroup, Content, bFilesOnly: false);
					}
				}
			}
		}

		private void AppendBuildFileSection(StringBuilder Content, StringBuilder SectionContent)
		{
			Content.Append("/* Begin PBXBuildFile section */" + ProjectFileGenerator.NewLine);
			Content.Append(SectionContent);
			Content.Append("/* End PBXBuildFile section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendFileReferenceSection(StringBuilder Content, StringBuilder SectionContent)
		{
			Content.Append("/* Begin PBXFileReference section */" + ProjectFileGenerator.NewLine);
			Content.Append(SectionContent);
			Content.Append("/* End PBXFileReference section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendSourcesBuildPhaseSection(StringBuilder Content, StringBuilder SectionContent, string SourcesBuildPhaseGuid)
		{
			Content.Append("/* Begin PBXSourcesBuildPhase section */" + ProjectFileGenerator.NewLine);
			Content.Append(string.Format("\t\t{0} = {{{1}", SourcesBuildPhaseGuid, ProjectFileGenerator.NewLine));
			Content.Append("\t\t\tisa = PBXSourcesBuildPhase;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildActionMask = 2147483647;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tfiles = (" + ProjectFileGenerator.NewLine);
			Content.Append(SectionContent);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\trunOnlyForDeploymentPostprocessing = 0;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("/* End PBXSourcesBuildPhase section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendGroupSection(StringBuilder Content, string MainGroupGuid, string ProductRefGroupGuid, string TargetAppGuid, string TargetName)
		{
			Content.Append("/* Begin PBXGroup section */" + ProjectFileGenerator.NewLine);

			// Main group
			Content.Append(string.Format("\t\t{0} = {{{1}", MainGroupGuid, ProjectFileGenerator.NewLine));
			Content.Append("\t\t\tisa = PBXGroup;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tchildren = (" + ProjectFileGenerator.NewLine);

			foreach (XcodeFileGroup Group in Groups.Values)
			{
				if (!string.IsNullOrEmpty(Group.GroupName))
				{
					Content.Append(string.Format("\t\t\t\t{0} /* {1} */,{2}", Group.GroupGuid, Group.GroupName, ProjectFileGenerator.NewLine));
				}
			}

			if (Groups.ContainsKey(""))
			{
				AppendGroup(Groups[""], Content, bFilesOnly: true);
			}

			Content.Append(string.Format("\t\t\t\t{0} /* Products */,{1}", ProductRefGroupGuid, ProjectFileGenerator.NewLine));
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tsourceTree = \"<group>\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

			// Sources groups
			foreach (XcodeFileGroup Group in Groups.Values)
			{
				if (Group.GroupName != "")
				{
					AppendGroup(Group, Content, bFilesOnly: false);
				}
			}

			// Products group
			Content.Append(string.Format("\t\t{0} /* Products */ = {{{1}", ProductRefGroupGuid, ProjectFileGenerator.NewLine));
			Content.Append("\t\t\tisa = PBXGroup;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tchildren = (" + ProjectFileGenerator.NewLine);
			Content.Append(string.Format("\t\t\t\t{0} /* {1} */,{2}", TargetAppGuid, TargetName, ProjectFileGenerator.NewLine));
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = Products;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tsourceTree = \"<group>\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

			Content.Append("/* End PBXGroup section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendLegacyTargetSection(StringBuilder Content, string TargetName, string TargetGuid, string TargetBuildConfigGuid, string UProjectPath)
		{
            MacToolChain Toolchain = UEToolChain.GetPlatformToolChain(CPPTargetPlatform.Mac) as MacToolChain;
            string UE4Dir = Toolchain.ConvertPath(Path.GetFullPath(Directory.GetCurrentDirectory() + "../../.."));
			string BuildToolPath = UE4Dir + "/Engine/Build/BatchFiles/Mac/" + (XcodeProjectFileGenerator.bGeneratingRocketProjectFiles ? "Rocket" : "") + "Build.sh";

			Content.Append("/* Begin PBXLegacyTarget section */" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t" + TargetGuid + " /* " + TargetName + " */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = PBXLegacyTarget;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildArgumentsString = \"$(ACTION) $(UE_BUILD_TARGET_NAME) $(PLATFORM_NAME) $(UE_BUILD_TARGET_CONFIG)" + (string.IsNullOrEmpty(UProjectPath) ? "" : " \\\"" + UProjectPath + "\\\"") + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildConfigurationList = "  + TargetBuildConfigGuid + " /* Build configuration list for PBXLegacyTarget \"" + TargetName + "\" */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildPhases = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildToolPath = \"" + BuildToolPath + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildWorkingDirectory = \"" + UE4Dir + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tdependencies = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tpassBuildSettingsInEnvironment = 1;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductName = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

			Content.Append("/* End PBXLegacyTarget section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendRunTargetSection(StringBuilder Content, string TargetName, string TargetGuid, string TargetBuildConfigGuid, string TargetDependencyGuid, string TargetAppGuid)
		{
			Content.Append("/* Begin PBXNativeTarget section */" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t" + TargetGuid + " /* " + TargetName + " */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = PBXNativeTarget;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildConfigurationList = "  + TargetBuildConfigGuid + " /* Build configuration list for PBXNativeTarget \"" + TargetName + "\" */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildPhases = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tdependencies = (" + ProjectFileGenerator.NewLine);
			if (!XcodeProjectFileGenerator.bGeneratingRunIOSProject)
			{
				Content.Append("\t\t\t\t" + TargetDependencyGuid + " /* PBXTargetDependency */," + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tpassBuildSettingsInEnvironment = 1;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductName = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductReference = \"" + TargetAppGuid + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductType = \"com.apple.product-type.application\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

			Content.Append("/* End PBXNativeTarget section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendIndexTargetSection(StringBuilder Content, string TargetName, string TargetGuid, string TargetBuildConfigGuid, string SourcesBuildPhaseGuid)
		{
			Content.Append("/* Begin PBXNativeTarget section */" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t" + TargetGuid + " /* " + TargetName + " */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = PBXNativeTarget;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildConfigurationList = "  + TargetBuildConfigGuid + " /* Build configuration list for PBXNativeTarget \"" + TargetName + "\" */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildPhases = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\t" + SourcesBuildPhaseGuid + " /* Sources */," + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tdependencies = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tpassBuildSettingsInEnvironment = 1;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductName = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductType = \"com.apple.product-type.library.static\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

			Content.Append("/* End PBXNativeTarget section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendProjectSection(StringBuilder Content, string TargetName, string TargetGuid, string BuildTargetName, string BuildTargetGuid, string IndexTargetName, string IndexTargetGuid, string MainGroupGuid, string ProductRefGroupGuid, string ProjectGuid, string ProjectBuildConfigGuid)
		{
			Content.Append("/* Begin PBXProject section */" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t" + ProjectGuid + " /* Project object */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = PBXProject;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tattributes = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tLastUpgradeCheck = 0700;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tORGANIZATIONNAME = \"Epic Games, Inc.\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildConfigurationList = " + ProjectBuildConfigGuid + " /* Build configuration list for PBXProject \"" + TargetName + "\" */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tcompatibilityVersion = \"Xcode 3.2\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tdevelopmentRegion = English;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\thasScannedForEncodings = 0;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tknownRegions = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\ten" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tmainGroup = " + MainGroupGuid + ";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproductRefGroup = " + ProductRefGroupGuid + ";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tprojectDirPath = \"\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tprojectRoot = \"\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\ttargets = (" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t" + TargetGuid + " /* " + TargetName + " */," + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t" + BuildTargetGuid + " /* " + BuildTargetName + " */," + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t" + IndexTargetGuid + " /* " + IndexTargetName + " */," + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);

			Content.Append("/* End PBXProject section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendContainerItemProxySection(StringBuilder Content, string TargetName, string TargetGuid, string TargetProxyGuid, string ProjectGuid)
		{
			Content.Append("/* Begin PBXContainerItemProxy section */" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t" + TargetProxyGuid + " /* PBXContainerItemProxy */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = PBXContainerItemProxy;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tcontainerPortal = " + ProjectGuid + " /* Project object */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tproxyType = 1;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tremoteGlobalIDString = " + TargetGuid + ";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tremoteInfo = \"" + TargetName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("/* End PBXContainerItemProxy section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendTargetDependencySection(StringBuilder Content, string TargetName, string TargetGuid, string TargetDependencyGuid, string TargetProxyGuid)
		{
			Content.Append("/* Begin PBXTargetDependency section */" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t" + TargetDependencyGuid + " /* PBXTargetDependency */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = PBXTargetDependency;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\ttarget = " + TargetGuid + " /* " + TargetName + " */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\ttargetProxy = " + TargetProxyGuid + " /* PBXContainerItemProxy */;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("/* End PBXTargetDependency section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendProjectBuildConfiguration(StringBuilder Content, string ConfigName, string ConfigGuid)
		{
			Content.Append("\t\t" + ConfigGuid + " /* \"" + ConfigName + "\" */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t\t\tGCC_PREPROCESSOR_DEFINITIONS = (" + ProjectFileGenerator.NewLine);
			foreach (var Definition in IntelliSensePreprocessorDefinitions)
			{
				Content.Append("\t\t\t\t\t\"" + Definition.Replace("\"", "") + "\"," + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t\t\t\"MONOLITHIC_BUILD=1\"," + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\t);" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t\t\tHEADER_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine);
			foreach (var Path in IntelliSenseSystemIncludeSearchPaths)
			{
				Content.Append("\t\t\t\t\t\"" + Path + "\"," + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t\t);" + ProjectFileGenerator.NewLine);

			Content.Append("\t\t\t\tUSER_HEADER_SEARCH_PATHS = (" + ProjectFileGenerator.NewLine);
			foreach (var Path in IntelliSenseIncludeSearchPaths)
			{
				Content.Append("\t\t\t\t\t\"" + Path + "\"," + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t\t);" + ProjectFileGenerator.NewLine);

			if (ConfigName == "Debug")
			{
				Content.Append("\t\t\t\tONLY_ACTIVE_ARCH = YES;" + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t\tALWAYS_SEARCH_USER_PATHS = NO;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tCLANG_CXX_LANGUAGE_STANDARD = \"c++0x\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tGCC_ENABLE_CPP_RTTI = NO;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tGCC_WARN_CHECK_SWITCH_STATEMENTS = NO;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tUSE_HEADERMAP = NO;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = \"" + ConfigName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
		}

		private void AppendNativeTargetBuildConfiguration(StringBuilder Content, XcodeBuildConfig Config, string ConfigGuid, bool bIsAGame, string GameProjectPath)
		{
			bool bMacOnly = true;
			if (Config.ProjectTarget.TargetRules != null && XcodeProjectFileGenerator.ProjectFilePlatform.HasFlag(XcodeProjectFileGenerator.XcodeProjectFilePlatform.iOS))
			{
				var SupportedPlatforms = new List<UnrealTargetPlatform>();
				Config.ProjectTarget.TargetRules.GetSupportedPlatforms(ref SupportedPlatforms);
				if (SupportedPlatforms.Contains(UnrealTargetPlatform.IOS))
				{
					bMacOnly = false;
				}
			}

			Content.Append("\t\t" + ConfigGuid + " /* \"" + Config.DisplayName + "\" */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine);

			MacPlatform MacBuildPlat = UEBuildPlatform.GetBuildPlatform(UnrealTargetPlatform.Mac) as MacPlatform;
			MacBuildPlat.SetUpProjectEnvironment(UnrealTargetPlatform.Mac);

            MacToolChain Toolchain = UEToolChain.GetPlatformToolChain(CPPTargetPlatform.Mac) as MacToolChain;
            string UE4Dir = Toolchain.ConvertPath(Path.GetFullPath(Directory.GetCurrentDirectory() + "../../.."));
			string MacExecutableFileName = Path.GetFileName(Config.MacExecutablePath);

			if (bMacOnly)
			{
				Content.Append("\t\t\t\tVALID_ARCHS = \"x86_64\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tSUPPORTED_PLATFORMS = \"macosx\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tPRODUCT_NAME = \"" + MacExecutableFileName + "\";" + ProjectFileGenerator.NewLine);
                Content.Append("\t\t\t\tCONFIGURATION_BUILD_DIR = \"" + Toolchain.ConvertPath(Path.GetDirectoryName(Config.MacExecutablePath)) + "\";" + ProjectFileGenerator.NewLine);
			}
			else
			{
				IOSPlatform IOSBuildPlat = UEBuildPlatform.GetBuildPlatform(UnrealTargetPlatform.IOS) as IOSPlatform;
				IOSBuildPlat.SetUpProjectEnvironment(UnrealTargetPlatform.IOS);

				Content.Append("\t\t\t\tVALID_ARCHS = \"x86_64 arm64 armv7 armv7s\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tSUPPORTED_PLATFORMS = \"macosx iphoneos\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\t\"PRODUCT_NAME[sdk=macosx*]\" = \"" + MacExecutableFileName + "\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\t\"PRODUCT_NAME[sdk=iphoneos*]\" = \"" + Config.BuildTarget + "\";" + ProjectFileGenerator.NewLine); // @todo: change to Path.GetFileName(Config.IOSExecutablePath) when we stop using payload
				Content.Append("\t\t\t\tIPHONEOS_DEPLOYMENT_TARGET = " + IOSBuildPlat.GetRunTimeVersion() + ";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\t\"CODE_SIGN_IDENTITY[sdk=iphoneos*]\" = \"iPhone Developer\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tTARGETED_DEVICE_FAMILY = \"" + IOSBuildPlat.GetRunTimeDevices() + "\";" + ProjectFileGenerator.NewLine);
                Content.Append("\t\t\t\t\"CONFIGURATION_BUILD_DIR[sdk=macosx*]\" = \"" + Toolchain.ConvertPath(Path.GetDirectoryName(Config.MacExecutablePath)) + "\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\t\"SDKROOT[arch=x86_64]\" = macosx;" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\t\"SDKROOT[arch=arm*]\" = iphoneos;" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tINFOPLIST_OUTPUT_FORMAT = xml;" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\t\"PROVISIONING_PROFILE[sdk=iphoneos*]\" = \"\";" + ProjectFileGenerator.NewLine);

				bool bIsUE4Game = Config.BuildTarget.Equals("UE4Game", StringComparison.InvariantCultureIgnoreCase);
				bool bIsUE4Client = Config.BuildTarget.Equals("UE4Client", StringComparison.InvariantCultureIgnoreCase);

                string GamePath = string.IsNullOrEmpty(GameProjectPath) ? null : Toolchain.ConvertPath(Path.GetDirectoryName(GameProjectPath));

				string IOSInfoPlistPath = null;
				string MacInfoPlistPath = null;
				if (bIsUE4Game)
				{
					IOSInfoPlistPath = UE4Dir + "/Engine/Intermediate/IOS/" + Config.BuildTarget + "-Info.plist";
					MacInfoPlistPath = UE4Dir + "/Engine/Intermediate/Mac/" + MacExecutableFileName + "-Info.plist";
					Content.Append("\t\t\t\t\"CONFIGURATION_BUILD_DIR[sdk=iphoneos*]\" = \"" + UE4Dir + "/Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
				}
				else if (bIsUE4Client)
				{
					IOSInfoPlistPath = UE4Dir + "/Engine/Intermediate/IOS/UE4Game-Info.plist";
					MacInfoPlistPath = UE4Dir + "/Engine/Intermediate/Mac/" + MacExecutableFileName + "-Info.plist";
					Content.Append("\t\t\t\t\"CONFIGURATION_BUILD_DIR[sdk=iphoneos*]\" = \"" + UE4Dir + "/Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
				}
				else if (bIsAGame)
				{
					IOSInfoPlistPath = GamePath + "/Intermediate/IOS/" + Config.BuildTarget + "-Info.plist";
					MacInfoPlistPath = GamePath + "/Intermediate/Mac/" + MacExecutableFileName + "-Info.plist";
					Content.Append("\t\t\t\t\"CONFIGURATION_BUILD_DIR[sdk=iphoneos*]\" = \"" + GamePath + "/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
				}
				else
				{
					if (string.IsNullOrEmpty(GamePath))
					{
						IOSInfoPlistPath = UE4Dir + "/Engine/Intermediate/IOS/" + Config.BuildTarget + "-Info.plist";
						MacInfoPlistPath = UE4Dir + "/Engine/Intermediate/Mac/" + MacExecutableFileName + "-Info.plist";
					}
					else
					{
						IOSInfoPlistPath = GamePath + "/Intermediate/IOS/" + Config.BuildTarget + "-Info.plist";
						MacInfoPlistPath = GamePath + "/Intermediate/Mac/" + MacExecutableFileName + "-Info.plist";
					}
					Content.Append("\t\t\t\t\"CONFIGURATION_BUILD_DIR[sdk=iphoneos*]\" = \"" + UE4Dir + "/Engine/Binaries/IOS/Payload\";" + ProjectFileGenerator.NewLine);
				}

				if (XcodeProjectFileGenerator.bGeneratingRunIOSProject)
				{
					Content.Append("\t\t\t\tINFOPLIST_FILE = \"" + IOSInfoPlistPath + "\";" + ProjectFileGenerator.NewLine);
				}
				else
				{
					Content.Append("\t\t\t\t\"INFOPLIST_FILE[sdk=macosx*]\" = \"" + MacInfoPlistPath + "\";" + ProjectFileGenerator.NewLine);
					Content.Append("\t\t\t\t\"INFOPLIST_FILE[sdk=iphoneos*]\" = \"" + IOSInfoPlistPath + "\";" + ProjectFileGenerator.NewLine);
				}

				// Prepare a temp Info.plist file so Xcode has some basic info about the target immediately after opening the project.
				// This is needed for the target to pass the settings validation before code signing. UBT will overwrite this plist file later, with proper contents.
				if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Mac)
				{
					bool bCreateMacInfoPlist = !File.Exists(MacInfoPlistPath);
					bool bCreateIOSInfoPlist = !File.Exists(IOSInfoPlistPath);
					if (bCreateMacInfoPlist || bCreateIOSInfoPlist)
					{
						string ProjectPath = GamePath;
						string EngineDir = UE4Dir + "/Engine";
						string GameName = Config.BuildTarget;
						if (string.IsNullOrEmpty(ProjectPath))
						{
							ProjectPath = EngineDir;
						}
						if (bIsUE4Game)
						{
							ProjectPath = EngineDir;
							GameName = "UE4Game";
						}

						if (bCreateMacInfoPlist)
						{
							Directory.CreateDirectory(Path.GetDirectoryName(MacInfoPlistPath));
							Mac.UEDeployMac.GeneratePList(ProjectPath, bIsUE4Game, GameName, Config.BuildTarget, EngineDir, MacExecutableFileName);
						}
						if (bCreateIOSInfoPlist)
						{
							Directory.CreateDirectory(Path.GetDirectoryName(IOSInfoPlistPath));
							IOS.UEDeployIOS.GeneratePList(ProjectPath, bIsUE4Game, GameName, Config.BuildTarget, EngineDir, ProjectPath + "/Binaries/IOS/Payload");
						}
					}
				}
			}
			Content.Append("\t\t\t\tMACOSX_DEPLOYMENT_TARGET = " + MacToolChain.MacOSVersion + ";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tSDKROOT = macosx;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tGCC_PRECOMPILE_PREFIX_HEADER = YES;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tGCC_PREFIX_HEADER = \"" + UE4Dir + "/Engine/Source/Editor/UnrealEd/Public/UnrealEd.h\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = \"" + Config.DisplayName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
		}

		private void AppendLegacyTargetBuildConfiguration(StringBuilder Content, XcodeBuildConfig Config, string ConfigGuid)
		{
			bool bMacOnly = true;
			if (Config.ProjectTarget.TargetRules != null && XcodeProjectFileGenerator.ProjectFilePlatform.HasFlag(XcodeProjectFileGenerator.XcodeProjectFilePlatform.iOS))
			{
				var SupportedPlatforms = new List<UnrealTargetPlatform>();
				Config.ProjectTarget.TargetRules.GetSupportedPlatforms(ref SupportedPlatforms);
				if (SupportedPlatforms.Contains(UnrealTargetPlatform.IOS))
				{
					bMacOnly = false;
				}
			}

			Content.Append("\t\t" + ConfigGuid + " /* \"" + Config.DisplayName + "\" */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = XCBuildConfiguration;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildSettings = {" + ProjectFileGenerator.NewLine);
			if (bMacOnly)
			{
				Content.Append("\t\t\t\tVALID_ARCHS = \"x86_64\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tSUPPORTED_PLATFORMS = \"macosx\";" + ProjectFileGenerator.NewLine);
			}
			else
			{
				Content.Append("\t\t\t\tVALID_ARCHS = \"x86_64 arm64 armv7 armv7s\";" + ProjectFileGenerator.NewLine);
				Content.Append("\t\t\t\tSUPPORTED_PLATFORMS = \"macosx iphoneos\";" + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t\tUE_BUILD_TARGET_NAME = \"" + Config.BuildTarget + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t\tUE_BUILD_TARGET_CONFIG = \"" + Config.BuildConfig + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t};" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tname = \"" + Config.DisplayName + "\";" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
		}

		private void AppendXCBuildConfigurationSection(StringBuilder Content, Dictionary<string, XcodeBuildConfig> ProjectBuildConfigs, Dictionary<string, XcodeBuildConfig> TargetBuildConfigs,
			Dictionary<string, XcodeBuildConfig> BuildTargetBuildConfigs, Dictionary<string, XcodeBuildConfig> IndexTargetBuildConfigs, bool bIsAGame, string GameProjectPath)
		{
			Content.Append("/* Begin XCBuildConfiguration section */" + ProjectFileGenerator.NewLine);

			foreach (var Config in ProjectBuildConfigs)
			{
				AppendProjectBuildConfiguration(Content, Config.Value.DisplayName, Config.Key);
			}

			foreach (var Config in TargetBuildConfigs)
			{
				AppendNativeTargetBuildConfiguration(Content, Config.Value, Config.Key, bIsAGame, GameProjectPath);
			}

			foreach (var Config in BuildTargetBuildConfigs)
			{
				AppendLegacyTargetBuildConfiguration(Content, Config.Value, Config.Key);
			}

			foreach (var Config in IndexTargetBuildConfigs)
			{
				AppendNativeTargetBuildConfiguration(Content, Config.Value, Config.Key, bIsAGame, GameProjectPath);
			}

			Content.Append("/* End XCBuildConfiguration section */" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);
		}

		private void AppendXCConfigurationList(StringBuilder Content, string TypeName, string TargetName, string ConfigListGuid, Dictionary<string, XcodeBuildConfig> BuildConfigs)
		{
			Content.Append("\t\t" + ConfigListGuid + " /* Build configuration list for " + TypeName + " \"" + TargetName + "\" */ = {" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tisa = XCConfigurationList;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tbuildConfigurations = (" + ProjectFileGenerator.NewLine);
			foreach (var Config in BuildConfigs)
			{
				Content.Append("\t\t\t\t" + Config.Key + " /* \"" + Config.Value.DisplayName + "\" */," + ProjectFileGenerator.NewLine);
			}
			Content.Append("\t\t\t);" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tdefaultConfigurationIsVisible = 0;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\tdefaultConfigurationName = Development;" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t};" + ProjectFileGenerator.NewLine);
		}

		private void AppendXCConfigurationListSection(StringBuilder Content, string TargetName, string BuildTargetName, string IndexTargetName, string ProjectConfigListGuid,
			Dictionary<string, XcodeBuildConfig> ProjectBuildConfigs, string TargetConfigListGuid, Dictionary<string, XcodeBuildConfig> TargetBuildConfigs,
			string BuildTargetConfigListGuid, Dictionary<string, XcodeBuildConfig> BuildTargetBuildConfigs,
			string IndexTargetConfigListGuid, Dictionary<string, XcodeBuildConfig> IndexTargetBuildConfigs)
		{
			Content.Append("/* Begin XCConfigurationList section */" + ProjectFileGenerator.NewLine);

			AppendXCConfigurationList(Content, "PBXProject", TargetName, ProjectConfigListGuid, ProjectBuildConfigs);
			AppendXCConfigurationList(Content, "PBXLegacyTarget", BuildTargetName, BuildTargetConfigListGuid, BuildTargetBuildConfigs);
			AppendXCConfigurationList(Content, "PBXNativeTarget", TargetName, TargetConfigListGuid, TargetBuildConfigs);
			AppendXCConfigurationList(Content, "PBXNativeTarget", IndexTargetName, IndexTargetConfigListGuid, IndexTargetBuildConfigs);

			Content.Append("/* End XCConfigurationList section */" + ProjectFileGenerator.NewLine);
		}

		public struct XcodeBuildConfig
		{
			public XcodeBuildConfig(string InDisplayName, string InBuildTarget, string InMacExecutablePath, string InIOSExecutablePath, ProjectTarget InProjectTarget, UnrealTargetConfiguration InBuildConfig)
			{
				DisplayName = InDisplayName;
				MacExecutablePath = InMacExecutablePath;
				IOSExecutablePath = InIOSExecutablePath;
				BuildTarget = InBuildTarget;
				ProjectTarget = InProjectTarget;
				BuildConfig = InBuildConfig;
			}

			public string DisplayName;
			public string MacExecutablePath;
			public string IOSExecutablePath;
			public string BuildTarget;
			public ProjectTarget ProjectTarget;
			public UnrealTargetConfiguration BuildConfig;
		};

		private List<XcodeBuildConfig> GetSupportedBuildConfigs(List<UnrealTargetPlatform> Platforms, List<UnrealTargetConfiguration> Configurations)
		{
			var BuildConfigs = new List<XcodeBuildConfig>();

			string ProjectName = Path.GetFileNameWithoutExtension(ProjectFilePath);

			foreach (var Configuration in Configurations)
			{
				if (UnrealBuildTool.IsValidConfiguration(Configuration))
				{
					foreach (var Platform in Platforms)
					{
						if (UnrealBuildTool.IsValidPlatform(Platform) && (Platform == UnrealTargetPlatform.Mac || Platform == UnrealTargetPlatform.IOS)) // @todo support other platforms
						{
							var BuildPlatform = UEBuildPlatform.GetBuildPlatform(Platform, true);
							if ((BuildPlatform != null) && (BuildPlatform.HasRequiredSDKsInstalled() == SDKStatus.Valid))
							{
								// Now go through all of the target types for this project
								if (ProjectTargets.Count == 0)
								{
									throw new BuildException("Expecting at least one ProjectTarget to be associated with project '{0}' in the TargetProjects list ", ProjectFilePath);
								}

								foreach (var ProjectTarget in ProjectTargets)
								{
									if (MSBuildProjectFile.IsValidProjectPlatformAndConfiguration(ProjectTarget, Platform, Configuration))
									{
										// Figure out if this is a monolithic build
										bool bShouldCompileMonolithic = BuildPlatform.ShouldCompileMonolithicBinary(Platform);
										bShouldCompileMonolithic |= ProjectTarget.TargetRules.ShouldCompileMonolithic(Platform, Configuration);

										var ConfigName = Configuration.ToString();
										if (ProjectTarget.TargetRules.ConfigurationName != "Game" && ProjectTarget.TargetRules.ConfigurationName != "Program")
										{
											ConfigName += " " + ProjectTarget.TargetRules.ConfigurationName;
										}

										if (BuildConfigs.Where(Config => Config.DisplayName == ConfigName).ToList().Count == 0)
										{
											string TargetName = Path.GetFileNameWithoutExtension(Path.GetFileNameWithoutExtension(ProjectTarget.TargetFilePath));

											// Get the output directory
											string EngineRootDirectory = Path.GetFullPath(ProjectFileGenerator.EngineRelativePath);
											string RootDirectory = EngineRootDirectory;
											if ((TargetRules.IsAGame(ProjectTarget.TargetRules.Type) || ProjectTarget.TargetRules.Type == TargetRules.TargetType.Server) && bShouldCompileMonolithic && !ProjectTarget.TargetRules.bOutputToEngineBinaries)
											{
												if (UnrealBuildTool.HasUProjectFile() && Utils.IsFileUnderDirectory(ProjectTarget.TargetFilePath, UnrealBuildTool.GetUProjectPath()))
												{
													RootDirectory = Path.GetFullPath(UnrealBuildTool.GetUProjectPath());
												}
												else
												{
													string UnrealProjectPath = UProjectInfo.GetProjectFilePath(ProjectName);
													if (!String.IsNullOrEmpty(UnrealProjectPath))
													{
														RootDirectory = Path.GetDirectoryName(Path.GetFullPath(UnrealProjectPath));
													}
												}
											}

											if(ProjectTarget.TargetRules.Type == TargetRules.TargetType.Program && !ProjectTarget.TargetRules.bOutputToEngineBinaries)
											{
												string UnrealProjectPath = UProjectInfo.GetProjectForTarget(TargetName);
												if (!String.IsNullOrEmpty(UnrealProjectPath))
												{
													RootDirectory = Path.GetDirectoryName(Path.GetFullPath(UnrealProjectPath));
												}
											}

											// Get the output directory
											string OutputDirectory = Path.Combine(RootDirectory, "Binaries");

											string ExeName = TargetName;
											if (!bShouldCompileMonolithic && ProjectTarget.TargetRules.Type != TargetRules.TargetType.Program)
											{
												// Figure out what the compiled binary will be called so that we can point the IDE to the correct file
												string TargetConfigurationName = ProjectTarget.TargetRules.ConfigurationName;
												if (TargetConfigurationName != TargetRules.TargetType.Game.ToString() && TargetConfigurationName != TargetRules.TargetType.Program.ToString())
												{
													ExeName = "UE4" + TargetConfigurationName;
												}
											}

											string MacExecutableName = UEBuildTarget.MakeBinaryFileName(ExeName, UnrealTargetPlatform.Mac, (ExeName == "UE4Editor" && Configuration == UnrealTargetConfiguration.DebugGame) ? UnrealTargetConfiguration.Development : Configuration, ProjectTarget.TargetRules.UndecoratedConfiguration, UEBuildBinaryType.Executable);
											string IOSExecutableName = MacExecutableName.Replace("-Mac-", "-IOS-");
											BuildConfigs.Add(new XcodeBuildConfig(ConfigName, TargetName, Path.Combine(OutputDirectory, "Mac", MacExecutableName), Path.Combine(OutputDirectory, "IOS", IOSExecutableName), ProjectTarget, Configuration));
										}
									}
								}
							}
						}
					}
				}
			}

			return BuildConfigs;
		}

		private void WriteSchemeFile(string TargetName, string TargetGuid, string BuildTargetGuid, string IndexTargetGuid, bool bHasEditorConfiguration, string GameProjectPath)
		{
			string DefaultConfiguration = bHasEditorConfiguration && !XcodeProjectFileGenerator.bGeneratingRunIOSProject ? "Development Editor" : "Development";

			var Content = new StringBuilder();

			Content.Append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + ProjectFileGenerator.NewLine);
			Content.Append("<Scheme" + ProjectFileGenerator.NewLine);
			Content.Append("   LastUpgradeVersion = \"0710\"" + ProjectFileGenerator.NewLine);
			Content.Append("   version = \"1.3\">" + ProjectFileGenerator.NewLine);
			Content.Append("   <BuildAction" + ProjectFileGenerator.NewLine);
			Content.Append("      parallelizeBuildables = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("      buildImplicitDependencies = \"YES\">" + ProjectFileGenerator.NewLine);
			Content.Append("      <BuildActionEntries>" + ProjectFileGenerator.NewLine);
			Content.Append("         <BuildActionEntry" + ProjectFileGenerator.NewLine);
			Content.Append("            buildForTesting = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("            buildForRunning = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("            buildForProfiling = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("            buildForArchiving = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("            buildForAnalyzing = \"YES\">" + ProjectFileGenerator.NewLine);
			Content.Append("            <BuildableReference" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintIdentifier = \"" + TargetGuid + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableName = \"" + TargetName + ".app\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintName = \"" + TargetName + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               ReferencedContainer = \"container:" + TargetName + ".xcodeproj\">" + ProjectFileGenerator.NewLine);
			Content.Append("            </BuildableReference>" + ProjectFileGenerator.NewLine);
			Content.Append("         </BuildActionEntry>" + ProjectFileGenerator.NewLine);
			Content.Append("      </BuildActionEntries>" + ProjectFileGenerator.NewLine);
			Content.Append("   </BuildAction>" + ProjectFileGenerator.NewLine);
			Content.Append("   <TestAction" + ProjectFileGenerator.NewLine);
			Content.Append("      buildConfiguration = \"" + DefaultConfiguration + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("      selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\"" + ProjectFileGenerator.NewLine);
			Content.Append("      selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\"" + ProjectFileGenerator.NewLine);
			Content.Append("      shouldUseLaunchSchemeArgsEnv = \"YES\">" + ProjectFileGenerator.NewLine);
			Content.Append("      <Testables>" + ProjectFileGenerator.NewLine);
			Content.Append("      </Testables>" + ProjectFileGenerator.NewLine);
			Content.Append("      <MacroExpansion>" + ProjectFileGenerator.NewLine);
			Content.Append("            <BuildableReference" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintIdentifier = \"" + TargetGuid + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableName = \"" + TargetName + ".app\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintName = \"" + TargetName + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               ReferencedContainer = \"container:" + TargetName + ".xcodeproj\">" + ProjectFileGenerator.NewLine);
			Content.Append("            </BuildableReference>" + ProjectFileGenerator.NewLine);
			Content.Append("      </MacroExpansion>" + ProjectFileGenerator.NewLine);
			Content.Append("      <AdditionalOptions>" + ProjectFileGenerator.NewLine);
			Content.Append("      </AdditionalOptions>" + ProjectFileGenerator.NewLine);
			Content.Append("   </TestAction>" + ProjectFileGenerator.NewLine);
			Content.Append("   <LaunchAction" + ProjectFileGenerator.NewLine);
			Content.Append("      buildConfiguration = \"" + DefaultConfiguration + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("      selectedDebuggerIdentifier = \"Xcode.DebuggerFoundation.Debugger.LLDB\"" + ProjectFileGenerator.NewLine);
			Content.Append("      selectedLauncherIdentifier = \"Xcode.DebuggerFoundation.Launcher.LLDB\"" + ProjectFileGenerator.NewLine);
			Content.Append("      launchStyle = \"0\"" + ProjectFileGenerator.NewLine);
			Content.Append("      useCustomWorkingDirectory = \"NO\"" + ProjectFileGenerator.NewLine);
			Content.Append("      ignoresPersistentStateOnLaunch = \"NO\"" + ProjectFileGenerator.NewLine);
			Content.Append("      debugDocumentVersioning = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("      debugServiceExtension = \"internal\"" + ProjectFileGenerator.NewLine);
			Content.Append("      allowLocationSimulation = \"YES\">" + ProjectFileGenerator.NewLine);
			Content.Append("      <BuildableProductRunnable" + ProjectFileGenerator.NewLine);
			Content.Append("         runnableDebuggingMode = \"0\">" + ProjectFileGenerator.NewLine);
			Content.Append("            <BuildableReference" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintIdentifier = \"" + TargetGuid + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableName = \"" + TargetName + ".app\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintName = \"" + TargetName + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               ReferencedContainer = \"container:" + TargetName + ".xcodeproj\">" + ProjectFileGenerator.NewLine);
			Content.Append("            </BuildableReference>" + ProjectFileGenerator.NewLine);
			Content.Append("      </BuildableProductRunnable>" + ProjectFileGenerator.NewLine);
			if (bHasEditorConfiguration && TargetName != "UE4")
			{
				Content.Append("      <CommandLineArguments>" + ProjectFileGenerator.NewLine);
				if (IsForeignProject)
				{
					Content.Append("         <CommandLineArgument" + ProjectFileGenerator.NewLine);
                    Content.Append("            argument = \"&quot;" + GameProjectPath + "&quot;\"" + ProjectFileGenerator.NewLine);
					Content.Append("            isEnabled = \"YES\">" + ProjectFileGenerator.NewLine);
					Content.Append("         </CommandLineArgument>" + ProjectFileGenerator.NewLine);
				}
				else
				{
					Content.Append("         <CommandLineArgument" + ProjectFileGenerator.NewLine);
					Content.Append("            argument = \"" + TargetName + "\"" + ProjectFileGenerator.NewLine);
					Content.Append("            isEnabled = \"YES\">" + ProjectFileGenerator.NewLine);
					Content.Append("         </CommandLineArgument>" + ProjectFileGenerator.NewLine);
				}
				Content.Append("      </CommandLineArguments>" + ProjectFileGenerator.NewLine);
			}
			Content.Append("      <AdditionalOptions>" + ProjectFileGenerator.NewLine);
			Content.Append("      </AdditionalOptions>" + ProjectFileGenerator.NewLine);
			Content.Append("   </LaunchAction>" + ProjectFileGenerator.NewLine);
			Content.Append("   <ProfileAction" + ProjectFileGenerator.NewLine);
			Content.Append("      buildConfiguration = \"" + DefaultConfiguration + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("      shouldUseLaunchSchemeArgsEnv = \"YES\"" + ProjectFileGenerator.NewLine);
			Content.Append("      savedToolIdentifier = \"\"" + ProjectFileGenerator.NewLine);
			Content.Append("      useCustomWorkingDirectory = \"NO\"" + ProjectFileGenerator.NewLine);
			Content.Append("      debugDocumentVersioning = \"YES\">" + ProjectFileGenerator.NewLine);
			Content.Append("      <BuildableProductRunnable" + ProjectFileGenerator.NewLine);
			Content.Append("         runnableDebuggingMode = \"0\">" + ProjectFileGenerator.NewLine);
			Content.Append("            <BuildableReference" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableIdentifier = \"primary\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintIdentifier = \"" + TargetGuid + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BuildableName = \"" + TargetName + ".app\"" + ProjectFileGenerator.NewLine);
			Content.Append("               BlueprintName = \"" + TargetName + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("               ReferencedContainer = \"container:" + TargetName + ".xcodeproj\">" + ProjectFileGenerator.NewLine);
			Content.Append("            </BuildableReference>" + ProjectFileGenerator.NewLine);
			Content.Append("      </BuildableProductRunnable>" + ProjectFileGenerator.NewLine);
			Content.Append("   </ProfileAction>" + ProjectFileGenerator.NewLine);
			Content.Append("   <AnalyzeAction" + ProjectFileGenerator.NewLine);
			Content.Append("      buildConfiguration = \"" + DefaultConfiguration + "\">" + ProjectFileGenerator.NewLine);
			Content.Append("   </AnalyzeAction>" + ProjectFileGenerator.NewLine);
			Content.Append("   <ArchiveAction" + ProjectFileGenerator.NewLine);
			Content.Append("      buildConfiguration = \"" + DefaultConfiguration + "\"" + ProjectFileGenerator.NewLine);
			Content.Append("      revealArchiveInOrganizer = \"YES\">" + ProjectFileGenerator.NewLine);
			Content.Append("   </ArchiveAction>" + ProjectFileGenerator.NewLine);
			Content.Append("</Scheme>" + ProjectFileGenerator.NewLine);

			string SchemesDir = ProjectFilePath + "/xcshareddata/xcschemes";
			if (!Directory.Exists(SchemesDir))
			{
				Directory.CreateDirectory(SchemesDir);
			}

			string SchemeFilePath = SchemesDir + "/" + TargetName + ".xcscheme";
            File.WriteAllText(SchemeFilePath, Content.ToString(), new UTF8Encoding());

			Content.Clear();

			Content.Append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + ProjectFileGenerator.NewLine);
			Content.Append("<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">" + ProjectFileGenerator.NewLine);
			Content.Append("<plist version=\"1.0\">" + ProjectFileGenerator.NewLine);
			Content.Append("<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t<key>SchemeUserState</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<key>" + TargetName + ".xcscheme_^#shared#^_</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<key>orderHint</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<integer>" + SchemeOrderHint.ToString() + "</integer>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t<key>SuppressBuildableAutocreation</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<key>" + TargetGuid + "</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<key>primary</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<true/>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<key>" + BuildTargetGuid + "</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<key>primary</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<true/>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<key>" + IndexTargetGuid + "</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t<dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<key>primary</key>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t\t<true/>" + ProjectFileGenerator.NewLine);
			Content.Append("\t\t</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("\t</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("</dict>" + ProjectFileGenerator.NewLine);
			Content.Append("</plist>" + ProjectFileGenerator.NewLine);

			string ManagementFileDir = ProjectFilePath + "/xcuserdata/" + Environment.UserName + ".xcuserdatad/xcschemes";
			if (!Directory.Exists(ManagementFileDir))
			{
				Directory.CreateDirectory(ManagementFileDir);
			}

			string ManagementFilePath = ManagementFileDir + "/xcschememanagement.plist";
            File.WriteAllText(ManagementFilePath, Content.ToString(), new UTF8Encoding());

			SchemeOrderHint++;
		}

		/// Implements Project interface
		public override bool WriteProjectFile(List<UnrealTargetPlatform> InPlatforms, List<UnrealTargetConfiguration> InConfigurations)
		{
			bool bSuccess = true;

			var TargetName = Path.GetFileNameWithoutExtension(RelativeProjectFilePath);
			var TargetGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var TargetConfigListGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var TargetDependencyGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var TargetProxyGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var TargetAppGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var BuildTargetName = TargetName + "_Build";
			var BuildTargetGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var BuildTargetConfigListGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var IndexTargetName = TargetName + "_Index";
			var IndexTargetGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var IndexTargetConfigListGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var ProjectGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var ProjectConfigListGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var MainGroupGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var ProductRefGroupGuid = XcodeProjectFileGenerator.MakeXcodeGuid();
			var SourcesBuildPhaseGuid = XcodeProjectFileGenerator.MakeXcodeGuid();

			// Figure out all the desired configurations
			var BuildConfigs = GetSupportedBuildConfigs(InPlatforms, InConfigurations);
			if (BuildConfigs.Count == 0)
			{
				return true;
			}

			bool bIsAGame = false;
			string GameProjectPath = null;
			List<string> GameFolders = UEBuildTarget.DiscoverAllGameFolders();
			foreach (string GameFolder in GameFolders)
			{
				string UProjectPath = Path.Combine(GameFolder, TargetName + ".uproject");
				if (File.Exists(UProjectPath))
				{
					bIsAGame = true;
					GameProjectPath = Path.GetFullPath(UProjectPath);
					break;
				}
			}

			bool bHasEditorConfiguration = false;

			var ProjectBuildConfigs = new Dictionary<string, XcodeBuildConfig>();
			var TargetBuildConfigs = new Dictionary<string, XcodeBuildConfig>();
			var BuildTargetBuildConfigs = new Dictionary<string, XcodeBuildConfig>();
			var IndexTargetBuildConfigs = new Dictionary<string, XcodeBuildConfig>();
			foreach (var Config in BuildConfigs)
			{
				ProjectBuildConfigs[XcodeProjectFileGenerator.MakeXcodeGuid()] = Config;
				TargetBuildConfigs[XcodeProjectFileGenerator.MakeXcodeGuid()] = Config;
				BuildTargetBuildConfigs[XcodeProjectFileGenerator.MakeXcodeGuid()] = Config;
				IndexTargetBuildConfigs[XcodeProjectFileGenerator.MakeXcodeGuid()] = Config;

				if (Config.ProjectTarget.TargetRules.Type == TargetRules.TargetType.Editor)
				{
					bHasEditorConfiguration = true;
				}
			}

			var PBXBuildFileSection = new StringBuilder();
			var PBXFileReferenceSection = new StringBuilder();
			var PBXSourcesBuildPhaseSection = new StringBuilder();
			GenerateSectionsWithSourceFiles(PBXBuildFileSection, PBXFileReferenceSection, PBXSourcesBuildPhaseSection, TargetAppGuid, TargetName);

			var ProjectFileContent = new StringBuilder();

			ProjectFileContent.Append("// !$*UTF8*$!" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("{" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("\tarchiveVersion = 1;" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("\tclasses = {" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("\t};" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("\tobjectVersion = 46;" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("\tobjects = {" + ProjectFileGenerator.NewLine + ProjectFileGenerator.NewLine);

			AppendBuildFileSection(ProjectFileContent, PBXBuildFileSection);
			AppendFileReferenceSection(ProjectFileContent, PBXFileReferenceSection);
			AppendSourcesBuildPhaseSection(ProjectFileContent, PBXSourcesBuildPhaseSection, SourcesBuildPhaseGuid);
			AppendContainerItemProxySection(ProjectFileContent, BuildTargetName, BuildTargetGuid, TargetProxyGuid, ProjectGuid);
			if (!XcodeProjectFileGenerator.bGeneratingRunIOSProject)
			{
				AppendTargetDependencySection(ProjectFileContent, BuildTargetName, BuildTargetGuid, TargetDependencyGuid, TargetProxyGuid);
			}
			AppendGroupSection(ProjectFileContent, MainGroupGuid, ProductRefGroupGuid, TargetAppGuid, TargetName);
			AppendLegacyTargetSection(ProjectFileContent, BuildTargetName, BuildTargetGuid, BuildTargetConfigListGuid, GameProjectPath);
			AppendRunTargetSection(ProjectFileContent, TargetName, TargetGuid, TargetConfigListGuid, TargetDependencyGuid, TargetAppGuid);
			AppendIndexTargetSection(ProjectFileContent, IndexTargetName, IndexTargetGuid, IndexTargetConfigListGuid, SourcesBuildPhaseGuid);
			AppendProjectSection(ProjectFileContent, TargetName, TargetGuid, BuildTargetName, BuildTargetGuid, IndexTargetName, IndexTargetGuid, MainGroupGuid, ProductRefGroupGuid, ProjectGuid, ProjectConfigListGuid);
			AppendXCBuildConfigurationSection(ProjectFileContent, ProjectBuildConfigs, TargetBuildConfigs, BuildTargetBuildConfigs, IndexTargetBuildConfigs, bIsAGame, GameProjectPath);
			AppendXCConfigurationListSection(ProjectFileContent, TargetName, BuildTargetName, IndexTargetName, ProjectConfigListGuid, ProjectBuildConfigs,
				TargetConfigListGuid, TargetBuildConfigs, BuildTargetConfigListGuid, BuildTargetBuildConfigs, IndexTargetConfigListGuid, IndexTargetBuildConfigs);

			ProjectFileContent.Append("\t};" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("\trootObject = " + ProjectGuid + " /* Project object */;" + ProjectFileGenerator.NewLine);
			ProjectFileContent.Append("}" + ProjectFileGenerator.NewLine);

			if (bSuccess)
			{
				var PBXProjFilePath = ProjectFilePath + "/project.pbxproj";
				bSuccess = ProjectFileGenerator.WriteFileIfChanged(PBXProjFilePath, ProjectFileContent.ToString(), new UTF8Encoding());
			}

			if (bSuccess)
			{
				WriteSchemeFile(TargetName, TargetGuid, BuildTargetGuid, IndexTargetGuid, bHasEditorConfiguration, GameProjectPath);
			}

			return bSuccess;
		}

		static private int SchemeOrderHint = 0;
	}
}
