// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Xml.Linq;

namespace UnrealBuildTool
{
	// Represents a folder within the master project. TODO Not using at the moment.
	public class CodeLiteFolder : MasterProjectFolder
	{
		public CodeLiteFolder( ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName )
			: base(InitOwnerProjectFileGenerator, InitFolderName)
		{
		}
	}

	public class CodeLiteGenerator : ProjectFileGenerator
	{
		public string SolutionExtension = ".workspace";
		public string CodeCompletionFileName = "CodeCompletionFolders.txt";
		public string CodeCompletionPreProcessorFileName = "CodeLitePreProcessor.txt";

		//
		// Returns CodeLite's project filename extension.
		//
		override public string ProjectFileExtension
		{
			get
			{
				return ".project";
			}
		}

		protected override bool WriteMasterProjectFile( ProjectFile UBTProject )
		{
			var SolutionFileName = MasterProjectName + SolutionExtension;
			var CodeCompletionFile = MasterProjectName + CodeCompletionFileName;
			var CodeCompletionPreProcessorFile = MasterProjectName + CodeCompletionPreProcessorFileName;

			var FullCodeLiteMasterFile = Path.Combine(MasterProjectRelativePath, SolutionFileName);
			var FullCodeLiteCodeCompletionFile = Path.Combine(MasterProjectRelativePath, CodeCompletionFile);
			var FullCodeLiteCodeCompletionPreProcessorFile = Path.Combine(MasterProjectRelativePath, CodeCompletionPreProcessorFile);

			//
			// HACK 
			// TODO This is for now a hack. According to the original submitter, Eranif (a CodeLite developer) will support code completion folders in *.workspace files.
			// We create a seperate file with all the folder name in it to copy manually into the code completion
			// filed of CodeLite workspace. (Workspace Settings/Code Completion -> copy the content of the file threre.)
			List<string> IncludeDirectories = new List<string>();
			List<string> PreProcessor = new List<string>();

			foreach (var CurProject in GeneratedProjectFiles) {

				CodeLiteProject Project = CurProject as CodeLiteProject;
				if (Project == null)
				{
					continue;
				}

				foreach (var CurrentPath in Project.IntelliSenseIncludeSearchPaths) 
				{
					// Convert relative path into abosulte.
					string IntelliSenseIncludeSearchPath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Path.GetFullPath(Project.ProjectFilePath)), CurrentPath));
					IncludeDirectories.Add(IntelliSenseIncludeSearchPath);
				}
				foreach (var CurrentPath in Project.IntelliSenseSystemIncludeSearchPaths) 
				{
					// Convert relative path into abosulte.
					string IntelliSenseSystemIncludeSearchPath = Path.GetFullPath(Path.Combine(Path.GetDirectoryName(Path.GetFullPath(Project.ProjectFilePath)), CurrentPath));
					IncludeDirectories.Add (IntelliSenseSystemIncludeSearchPath);
				}

				foreach( var CurDef in Project.IntelliSensePreprocessorDefinitions )
				{
					if (!PreProcessor.Contains(CurDef))
					{
						PreProcessor.Add(CurDef);
					}
				}

			}

			//
			// Write code completions data into files.
			//
			File.WriteAllLines(FullCodeLiteCodeCompletionFile, IncludeDirectories);
			File.WriteAllLines(FullCodeLiteCodeCompletionPreProcessorFile, PreProcessor);

			//
			// Write CodeLites Workspace
			//
			XmlWriterSettings settings = new XmlWriterSettings();
			settings.Indent = true;

			XElement CodeLiteWorkspace = new XElement("CodeLite_Workspace");
			XAttribute CodeLiteWorkspaceName = new XAttribute("Name", MasterProjectName);
			XAttribute CodeLiteWorkspaceSWTLW = new XAttribute("SWTLW", "Yes"); // This flag will only work in CodeLite version > 8.0. See below
			CodeLiteWorkspace.Add(CodeLiteWorkspaceName);
			CodeLiteWorkspace.Add(CodeLiteWorkspaceSWTLW);

			//
			// ATTN This part will work for the next release of CodeLite. That may
			// be CodeLite version > 8.0. CodeLite 8.0 does not have this functionality.
			// TODO Macros are ignored for now.
			//
			// Write Code Completion folders into the WorkspaceParserPaths section.
			//
			XElement CodeLiteWorkspaceParserPaths = new XElement("WorkspaceParserPaths");
			foreach (var CurrentPath in IncludeDirectories) 
			{
				XElement CodeLiteWorkspaceParserPathInclude = new XElement("Include");
				XAttribute CodeLiteWorkspaceParserPath = new XAttribute("Path", CurrentPath);
				CodeLiteWorkspaceParserPathInclude.Add(CodeLiteWorkspaceParserPath);
				CodeLiteWorkspaceParserPaths.Add(CodeLiteWorkspaceParserPathInclude);

			}
			CodeLiteWorkspace.Add(CodeLiteWorkspaceParserPaths);

			//
			// Write project file information into CodeLite's workspace file.
			//
			foreach( var CurProject in AllProjectFiles )
			{
				var ProjectExtension = Path.GetExtension (CurProject.ProjectFilePath);

				//
				// TODO For now ignore C# project files.
				//
				if (ProjectExtension == ".csproj") 
				{
					continue;
				}

				//
				// Iterate through all targets.
				// 
				foreach (ProjectTarget CurrentTarget in CurProject.ProjectTargets) 
				{
					string[] tmp = CurrentTarget.ToString ().Split ('.');
					string ProjectTargetFileName = Path.GetDirectoryName (CurProject.RelativeProjectFilePath) + "/" + tmp [0] + ProjectExtension;
					String ProjectName = tmp [0];


					XElement CodeLiteWorkspaceProject = new XElement("Project");
					XAttribute CodeLiteWorkspaceProjectName = new XAttribute("Name", ProjectName);
					XAttribute CodeLiteWorkspaceProjectPath = new XAttribute("Path", ProjectTargetFileName);
					XAttribute CodeLiteWorkspaceProjectActive = new XAttribute("Active", "No");
					CodeLiteWorkspaceProject.Add(CodeLiteWorkspaceProjectName);
					CodeLiteWorkspaceProject.Add(CodeLiteWorkspaceProjectPath);
					CodeLiteWorkspaceProject.Add(CodeLiteWorkspaceProjectActive);
					CodeLiteWorkspace.Add(CodeLiteWorkspaceProject);
				}
			}

			//
			// We need to create the configuration matrix. That will assign the project configuration to 
			// the samge workspace configuration.
			//
			XElement CodeLiteWorkspaceBuildMatrix = new XElement("BuildMatrix");
			foreach (UnrealTargetConfiguration CurConfiguration in SupportedConfigurations) 
			{
				if (UnrealBuildTool.IsValidConfiguration(CurConfiguration))
				{
					XElement CodeLiteWorkspaceBuildMatrixConfiguration = new XElement("WorkspaceConfiguration");
					XAttribute CodeLiteWorkspaceProjectName = new XAttribute("Name", CurConfiguration.ToString());
					XAttribute CodeLiteWorkspaceProjectSelected = new XAttribute("Selected", "no");
					CodeLiteWorkspaceBuildMatrixConfiguration.Add(CodeLiteWorkspaceProjectName);
					CodeLiteWorkspaceBuildMatrixConfiguration.Add(CodeLiteWorkspaceProjectSelected);

					foreach( var CurProject in AllProjectFiles )
					{
						var ProjectExtension = Path.GetExtension (CurProject.ProjectFilePath);

						//
						// TODO For now ignore C# project files.
						//
						if (ProjectExtension == ".csproj") 
						{
							continue;
						}

						foreach (ProjectTarget target in CurProject.ProjectTargets) 
						{
							string[] tmp = target.ToString ().Split ('.');
							String ProjectName = tmp [0];

							XElement CodeLiteWorkspaceBuildMatrixConfigurationProject = new XElement("Project");
							XAttribute CodeLiteWorkspaceBuildMatrixConfigurationProjectName = new XAttribute("Name", ProjectName);
							XAttribute CodeLiteWorkspaceBuildMatrixConfigurationProjectConfigName = new XAttribute("ConfigName", CurConfiguration.ToString());
							CodeLiteWorkspaceBuildMatrixConfigurationProject.Add(CodeLiteWorkspaceBuildMatrixConfigurationProjectName);
							CodeLiteWorkspaceBuildMatrixConfigurationProject.Add(CodeLiteWorkspaceBuildMatrixConfigurationProjectConfigName);
							CodeLiteWorkspaceBuildMatrixConfiguration.Add(CodeLiteWorkspaceBuildMatrixConfigurationProject);
						}
					}
					CodeLiteWorkspaceBuildMatrix.Add(CodeLiteWorkspaceBuildMatrixConfiguration);
				}
			}

			CodeLiteWorkspace.Add(CodeLiteWorkspaceBuildMatrix);
			CodeLiteWorkspace.Save(FullCodeLiteMasterFile);

			return true;
		}

		protected override ProjectFile AllocateProjectFile( string InitFilePath )
		{
			return new CodeLiteProject( InitFilePath );
		}

		public override MasterProjectFolder AllocateMasterProjectFolder( ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName )
		{
			return new CodeLiteFolder( InitOwnerProjectFileGenerator, InitFolderName );
		}

		public override void CleanProjectFiles(string InMasterProjectRelativePath, string InMasterProjectName, string InIntermediateProjectFilesPath)
		{
			// TODO Delete all files here. Not finished yet.
			var SolutionFileName = InMasterProjectName + SolutionExtension;
			var CodeCompletionFile = InMasterProjectName + CodeCompletionFileName;
			var CodeCompletionPreProcessorFile = InMasterProjectName + CodeCompletionPreProcessorFileName;

			var FullCodeLiteMasterFile = Path.Combine(InMasterProjectRelativePath, SolutionFileName);
			var FullCodeLiteCodeCompletionFile = Path.Combine(InMasterProjectRelativePath, CodeCompletionFile);
			var FullCodeLiteCodeCompletionPreProcessorFile = Path.Combine(InMasterProjectRelativePath, CodeCompletionPreProcessorFile);

			if (File.Exists(FullCodeLiteMasterFile))
			{
				File.Delete(FullCodeLiteMasterFile);
			}
			if (File.Exists(FullCodeLiteCodeCompletionFile))
			{
				File.Delete(FullCodeLiteCodeCompletionFile);
			}
			if (File.Exists(FullCodeLiteCodeCompletionPreProcessorFile))
			{
				File.Delete(FullCodeLiteCodeCompletionPreProcessorFile);
			}

			// Delete the project files folder
			if (Directory.Exists(InIntermediateProjectFilesPath))
			{
				try
				{
					Directory.Delete(InIntermediateProjectFilesPath, true);
				}
				catch (Exception Ex)
				{
					Log.TraceInformation("Error while trying to clean project files path {0}. Ignored.", InIntermediateProjectFilesPath);
					Log.TraceInformation("\t" + Ex.Message);
				}
			}
		}

	}
}
