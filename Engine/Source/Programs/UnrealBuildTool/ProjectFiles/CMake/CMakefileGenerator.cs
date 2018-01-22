// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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
		/// Is this a project build?
		/// </summary>
		/// <remarks>
		/// This determines if engine files are included in the source lists.
		/// </remarks>
		/// <returns><value>true</value> if we should treat this as a project build; <value>false</value> otherwise.</returns>
		public bool IsProjectBuild
		{
			get { return !string.IsNullOrEmpty(GameProjectName); }
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
		/// The CMake helper file extension
		/// </summary>
		public string CMakeExtension
		{
			get
			{
				return ".cmake";
			}
		}

		/// <summary>
		/// The CMake file used to store the list of includes for the project.
		/// </summary>
		public string CMakeIncludesFileName
		{
			get
			{
				return "cmake-includes" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the configuration files (INI) for the engine.
		/// </summary>
		public string CMakeEngineConfigsFileName
		{
			get
			{
				return "cmake-config-engine" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the configuration files (INI) for the project.
		/// </summary>
		public string CMakeProjectConfigsFileName
		{
			get
			{
				return "cmake-config-project" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the additional build configuration files (CSharp) for the engine.
		/// </summary>
		public string CMakeEngineCSFileName
		{
			get
			{
				return "cmake-csharp-engine" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the additional configuration files (CSharp) for the project.
		/// </summary>
		public string CMakeProjectCSFileName
		{
			get
			{
				return "cmake-csharp-project" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the additional shader files (usf/ush) for the engine.
		/// </summary>
		public string CMakeEngineShadersFileName
		{
			get
			{
				return "cmake-shaders-engine" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the additional shader files (usf/ush) for the project.
		/// </summary>
		public string CMakeProjectShadersFileName
		{
			get
			{
				return "cmake-shaders-project" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the list of engine headers.
		/// </summary>
		public string CMakeEngineHeadersFileName
		{
			get
			{
				return "cmake-headers-ue4" + CMakeExtension;
			}
		}
		/// <summary>
		/// The CMake file used to store the list of engine headers.
		/// </summary>
		public string CMakeProjectHeadersFileName
		{
			get
			{
				return "cmake-headers-project" + CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the list of sources for the engine.
		/// </summary>
		public string CMakeEngineSourcesFileName
		{
			get
			{
				return "cmake-sources-engine" + CMakeExtension;
			}
		}
		
		/// <summary>
		/// The CMake file used to store the list of sources for the project.
		/// </summary>
		public string CMakeProjectSourcesFileName
		{
			get
			{
				return "cmake-sources-project" +  CMakeExtension;
			}
		}

		/// <summary>
		/// The CMake file used to store the list of definitions for the project.
		/// </summary>
		public string CMakeDefinitionsFileName
		{
			get
			{
				return "cmake-definitions" + CMakeExtension;
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

		private void AppendCleanedPathToList(StringBuilder EngineFiles, StringBuilder ProjectFiles, String SourceFileRelativeToRoot, String FullName, String GameProjectPath, String UE4RootPath, String GameRootPath)
		{
			if (!SourceFileRelativeToRoot.StartsWith("..") && !Path.IsPathRooted(SourceFileRelativeToRoot))
			{
				EngineFiles.Append("\t\"" + UE4RootPath + "/Engine/" + Utils.CleanDirectorySeparators(SourceFileRelativeToRoot, '/') + "\"\n");
			}
			else
			{
				if (String.IsNullOrEmpty(GameProjectName))
				{
					EngineFiles.Append("\t\"" + Utils.CleanDirectorySeparators(SourceFileRelativeToRoot, '/').Substring(3) + "\"\n");
				}
				else
				{
					string RelativeGameSourcePath = Utils.MakePathRelativeTo(FullName, GameProjectPath);
					ProjectFiles.Append("\t\"" + GameRootPath + "/" + Utils.CleanDirectorySeparators(RelativeGameSourcePath, '/') + "\"\n");
				}
			}
		}

		private bool WriteCMakeLists()
		{
			string BuildCommand;
			const string CMakeSectionEnd = " )\n\n";
			var CMakefileContent = new StringBuilder();

			// Create Engine/Project specific lists
			StringBuilder CMakeEngineSourceFilesList = new StringBuilder("set(ENGINE_SOURCE_FILES \n");
			StringBuilder CMakeProjectSourceFilesList = new StringBuilder("set(PROJECT_SOURCE_FILES \n");
			StringBuilder CMakeEngineHeaderFilesList = new StringBuilder("set(ENGINE_HEADER_FILES \n");
			StringBuilder CMakeProjectHeaderFilesList = new StringBuilder("set(PROJECT_HEADER_FILES \n");
			StringBuilder CMakeEngineCSFilesList = new StringBuilder("set(ENGINE_CSHARP_FILES \n");
			StringBuilder CMakeProjectCSFilesList = new StringBuilder("set(PROJECT_CSHARP_FILES \n");
			StringBuilder CMakeEngineConfigFilesList = new StringBuilder("set(ENGINE_CONFIG_FILES \n");
			StringBuilder CMakeProjectConfigFilesList = new StringBuilder("set(PROJECT_CONFIG_FILES \n");
			StringBuilder CMakeEngineShaderFilesList = new StringBuilder("set(ENGINE_SHADER_FILES \n");
			StringBuilder CMakeProjectShaderFilesList = new StringBuilder("set(PROJECT_SHADER_FILES \n");

			StringBuilder IncludeDirectoriesList = new StringBuilder("include_directories( \n");
			StringBuilder PreprocessorDefinitionsList = new StringBuilder("add_definitions( \n");

            string UE4RootPath = Utils.CleanDirectorySeparators(UnrealBuildTool.RootDirectory.FullName, '/');
            var CMakeGameRootPath = "";
			string GameProjectPath = "";
			string CMakeGameProjectFile = "";

			string HostArchitecture;
			switch (BuildHostPlatform.Current.Platform)
			{
				case UnrealTargetPlatform.Win64:
				{
					HostArchitecture = "Win64";
					BuildCommand = "call \"" + UE4RootPath + "/Engine/Build/BatchFiles/Build.bat\"";
					break;
				}
				case UnrealTargetPlatform.Mac:
				{
					HostArchitecture = "Mac";
					BuildCommand = "cd \"" + UE4RootPath + "\" && bash \"" + UE4RootPath + "/Engine/Build/BatchFiles/" + HostArchitecture + "/Build.sh\"";
					bIncludeIOSTargets = true;
					bIncludeTVOSTargets = true;
					break;
				}
				case UnrealTargetPlatform.Linux:
				{
					HostArchitecture = "Linux";
					BuildCommand = "cd \"" + UE4RootPath + "\" && bash \"" + UE4RootPath + "/Engine/Build/BatchFiles/" + HostArchitecture + "/Build.sh\"";
					break;
				}
				default:
				{
					throw new BuildException("ERROR: CMakefileGenerator does not support this platform");
				}
			}

			if (IsProjectBuild)
			{
				GameProjectPath = OnlyGameProject.Directory.FullName;
				CMakeGameRootPath = Utils.CleanDirectorySeparators(OnlyGameProject.Directory.FullName, '/');
                CMakeGameProjectFile = Utils.CleanDirectorySeparators(OnlyGameProject.FullName, '/');
			}

			// Additional CMake file definitions
			string EngineHeadersFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeEngineHeadersFileName).ToString();
			string ProjectHeadersFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeProjectHeadersFileName).ToString();
			string EngineSourcesFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeEngineSourcesFileName).ToString();
			string ProjectSourcesFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeProjectSourcesFileName).ToString();
			string ProjectFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeProjectSourcesFileName).ToString();
			string IncludeFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeIncludesFileName).ToString();
			string EngineConfigsFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeEngineConfigsFileName).ToString();
			string ProjectConfigsFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeProjectConfigsFileName).ToString();
			string EngineCSFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeEngineCSFileName).ToString();
			string ProjectCSFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeProjectCSFileName).ToString();
			string EngineShadersFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeEngineShadersFileName).ToString();
			string ProjectShadersFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeProjectShadersFileName).ToString();
			string DefinitionsFilePath = FileReference.Combine(IntermediateProjectFilesPath, CMakeDefinitionsFileName).ToString();

			CMakefileContent.Append(
				"# Makefile generated by CMakefileGenerator.cs (v1.2)\n" +
				"# *DO NOT EDIT*\n\n" +
				"cmake_minimum_required (VERSION 2.6)\n" +
				"project (UE4)\n\n" + 			
				"# CMake Flags\n" +
				"set(CMAKE_CXX_STANDARD 14)\n" + // Need to keep this updated
				"set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1 CACHE BOOL \"\" FORCE)\n" +
				"set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 1 CACHE BOOL \"\" FORCE)\n\n" +
				"# Standard Includes\n" +
				"include(\"" + IncludeFilePath + "\")\n" +
				"include(\"" + DefinitionsFilePath + "\")\n" +
				"include(\"" + EngineHeadersFilePath + "\")\n" +
				"include(\"" + ProjectHeadersFilePath + "\")\n" +
				"include(\"" + EngineSourcesFilePath + "\")\n" +
				"include(\"" + ProjectSourcesFilePath + "\")\n" +
				"include(\"" + EngineCSFilePath + "\")\n" +
				"include(\"" + ProjectCSFilePath + "\")\n\n"
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
							IncludeDirectories.Add(IncludeDirectory.Replace(UnrealBuildTool.RootDirectory.FullName, UE4RootPath));
						}
						else
						{
							// If the path isn't rooted, then it is relative to the game root
							if (!Path.IsPathRooted(IncludeDirectory))
							{
								IncludeDirectories.Add(CMakeGameRootPath + "/" + IncludeDirectory);
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
							AppendCleanedPathToList(CMakeEngineSourceFilesList, CMakeProjectSourceFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".h"))
						{
							AppendCleanedPathToList(CMakeEngineHeaderFilesList, CMakeProjectHeaderFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".cs"))
						{
							AppendCleanedPathToList(CMakeEngineCSFilesList, CMakeProjectCSFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".usf") || SourceFileRelativeToRoot.EndsWith(".ush"))
						{
							AppendCleanedPathToList(CMakeEngineShaderFilesList, CMakeProjectShaderFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
						}
						else if (SourceFileRelativeToRoot.EndsWith(".ini"))
						{
							AppendCleanedPathToList(CMakeEngineConfigFilesList, CMakeProjectConfigFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
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
					AppendCleanedPathToList(CMakeEngineShaderFilesList, CMakeProjectShaderFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
				}
			}

			// Add Engine/Config ini files (game are added via modules)
			var EngineConfigFiles = SourceFileSearch.FindFiles(DirectoryReference.Combine(UnrealBuildTool.EngineDirectory, "Config"));
			foreach (FileReference CurSourceFile in EngineConfigFiles)
			{
				string SourceFileRelativeToRoot = CurSourceFile.MakeRelativeTo(UnrealBuildTool.EngineDirectory);
				if (SourceFileRelativeToRoot.EndsWith(".ini"))
				{
					AppendCleanedPathToList(CMakeEngineConfigFilesList, CMakeProjectConfigFilesList, SourceFileRelativeToRoot, CurSourceFile.FullName, GameProjectPath, UE4RootPath, CMakeGameRootPath);
				}
			}

			// Add section end to section strings;
			CMakeEngineSourceFilesList.Append(CMakeSectionEnd);
			CMakeEngineHeaderFilesList.Append(CMakeSectionEnd);
			CMakeEngineCSFilesList.Append(CMakeSectionEnd);
			CMakeEngineConfigFilesList.Append(CMakeSectionEnd);
			CMakeEngineShaderFilesList.Append(CMakeSectionEnd);

			CMakeProjectSourceFilesList.Append(CMakeSectionEnd);
			CMakeProjectHeaderFilesList.Append(CMakeSectionEnd);
			CMakeProjectCSFilesList.Append(CMakeSectionEnd);
			CMakeProjectConfigFilesList.Append(CMakeSectionEnd);
			CMakeProjectShaderFilesList.Append(CMakeSectionEnd);
			
			IncludeDirectoriesList.Append(CMakeSectionEnd);
			PreprocessorDefinitionsList.Append(CMakeSectionEnd);

			if (bIncludeShaderSource)
			{	
				CMakefileContent.Append("# Optional Shader Include\n");
				if (!IsProjectBuild || bIncludeEngineSource)
				{
					CMakefileContent.Append("include(\"" + EngineShadersFilePath + "\")\n");
					CMakefileContent.Append("set_source_files_properties(${ENGINE_SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)\n");	
				}
				CMakefileContent.Append("include(\"" + ProjectShadersFilePath + "\")\n");
                CMakefileContent.Append("set_source_files_properties(${PROJECT_SHADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)\n");
                CMakefileContent.Append("source_group(\"Shader Files\" REGULAR_EXPRESSION .*.usf)\n\n");
			}

			if (bIncludeConfigFiles)
			{
				CMakefileContent.Append("# Optional Config Include\n");
				if (!IsProjectBuild || bIncludeEngineSource)
				{
					CMakefileContent.Append("include(\"" + EngineConfigsFilePath + "\")\n");
					CMakefileContent.Append("set_source_files_properties(${ENGINE_CONFIG_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)\n");
				}
				CMakefileContent.Append("include(\"" + ProjectConfigsFilePath + "\")\n");
				CMakefileContent.Append("set_source_files_properties(${PROJECT_CONFIG_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)\n");
				CMakefileContent.Append("source_group(\"Config Files\" REGULAR_EXPRESSION .*.ini)\n\n");
			}

			string CMakeProjectCmdArg = "";
            string UBTArguements = "";

            if (bGeneratingGameProjectFiles)
			{
                UBTArguements += " -game";
            }
			// Should the builder output progress ticks
			if (ProgressWriter.bWriteMarkup)
			{
				UBTArguements += " -progress";	
			}

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
							if (UnrealBuildTool.IsValidConfiguration(CurConfiguration) && !IsTargetExcluded(TargetName, BuildHostPlatform.Current.Platform, CurConfiguration))
							{
								if (TargetName == GameProjectName || TargetName == (GameProjectName + "Editor"))
								{
									CMakeProjectCmdArg = "\"-project="+ CMakeGameProjectFile + "\"";
								}

								string ConfName = Enum.GetName(typeof(UnrealTargetConfiguration), CurConfiguration);
								CMakefileContent.Append(String.Format("add_custom_target({0}-{3}-{1} {5} {0} {3} {1} {2}{4} VERBATIM)\n", TargetName, ConfName, CMakeProjectCmdArg, HostArchitecture, UBTArguements, BuildCommand));

								// Add iOS and TVOS targets if valid
								if (bIncludeIOSTargets && !IsTargetExcluded(TargetName, UnrealTargetPlatform.IOS, CurConfiguration))
								{
    								CMakefileContent.Append(String.Format("add_custom_target({0}-{3}-{1} {5} {0} {3} {1} {2}{4} VERBATIM)\n", TargetName, ConfName, CMakeProjectCmdArg, UnrealTargetPlatform.IOS, UBTArguements, BuildCommand));
								}
								if (bIncludeTVOSTargets && !IsTargetExcluded(TargetName, UnrealTargetPlatform.TVOS, CurConfiguration))
								{
    								CMakefileContent.Append(String.Format("add_custom_target({0}-{3}-{1} {5} {0} {3} {1} {2}{4} VERBATIM)\n", TargetName, ConfName, CMakeProjectCmdArg, UnrealTargetPlatform.TVOS, UBTArguements, BuildCommand));
								}
							}
						}
					}
                    if (!IsTargetExcluded(TargetName, BuildHostPlatform.Current.Platform, UnrealTargetConfiguration.Development))
                    {
                        if (TargetName == GameProjectName || TargetName == (GameProjectName + "Editor"))
                        {
                            CMakeProjectCmdArg = "\"-project=" + CMakeGameProjectFile + "\"";
                        }

                        CMakefileContent.Append(String.Format("add_custom_target({0} {4} {0} {2} Development {1}{3} VERBATIM)\n\n", TargetName, CMakeProjectCmdArg, HostArchitecture, UBTArguements, BuildCommand));

                        // Add iOS and TVOS targets if valid
                        if (bIncludeIOSTargets && !IsTargetExcluded(TargetName, UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Development))
                        {
                           CMakefileContent.Append(String.Format("add_custom_target({0}-{3} {5} {0} {3} {1} {2}{4} VERBATIM)\n", TargetName, UnrealTargetConfiguration.Development, CMakeProjectCmdArg, UnrealTargetPlatform.IOS, UBTArguements, BuildCommand));
                        }
                        if (bIncludeTVOSTargets && !IsTargetExcluded(TargetName, UnrealTargetPlatform.TVOS, UnrealTargetConfiguration.Development))
                        {
                            CMakefileContent.Append(String.Format("add_custom_target({0}-{3} {5} {0} {3} {1} {2}{4} VERBATIM)\n", TargetName, UnrealTargetConfiguration.Development, CMakeProjectCmdArg, UnrealTargetPlatform.TVOS, UBTArguements, BuildCommand));
                        }
                   }
				}		
			}

			// Create Build Template
			if (IsProjectBuild && !bIncludeEngineSource)
			{
				CMakefileContent.AppendLine("add_executable(FakeTarget ${PROJECT_HEADER_FILES} ${PROJECT_SOURCE_FILES} ${PROJECT_CSHARP_FILES} ${PROJECT_SHADER_FILES} ${PROJECT_CONFIG_FILES})");
			}
			else
			{
				CMakefileContent.AppendLine("add_executable(FakeTarget ${ENGINE_HEADER_FILES} ${ENGINE_SOURCE_FILES} ${ENGINE_CSHARP_FILES} ${ENGINE_SHADER_FILES} ${ENGINE_CONFIG_FILES} ${PROJECT_HEADER_FILES} ${PROJECT_SOURCE_FILES} ${PROJECT_CSHARP_FILES} ${PROJECT_SHADER_FILES} ${PROJECT_CONFIG_FILES})");
			}

			var FullFileName = Path.Combine(MasterProjectPath.FullName, ProjectFileName);

			// Write out CMake files
			bool bWriteMakeList = WriteFileIfChanged(FullFileName, CMakefileContent.ToString());
			bool bWriteEngineHeaders = WriteFileIfChanged(EngineHeadersFilePath, CMakeEngineHeaderFilesList.ToString());
			bool bWriteProjectHeaders = WriteFileIfChanged(ProjectHeadersFilePath, CMakeProjectHeaderFilesList.ToString());
			bool bWriteEngineSources = WriteFileIfChanged(EngineSourcesFilePath, CMakeEngineSourceFilesList.ToString());
			bool bWriteProjectSources = WriteFileIfChanged(ProjectSourcesFilePath, CMakeProjectSourceFilesList.ToString());
			bool bWriteIncludes = WriteFileIfChanged(IncludeFilePath, IncludeDirectoriesList.ToString());
			bool bWriteDefinitions = WriteFileIfChanged(DefinitionsFilePath, PreprocessorDefinitionsList.ToString());
			bool bWriteEngineConfigs = WriteFileIfChanged(EngineConfigsFilePath, CMakeEngineConfigFilesList.ToString());
			bool bWriteProjectConfigs = WriteFileIfChanged(ProjectConfigsFilePath, CMakeProjectConfigFilesList.ToString());
			bool bWriteEngineShaders = WriteFileIfChanged(EngineShadersFilePath, CMakeEngineShaderFilesList.ToString());
			bool bWriteProjectShaders = WriteFileIfChanged(ProjectShadersFilePath, CMakeProjectShaderFilesList.ToString());
			bool bWriteEngineCS = WriteFileIfChanged(EngineCSFilePath, CMakeEngineCSFilesList.ToString());
			bool bWriteProjectCS = WriteFileIfChanged(ProjectCSFilePath, CMakeProjectCSFilesList.ToString());			

			// Return success flag if all files were written out successfully
			return bWriteMakeList &&
					bWriteEngineHeaders && bWriteProjectHeaders &&
					bWriteEngineSources && bWriteProjectSources &&
					bWriteEngineConfigs && bWriteProjectConfigs &&
					bWriteEngineCS && bWriteProjectCS &&
					bWriteEngineShaders && bWriteProjectShaders &&
					bWriteIncludes && bWriteDefinitions;
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

		private bool IsTargetExcluded(string TargetName, UnrealTargetPlatform TargetPlatform, UnrealTargetConfiguration TargetConfig)
		{
			if (TargetPlatform == UnrealTargetPlatform.IOS || TargetPlatform == UnrealTargetPlatform.TVOS)
			{
				if ((TargetName.StartsWith("UE4Game") || (IsProjectBuild && TargetName.StartsWith(GameProjectName)) || TargetName.StartsWith("QAGame")) && !TargetName.StartsWith("QAGameEditor"))
				{
				    return false;
				}
				return true;
            }
			// Only do this level of filtering if we are trying to speed things up tremendously
			if (bCmakeMinimalTargets)
			{
				// Editor or game builds get all target configs
				// The game project editor or game get all configs
				if ((TargetName.StartsWith("UE4Editor") && !TargetName.StartsWith("UE4EditorServices")) ||
					TargetName.StartsWith("UE4Game") ||
					(IsProjectBuild && TargetName.StartsWith(GameProjectName)))
				{
					return false;
				}
				// SCW & CRC are minimally included as just development builds
				else if (TargetConfig == UnrealTargetConfiguration.Development &&
					(TargetName.StartsWith("ShaderCompileWorker") || TargetName.StartsWith("CrashReportClient")))
				{
					return false;
				}
				else if ((TargetName.StartsWith("QAGameEditor") && !TargetName.StartsWith("QAGameEditorServices")) || TargetName.StartsWith("QAGame"))
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

		/// <summary>
		/// Whether to include iOS targets or not
		/// </summary>
		protected bool bIncludeIOSTargets = false;

		/// <summary>
		/// Whether to include TVOS targets or not
		/// </summary>
		protected bool bIncludeTVOSTargets = false;

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
			// Remove Project File
			FileReference MasterProjectFile = FileReference.Combine(InMasterProjectDirectory, ProjectFileName);
			if (FileReference.Exists(MasterProjectFile))
			{
				FileReference.Delete(MasterProjectFile);
			}

			// Remove Headers Files
			FileReference EngineHeadersFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeEngineHeadersFileName);
			if (FileReference.Exists(EngineHeadersFile))
			{
				FileReference.Delete(EngineHeadersFile);
			}
			FileReference ProjectHeadersFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeProjectHeadersFileName);
			if (FileReference.Exists(ProjectHeadersFile))
			{
				FileReference.Delete(ProjectHeadersFile);
			}

			// Remove Sources Files
			FileReference EngineSourcesFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeEngineSourcesFileName);
			if (FileReference.Exists(EngineSourcesFile))
			{
				FileReference.Delete(EngineSourcesFile);
			}
			FileReference ProjectSourcesFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeProjectSourcesFileName);
			if (FileReference.Exists(ProjectSourcesFile))
			{
				FileReference.Delete(ProjectSourcesFile);
			}

			// Remove Includes File
			FileReference IncludeFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeIncludesFileName);
			if (FileReference.Exists(IncludeFile))
			{
				FileReference.Delete(IncludeFile);
			}
			
			// Remove CSharp Files
			FileReference EngineCSFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeEngineCSFileName);
			if (FileReference.Exists(EngineCSFile))
			{
				FileReference.Delete(EngineCSFile);
			}
			FileReference ProjectCSFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeProjectCSFileName);
			if (FileReference.Exists(ProjectCSFile))
			{
				FileReference.Delete(ProjectCSFile);
			}

			// Remove Config Files
			FileReference EngineConfigFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeEngineConfigsFileName);
			if (FileReference.Exists(EngineConfigFile))
			{
				FileReference.Delete(EngineConfigFile);
			}
			FileReference ProjectConfigsFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeProjectConfigsFileName);
			if (FileReference.Exists(ProjectConfigsFile))
			{
				FileReference.Delete(ProjectConfigsFile);
			}

			// Remove Config Files
			FileReference EngineShadersFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeEngineShadersFileName);
			if (FileReference.Exists(EngineShadersFile))
			{
				FileReference.Delete(EngineShadersFile);
			}
			FileReference ProjectShadersFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeProjectShadersFileName);
			if (FileReference.Exists(ProjectShadersFile))
			{
				FileReference.Delete(ProjectShadersFile);
			}

			// Remove Definitions File
			FileReference DefinitionsFile = FileReference.Combine(InIntermediateProjectFilesDirectory, CMakeDefinitionsFileName);
			if (FileReference.Exists(DefinitionsFile))
			{
				FileReference.Delete(DefinitionsFile);
			}
		}

		#endregion ProjectFileGenerator implementation
	}
}
