// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	/// <summary>
	/// Represents a folder within the master project (e.g. Visual Studio solution)
	/// </summary>
	class CMakefileFolder : MasterProjectFolder
	{
		/// <summary>
		/// Constructor
		/// </summary>
		public CMakefileFolder(ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName)
			: base(InitOwnerProjectFileGenerator, InitFolderName)
		{
		}
	}

	class CMakefileProjectFile : ProjectFile
	{
		public CMakefileProjectFile(FileReference InitFilePath)
			: base(InitFilePath)
		{
		}
	}
	/// <summary>
	/// CMakefile project file generator implementation
	/// </summary>
	class CMakefileGenerator : ProjectFileGenerator
	{
		/// <summary>
		/// Creates a new instance of the <see cref="CMakefileGenerator"/> class.
		/// </summary>
		public CMakefileGenerator(FileReference InOnlyGameProject)
			: base(InOnlyGameProject)
		{
		}

		/// <summary>
		/// Determines whether or not we should generate code completion data whenever possible.
		/// </summary>
		/// <returns><value>true</value> if we should generate code completion data; <value>false</value> otherwise.</returns>
		public override bool ShouldGenerateIntelliSenseData()
		{
			return true;
		}

		/// <summary>
		/// The file extension for this project file.
		/// </summary>
		public override string ProjectFileExtension
		{
			get
			{
				return ".txt";
			}
		}

		public string ProjectFileName
		{
			get
			{
				return "CMakeLists" + ProjectFileExtension;
			}
		}

		/// <summary>
		/// Writes the master project file (e.g. Visual Studio Solution file)
		/// </summary>
		/// <param name="UBTProject">The UnrealBuildTool project</param>
		/// <returns>True if successful</returns>
		protected override bool WriteMasterProjectFile(ProjectFile UBTProject)
		{
			return true;
		}

		private void AppendCleanedPathToList(StringBuilder List, String SourceFileRelativeToRoot, String FullName, String GameProjectPath)
		{
			if (!SourceFileRelativeToRoot.StartsWith("..") && !Path.IsPathRooted(SourceFileRelativeToRoot))
			{
				List.Append("\t\"${UE4_ROOT_PATH}/Engine/" + Utils.CleanDirectorySeparators(SourceFileRelativeToRoot, '/') + "\"\n");

			}
			else
			{
				if (String.IsNullOrEmpty(GameProjectName))
				{
					List.Append("\t\"" + Utils.CleanDirectorySeparators(SourceFileRelativeToRoot, '/').Substring(3) + "\"\n");
				}
				else
				{
					string RelativeGameSourcePath = Utils.MakePathRelativeTo(FullName, GameProjectPath);
					List.Append("\t\"${GAME_ROOT_PATH}/" + Utils.CleanDirectorySeparators(RelativeGameSourcePath, '/') + "\"\n");
				}
			}
		}

		private bool WriteCMakeLists()
		{
			string BuildCommand;
			const string CMakeSectionEnd = " )\n\n";

			var CMakefileContent = new StringBuilder();

			StringBuilder CMakeSourceFilesList = new StringBuilder("set(SOURCE_FILES \n");
			StringBuilder CMakeHeaderFilesList = new StringBuilder("set(HEADER_FILES \n");
			StringBuilder CMakeCSFilesList = new StringBuilder("set(CS_FILES \n");
			StringBuilder IncludeDirectoriesList = new StringBuilder("include_directories( \n");
			StringBuilder PreprocessorDefinitionsList = new StringBuilder("add_definitions( \n");
			// These only exist so that IDEs can find them, not used during building
			StringBuilder CMakeShaderFilesList = new StringBuilder("set(SHADER_FILES \n");
			StringBuilder CMakeConfigFilesList = new StringBuilder("set(CONFIG_FILES \n");

			string CMakeCC = "\n";
			string CMakeC = "\n";

			var CMakeGameRootPath = "";
			var CMakeUE4RootPath = "set(UE4_ROOT_PATH " + Utils.CleanDirectorySeparators(UnrealBuildTool.RootDirectory.FullName, '/') + ")\n";

			string GameProjectPath = "";
			string CMakeGameProjectFile = "";

			string HostArchitecture;
			switch (BuildHostPlatform.Current.Platform)
			{
				case UnrealTargetPlatform.Win64:
				{
					HostArchitecture = "Win64";
					BuildCommand = "set(BUILD cmd /c \"${UE4_ROOT_PATH}/Engine/Build/BatchFiles/Build.bat\")\n";
					break;
				}
				case UnrealTargetPlatform.Mac:
				{
					MacToolChainSettings Settings = new MacToolChainSettings(false);
					HostArchitecture = "Mac";
					BuildCommand = "set(BUILD cd \"${UE4_ROOT_PATH}\" && bash \"${UE4_ROOT_PATH}/Engine/Build/BatchFiles/" + HostArchitecture + "/Build.sh\")\n";
					CMakeCC = "set(CMAKE_CXX_COMPILER " + Settings.ToolchainDir + "clang++)\n";
					CMakeC = "set(CMAKE_C_COMPILER " + Settings.ToolchainDir + "clang)\n";
					break;
				}
				case UnrealTargetPlatform.Linux:
				{
					HostArchitecture = "Linux";
					BuildCommand = "set(BUILD cd \"${UE4_ROOT_PATH}\" && bash \"${UE4_ROOT_PATH}/Engine/Build/BatchFiles/" + HostArchitecture + "/Build.sh\")\n";
					string Compiler = LinuxCommon.WhichClang();
					if (String.IsNullOrEmpty(Compiler))
					{
						Compiler = LinuxCommon.WhichGcc();
					}
					// Should not be possible, but...
					if (String.IsNullOrEmpty(Compiler))
					{
						Compiler = "clang++";
					}
					CMakeCC = "set(CMAKE_CXX_COMPILER \"" + Compiler + "\")\n";
					string CCompiler = Compiler.Replace("++", "");
					CMakeC = "set(CMAKE_C_COMPILER \"" + CCompiler + "\")\n";
					break;
				}
				default:
				{
					throw new BuildException("ERROR: CMakefileGenerator does not support this platform");
				}
			}

			if (!String.IsNullOrEmpty(GameProjectName))
			{
				GameProjectPath = OnlyGameProject.Directory.FullName;

				CMakeGameRootPath = "set(GAME_ROOT_PATH \"" + Utils.CleanDirectorySeparators(OnlyGameProject.Directory.FullName, '/') + "\")\n";
				CMakeGameProjectFile = "set(GAME_PROJECT_FILE \"" + Utils.CleanDirectorySeparators(OnlyGameProject.FullName, '/') + "\")\n";
			}

			CMakefileContent.Append(
				"# Makefile generated by CMakefileGenerator.cs (v1.1)\n" +
				"# *DO NOT EDIT*\n\n" +
				"cmake_minimum_required (VERSION 2.6)\n" +
				"project (UE4)\n\n" +
				CMakeCC +
				CMakeC +
				CMakeUE4RootPath +
				CMakeGameProjectFile +
				BuildCommand +
				CMakeGameRootPath + "\n"
			);

			List<string> IncludeDirectories = new List<string>();
			List<string> PreprocessorDefinitions = new List<string>();

			foreach (var CurProject in GeneratedProjectFiles)
			{
				foreach (var IncludeSearchPath in CurProject.IntelliSenseIncludeSearchPaths)
				{
					string IncludeDirectory = GetIncludeDirectory(IncludeSearchPath, Path.GetDirectoryName(CurProject.ProjectFilePath.FullName));
					if (IncludeDirectory != null && !IncludeDirectories.Contains(IncludeDirectory))
					{
						if (IncludeDirectory.Contains(UnrealBuildTool.RootDirectory.FullName))
						{
							IncludeDirectories.Add(IncludeDirectory.Replace(UnrealBuildTool.RootDirectory.FullName, "${UE4_ROOT_PATH}"));
						}
						else
						{
							// If the path isn't rooted, then it is relative to the game root
							if (!Path.IsPathRooted(IncludeDirectory))
							{
								IncludeDirectories.Add("${GAME_ROOT_PATH}/" + IncludeDirectory);
							}
							else
							{
								// This is a rooted path like /usr/local/sometool/include
								IncludeDirectories.Add(IncludeDirectory);
							}
						}
					}
				}

				foreach (var PreProcessorDefinition in CurProject.IntelliSensePreprocessorDefinitions)
				{
					string Definition = PreProcessorDefinition;
					string AlternateDefinition = Definition.Contains("=0") ? Definition.Replace("=0", "=1") : Definition.Replace("=1", "=0");

					if (Definition.Equals("WITH_EDITORONLY_DATA=0") || Definition.Equals("WITH_DATABASE_SUPPORT=1"))
					{
						Definition = AlternateDefinition;
					}

					if (!PreprocessorDefinitions.Contains(Definition) &&
						!PreprocessorDefinitions.Contains(AlternateDefinition) &&
						!Definition.StartsWith("UE_ENGINE_DIRECTORY") &&
						!Definition.StartsWith("ORIGINAL_FILE_NAME"))
					{
						PreprocessorDefinitions.Add(Definition);
					}
				}
			}

			// Create SourceFiles, HeaderFiles, and ConfigFiles sections.
			var AllModuleFiles = DiscoverModules(FindGameProjects());
			foreach (FileReference CurModuleFile in AllModuleFiles)
			{
				var FoundFiles = SourceFileSearch.FindModuleSourceFiles(CurModuleFile);
				foreach (FileReference CurSourceFile in FoundFiles)
				{
					string SourceFileRelativeToRoot = CurSourceFile.MakeRelativeTo(UnrealBuildTool.EngineDirectory);

					// Exclude files/folders on a per-platform basis.
					if (!IsPathExcludedOnPlatform(SourceFileRelativeToRoot, BuildHostPlatform.Current.Platform))
					{
						if (SourceFileRelativeToRoot.EndsWith(".cpp"))
						{
							AppendCleanedPathToList(CMakeSourceFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".h"))
						{
							AppendCleanedPathToList(CMakeHeaderFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".cs"))
						{
							AppendCleanedPathToList(CMakeCSFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".usf") || SourceFileRelativeToRoot.EndsWith(".ush"))
						{
							AppendCleanedPathToList(CMakeShaderFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".ini"))
						{
							AppendCleanedPathToList(CMakeConfigFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
						}
					}
				}
			}

			foreach (string IncludeDirectory in IncludeDirectories)
			{
				IncludeDirectoriesList.Append("\t\"" + Utils.CleanDirectorySeparators(IncludeDirectory, '/') + "\"\n");
			}

			foreach (string PreprocessorDefinition in PreprocessorDefinitions)
			{
				PreprocessorDefinitionsList.Append("\t-D" + PreprocessorDefinition + "\n");
			}

			// Add Engine/Shaders files (game are added via modules)
			var EngineShaderFiles = SourceFileSearch.FindFiles(DirectoryReference.Combine(UnrealBuildTool.EngineDirectory, "Shaders"));
			foreach (FileReference CurSourceFile in EngineShaderFiles)
			{
				string SourceFileRelativeToRoot = CurSourceFile.MakeRelativeTo(UnrealBuildTool.EngineDirectory);
				if (SourceFileRelativeToRoot.EndsWith(".usf") || SourceFileRelativeToRoot.EndsWith(".ush"))
				{
					AppendCleanedPathToList(CMakeShaderFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
				}
			}

			// Add Engine/Config ini files (game are added via modules)
			var EngineConfigFiles = SourceFileSearch.FindFiles(DirectoryReference.Combine(UnrealBuildTool.EngineDirectory, "Config"));
			foreach (FileReference CurSourceFile in EngineConfigFiles)
			{
				string SourceFileRelativeToRoot = CurSourceFile.MakeRelativeTo(UnrealBuildTool.EngineDirectory);
				if (SourceFileRelativeToRoot.EndsWith(".ini"))
				{
					AppendCleanedPathToList(CMakeConfigFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath);
				}
			}

			// Add section end to section strings;
			CMakeSourceFilesList.Append(CMakeSectionEnd);
			CMakeHeaderFilesList.Append(CMakeSectionEnd);
			CMakeCSFilesList.Append(CMakeSectionEnd);
			IncludeDirectoriesList.Append(CMakeSectionEnd);
			PreprocessorDefinitionsList.Append(CMakeSectionEnd);

			CMakeShaderFilesList.Append(CMakeSectionEnd);
			CMakeConfigFilesList.Append(CMakeSectionEnd);

			// Append sections to the CMakeLists.txt file
			CMakefileContent.Append(CMakeSourceFilesList);
			CMakefileContent.Append(CMakeHeaderFilesList);
			CMakefileContent.Append(CMakeCSFilesList);
			CMakefileContent.Append(IncludeDirectoriesList);
			CMakefileContent.Append(PreprocessorDefinitionsList);
			if (bIncludeShaderSource)
			{
				CMakefileContent.Append(CMakeShaderFilesList);
                CMakefileContent.Append("set_source_files_properties(${SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)\n");
                CMakefileContent.Append("source_group(\"Shader Files\" REGULAR_EXPRESSION .*.usf)\n");
				CMakefileContent.Append("\n");
			}
			if (bIncludeConfigFiles)
			{
				CMakefileContent.Append(CMakeConfigFilesList);
				CMakefileContent.Append("set_source_files_properties(${CONFIG_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)\n");
				CMakefileContent.Append("source_group(\"Config Files\" REGULAR_EXPRESSION .*.ini)\n");
				CMakefileContent.Append("\n");
			}

			string CMakeProjectCmdArg = "";

			foreach (var Project in GeneratedProjectFiles)
			{
				foreach (var TargetFile in Project.ProjectTargets)
				{
					if (TargetFile.TargetFilePath == null)
					{
						continue;
					}

					var TargetName = TargetFile.TargetFilePath.GetFileNameWithoutAnyExtensions();       // Remove both ".cs" and ".

					foreach (UnrealTargetConfiguration CurConfiguration in Enum.GetValues(typeof(UnrealTargetConfiguration)))
					{
						if (CurConfiguration != UnrealTargetConfiguration.Unknown && CurConfiguration != UnrealTargetConfiguration.Development)
						{
							if (UnrealBuildTool.IsValidConfiguration(CurConfiguration) && !IsTargetExcluded(TargetName, CurConfiguration))
							{
								if (TargetName == GameProjectName || TargetName == (GameProjectName + "Editor"))
								{
									CMakeProjectCmdArg = "-project=\"${GAME_PROJECT_FILE}\"";
								}

								string ConfName = Enum.GetName(typeof(UnrealTargetConfiguration), CurConfiguration);
								CMakefileContent.Append(String.Format("add_custom_target({0}-{3}-{1} ${{BUILD}} {0} {3} {1} {2} $(ARGS))\n", TargetName, ConfName, CMakeProjectCmdArg, HostArchitecture));
							}
						}
					}

					if (!IsTargetExcluded(TargetName, UnrealTargetConfiguration.Development))
					{
						if (TargetName == GameProjectName || TargetName == (GameProjectName + "Editor"))
						{
							CMakeProjectCmdArg = "-project=\"${GAME_PROJECT_FILE}\"";
						}

						CMakefileContent.Append(String.Format("add_custom_target({0} ${{BUILD}} {0} {2} Development {1} $(ARGS) SOURCES ${{SOURCE_FILES}} ${{HEADER_FILES}} ${{CS_FILES}})\n\n", TargetName, CMakeProjectCmdArg, HostArchitecture));
					}

					CMakefileContent.Append(String.Format("add_custom_target({0} ${{BUILD}} {0} {2} Development {1} $(ARGS) SOURCES ${{SOURCE_FILES}} ${{HEADER_FILES}} ${{CS_FILES}})\n\n", TargetName, CMakeProjectCmdArg, HostArchitecture));
				}
			}

			// Append a dummy executable target and add all of our file types for searchability in IDEs
			CMakefileContent.AppendLine("add_executable(FakeTarget ${SOURCE_FILES} ${HEADER_FILES} ${CS_FILES} ${SHADER_FILES} ${CONFIG_FILES})");

			var FullFileName = Path.Combine(MasterProjectPath.FullName, ProjectFileName);

			bool writeSuccess = WriteFileIfChanged(FullFileName, CMakefileContent.ToString());
			return writeSuccess;
		}

		private static bool IsPathExcludedOnPlatform(string SourceFileRelativeToRoot, UnrealTargetPlatform targetPlatform)
		{
			switch (targetPlatform)
			{
				case UnrealTargetPlatform.Linux:
				{
					return IsPathExcludedOnLinux(SourceFileRelativeToRoot);
				}
				case UnrealTargetPlatform.Mac:
				{
					return IsPathExcludedOnMac(SourceFileRelativeToRoot);
				}
				case UnrealTargetPlatform.Win64:
				{
					return IsPathExcludedOnWindows(SourceFileRelativeToRoot);
				}
				default:
				{
					return false;
				}
			}
		}

		private static bool IsPathExcludedOnLinux(string SourceFileRelativeToRoot)
		{
			// minimal filtering as it is helpful to be able to look up symbols from other platforms
			return SourceFileRelativeToRoot.Contains("Source/ThirdParty/");
		}

		private static bool IsPathExcludedOnMac(string SourceFileRelativeToRoot)
		{
			return SourceFileRelativeToRoot.Contains("Source/ThirdParty/") ||
				SourceFileRelativeToRoot.Contains("/Windows/") ||
				SourceFileRelativeToRoot.Contains("/Linux/") ||
				SourceFileRelativeToRoot.Contains("/VisualStudioSourceCodeAccess/") ||
				SourceFileRelativeToRoot.Contains("/WmfMedia/") ||
				SourceFileRelativeToRoot.Contains("/WindowsDeviceProfileSelector/") ||
				SourceFileRelativeToRoot.Contains("/WindowsMoviePlayer/") ||
				SourceFileRelativeToRoot.Contains("/WinRT/");
		}

		private static bool IsPathExcludedOnWindows(string SourceFileRelativeToRoot)
		{
			// minimal filtering as it is helpful to be able to look up symbols from other platforms
			return SourceFileRelativeToRoot.Contains("Source\\ThirdParty\\");
		}

		private bool IsTargetExcluded(string TargetName, UnrealTargetConfiguration TargetConfig)
		{
			// Only do this level of filtering if we are trying to speed things up tremendously
			if (bCmakeMinimalTargets)
			{
				// Editor or game builds get all target configs
				// The game project editor or game get all configs
				if ((TargetName.StartsWith("UE4Editor") && !TargetName.StartsWith("UE4EditorServices")) ||
					TargetName.StartsWith("UE4Game") ||
					(!String.IsNullOrEmpty(GameProjectName) && TargetName.StartsWith(GameProjectName)))
				{
					return false;
				}
				// SCW & CRC are minimally included as just development builds
				else if (TargetConfig == UnrealTargetConfiguration.Development &&
					(TargetName.StartsWith("ShaderCompileWorker") || TargetName.StartsWith("CrashReportClient")))
				{
					return false;
				}
				return true;
			}
			return false;
		}

		/// Adds the include directory to the list, after converting it to relative to UE4 root
		private static string GetIncludeDirectory(string IncludeDir, string ProjectDir)
		{
			string FullProjectPath = Path.GetFullPath(MasterProjectPath.FullName);
			string FullPath = "";
			// Check for paths outside of both the engine and the project
			if (Path.IsPathRooted(IncludeDir) &&
				!IncludeDir.StartsWith(FullProjectPath) &&
				!IncludeDir.StartsWith(UnrealBuildTool.RootDirectory.FullName))
			{
				// Full path to a folder outside of project
				FullPath = IncludeDir;
			}
			else
			{
				FullPath = Path.GetFullPath(Path.Combine(ProjectDir, IncludeDir));
				if (!FullPath.StartsWith(UnrealBuildTool.RootDirectory.FullName))
				{
					FullPath = Utils.MakePathRelativeTo(FullPath, FullProjectPath);
				}
				FullPath = FullPath.TrimEnd('/');
			}
			return FullPath;
		}

		/// <summary>
		/// Writes a file that CLion uses to know what directories to exclude from indexing. This should speed up indexing
		/// </summary>
		protected void WriteCLionIgnoreDirs()
		{
			string CLionIngoreXml =
				"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" + Environment.NewLine +
				"<project version=\"4\">" + Environment.NewLine +
				  "\t<component name=\"CMakeWorkspace\" PROJECT_DIR=\"$PROJECT_DIR$\" />" + Environment.NewLine +
				  "\t<component name=\"CidrRootsConfiguration\">" + Environment.NewLine +
					"\t\t<excludeRoots>" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Binaries\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Build\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Content\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/DataTables\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/DerivedDataCache\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/FeaturePacks\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Intermediate\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/LocalBuilds\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Samples\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Saved\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Templates\" />" + Environment.NewLine +
					  "" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Binaries\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Build\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Content\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/DerivedDataCache\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Documentation\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Extras\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Intermediate\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Programs\" />" + Environment.NewLine +
					  "\t\t\t<file path=\"$PROJECT_DIR$/Engine/Saved\" />" + Environment.NewLine +
					"\t\t</excludeRoots>" + Environment.NewLine +
				  "\t</component>" + Environment.NewLine +
				"</project>" + Environment.NewLine;

			var FullFileName = Path.Combine(MasterProjectPath.FullName, ".idea/misc.xml");
			WriteFileIfChanged(FullFileName, CLionIngoreXml);
		}

		#region ProjectFileGenerator implementation

		protected override bool WriteProjectFiles()
		{
			WriteCLionIgnoreDirs();
			return WriteCMakeLists();
		}

		public override MasterProjectFolder AllocateMasterProjectFolder(ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName)
		{
			return new CMakefileFolder(InitOwnerProjectFileGenerator, InitFolderName);
		}

		/// <summary>
		/// This will filter out numerous targets to speed up cmake processing
		/// </summary>
		protected bool bCmakeMinimalTargets = false;

		protected override void ConfigureProjectFileGeneration(String[] Arguments, ref bool IncludeAllPlatforms)
		{
			base.ConfigureProjectFileGeneration(Arguments, ref IncludeAllPlatforms);
			// Check for minimal build targets to speed up cmake processing
			foreach (string CurArgument in Arguments)
			{
				switch (CurArgument.ToUpperInvariant())
				{
					case "-CMAKEMINIMALTARGETS":
						// To speed things up
						bIncludeDocumentation = false;
						bIncludeShaderSource = true;
						bIncludeTemplateFiles = false;
						bIncludeConfigFiles = true;
						// We want to filter out sets of targets to speed up builds via cmake
						bCmakeMinimalTargets = true;
						break;
				}
			}
		}

		/// <summary>
		/// Allocates a generator-specific project file object
		/// </summary>
		/// <param name="InitFilePath">Path to the project file</param>
		/// <returns>The newly allocated project file object</returns>
		protected override ProjectFile AllocateProjectFile(FileReference InitFilePath)
		{
			return new CMakefileProjectFile(InitFilePath);
		}

		public override void CleanProjectFiles(DirectoryReference InMasterProjectDirectory, string InMasterProjectName, DirectoryReference InIntermediateProjectFilesDirectory)
		{
			FileReference MasterProjectFile = FileReference.Combine(InMasterProjectDirectory, "CMakeLists.txt");
			if (FileReference.Exists(MasterProjectFile))
			{
				FileReference.Delete(MasterProjectFile);
			}
		}

		#endregion ProjectFileGenerator implementation
	}
}
