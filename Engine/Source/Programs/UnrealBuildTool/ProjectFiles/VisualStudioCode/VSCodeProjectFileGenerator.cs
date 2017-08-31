// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using Tools.DotNETCommon;

namespace UnrealBuildTool
{
	class VSCodeProjectFolder : MasterProjectFolder
	{
		public VSCodeProjectFolder(ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName)
			: base(InitOwnerProjectFileGenerator, InitFolderName)
		{
		}
	}

	class VSCodeProject : ProjectFile
	{
		public VSCodeProject(FileReference InitFilePath)
			: base(InitFilePath)
		{
		}

		public override bool WriteProjectFile(List<UnrealTargetPlatform> InPlatforms, List<UnrealTargetConfiguration> InConfigurations)
		{
			return true;
		}
	}

	class VSCodeProjectFileGenerator : ProjectFileGenerator
	{
		private DirectoryReference VSCodeDir;
		private UnrealTargetPlatform HostPlatform;
		private bool BuildingForDotNetCore;

		public VSCodeProjectFileGenerator(FileReference InOnlyGameProject)
			: base(InOnlyGameProject)
		{
			BuildingForDotNetCore = Environment.CommandLine.Contains("-dotnetcore");
		}

		class JsonFile
		{
			public JsonFile()
			{
			}

			public void BeginRootObject()
			{
				BeginObject();
			}

			public void EndRootObject()
			{
				EndObject();
				if (TabString.Length > 0)
				{
					throw new Exception("Called EndRootObject before all objects and arrays have been closed");
				}
			}

			public void BeginObject(string Name = null)
			{
				string Prefix = Name == null ? "" : Quoted(Name) + ": ";
				Lines.Add(TabString + Prefix + "{");
				TabString += "\t";
			}

			public void EndObject()
			{
				Lines[Lines.Count - 1] = Lines[Lines.Count - 1].TrimEnd(',');
				TabString = TabString.Remove(TabString.Length - 1);
				Lines.Add(TabString + "},");
			}

			public void BeginArray(string Name = null)
			{
				string Prefix = Name == null ? "" : Quoted(Name) + ": ";
				Lines.Add(TabString + Prefix + "[");
				TabString += "\t";
			}

			public void EndArray()
			{
				Lines[Lines.Count - 1] = Lines[Lines.Count - 1].TrimEnd(',');
				TabString = TabString.Remove(TabString.Length - 1);
				Lines.Add(TabString + "],");
			}

			public void AddField(string Name, bool Value)
			{
				Lines.Add(TabString + Quoted(Name) + ": " + Value.ToString().ToLower() + ",");
			}

			public void AddField(string Name, string Value)
			{
				Lines.Add(TabString + Quoted(Name) + ": " + Quoted(Value) + ",");
			}

			public void AddUnnamedField(string Value)
			{
				Lines.Add(TabString + Quoted(Value) + ",");
			}

			public void Write(FileReference File)
			{
				Lines[Lines.Count - 1] = Lines[Lines.Count - 1].TrimEnd(',');
				FileReference.WriteAllLines(File, Lines.ToArray());
			}

			private string Quoted(string Value)
			{
				return "\"" + Value + "\"";
			}

			private List<string> Lines = new List<string>();
			private string TabString = "";
		}

		override public string ProjectFileExtension
		{
			get
			{
				return ".vscode";
			}
		}

		public override void CleanProjectFiles(DirectoryReference InMasterProjectDirectory, string InMasterProjectName, DirectoryReference InIntermediateProjectFilesPath)
		{
		}

		public override bool ShouldGenerateIntelliSenseData()
		{
			return true;
		}

		protected override ProjectFile AllocateProjectFile(FileReference InitFilePath)
		{
			return new VSCodeProject(InitFilePath);
		}

		public override MasterProjectFolder AllocateMasterProjectFolder(ProjectFileGenerator InitOwnerProjectFileGenerator, string InitFolderName)
		{
			return new VSCodeProjectFolder(InitOwnerProjectFileGenerator, InitFolderName);
		}

		protected override bool WriteMasterProjectFile(ProjectFile UBTProject)
		{
			VSCodeDir = DirectoryReference.Combine(MasterProjectPath, ".vscode");
			DirectoryReference.CreateDirectory(VSCodeDir);

			HostPlatform = BuildHostPlatform.Current.Platform;

			List<ProjectFile> Projects = new List<ProjectFile>(AllProjectFiles);
			Projects.Sort((A, B) => { return A.ProjectFilePath.GetFileName().CompareTo(B.ProjectFilePath.GetFileName()); });

			WriteCppPropertiesFile(Projects);
			WriteTasksFile(Projects);
			WriteLaunchFile(Projects);
			WriteWorkspaceSettingsFile(Projects);

			return true;
		}

		private void WriteCppPropertiesFile(List<ProjectFile> Projects)
		{
			JsonFile OutFile = new JsonFile();

			HashSet<string> IncludePaths = new HashSet<string>();

			foreach (ProjectFile Project in Projects)
			{
				if (Project is VSCodeProject)
				{
					foreach (string IncludePath in Project.IntelliSenseIncludeSearchPaths)
					{
						DirectoryReference AbsPath = DirectoryReference.Combine(Project.ProjectFilePath.Directory, IncludePath);
						if (DirectoryReference.Exists(AbsPath))
						{
							IncludePaths.Add(AbsPath.ToString());
						}
					}
				}
			}

			// NOTE: This needs to be expanded and improved so we can get system include paths in for other platforms
			if (HostPlatform == UnrealTargetPlatform.Win64)
			{
				string[] VCIncludePaths = VCToolChain.GetVCIncludePaths(CppPlatform.Win64, WindowsCompiler.Default).Split(';');

				foreach (string VCIncludePath in VCIncludePaths)
				{
					if (!string.IsNullOrEmpty(VCIncludePath))
					{
						IncludePaths.Add(VCIncludePath);
					}
				}
			}

			OutFile.BeginRootObject();
			{
				OutFile.BeginArray("configurations");
				{
					OutFile.BeginObject();
					{
						OutFile.AddField("name", "UnrealEngine");

						OutFile.BeginArray("includePath");
						{
							foreach (var Path in IncludePaths)
							{
								OutFile.AddUnnamedField(Path.Replace('\\', '/'));
							}
						}
						OutFile.EndArray();

						OutFile.BeginArray("defines");
						{
							OutFile.AddUnnamedField("_DEBUG");
							OutFile.AddUnnamedField("UNICODE");
						}
						OutFile.EndArray();
					}
					OutFile.EndObject();
				}
				OutFile.EndArray();
			}
			OutFile.EndRootObject();

			OutFile.Write(FileReference.Combine(VSCodeDir, "c_cpp_properties.json"));
		}

		private class PlatformAndConfigToBuild
		{
			public string ProjectName { get; set; }
			public UnrealTargetPlatform Platform { get; set; }
			public UnrealTargetConfiguration Config{ get; set; }
		}

		private void GatherBuildTargets(ProjectFile Project, List<PlatformAndConfigToBuild> OutToBuild)
		{
			foreach (ProjectTarget Target in Project.ProjectTargets)
			{
				string ProjectName = Target.TargetRules.Name;

				List<UnrealTargetConfiguration> Configs = new List<UnrealTargetConfiguration>();
				List<UnrealTargetPlatform> Platforms = new List<UnrealTargetPlatform>();
				Target.TargetRules.GetSupportedConfigurations(ref Configs, true);
				Target.TargetRules.GetSupportedPlatforms(ref Platforms);

				foreach (UnrealTargetPlatform Platform in Platforms)
				{
					if (SupportedPlatforms.Contains(Platform))
					{
						foreach (UnrealTargetConfiguration Config in Configs)
						{
							OutToBuild.Add(new PlatformAndConfigToBuild { ProjectName = ProjectName, Platform = Platform, Config = Config });
						}
					}
				}
			}
		}

		private void WriteNativeTask(VSCodeProject InProject, JsonFile OutFile)
		{
			string[] Commands = { "Build", "Clean" };

			List<PlatformAndConfigToBuild> ToBuildList = new List<PlatformAndConfigToBuild>();
			GatherBuildTargets(InProject, ToBuildList);

			foreach (PlatformAndConfigToBuild ToBuild in ToBuildList)
			{
				foreach (string Command in Commands)
				{
					string TaskName = String.Format("{0} {1} {2} {3}", ToBuild.ProjectName, ToBuild.Platform.ToString(), ToBuild.Config, Command);
					List<string> ExtraParams = new List<string>();

					OutFile.BeginObject();
					{
						OutFile.AddField("taskName", TaskName);
						OutFile.AddField("group", "build");

						string CleanParam = Command == "Clean" ? "-clean" : null;

						if (BuildingForDotNetCore)
						{
							OutFile.AddField("command", "dotnet");
						}
						else
						{
							if (HostPlatform == UnrealTargetPlatform.Win64)
							{
								OutFile.AddField("command", "${workspaceRoot}/Engine/Build/BatchFiles/" + Command + ".bat");
								CleanParam = null;
							}
							else
							{
								OutFile.AddField("command", "${workspaceRoot}/Engine/Build/BatchFiles/" + HostPlatform.ToString() + "/Build.sh");

								if (Command == "Clean")
								{
									CleanParam = "-clean";
								}								
							}
						}
						
						OutFile.BeginArray("args");
						{
							if (BuildingForDotNetCore)
							{
								OutFile.AddUnnamedField("${workspaceRoot}/Engine/Binaries/DotNET/UnrealBuildTool_NETCore.dll");
							}

							OutFile.AddUnnamedField(ToBuild.ProjectName);
							OutFile.AddUnnamedField(ToBuild.Platform.ToString());
							OutFile.AddUnnamedField(ToBuild.Config.ToString());
							OutFile.AddUnnamedField("-waitmutex");

							if (!string.IsNullOrEmpty(CleanParam))
							{
								OutFile.AddUnnamedField(CleanParam);
							}
						}
						OutFile.EndArray();
						OutFile.AddField("problemMatcher", "$msCompile");

						if (!BuildingForDotNetCore)
						{
							OutFile.AddField("type", "shell");
						}
					}
					OutFile.EndObject();
				}
			}
		}

		private void WriteCSharpTask(VCSharpProjectFile InProject, JsonFile OutFile)
		{
			string[] Commands = { "Build", "Clean" };

			if (InProject.IsDotNETCoreProject() != BuildingForDotNetCore)
			{
				return;
			}

			foreach (ProjectTarget Target in InProject.ProjectTargets)
			{
				UnrealTargetConfiguration[] CSharpConfigs = { UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development };
				foreach (UnrealTargetConfiguration Config in CSharpConfigs)
				{
					CsProjectInfo ProjectInfo = InProject.GetProjectInfo(Config);
					DirectoryReference OutputDir = null;
					if (ProjectInfo.Properties.ContainsKey("OutputPath"))
					{
						OutputDir = DirectoryReference.Combine(InProject.ProjectFilePath.Directory, ProjectInfo.Properties["OutputPath"]);
					}
					string PathToProjectFile = "${workspaceRoot}/" + InProject.ProjectFilePath.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace('\\', '/');
					string AssemblyName;
					if (!ProjectInfo.Properties.TryGetValue("AssemblyName", out AssemblyName))
					{
						AssemblyName = InProject.ProjectFilePath.GetFileNameWithoutExtension();
					}

					foreach (string Command in Commands)
					{
						string TaskName = String.Format("{0} {1} {2} {3}", AssemblyName, HostPlatform, Config, Command);

						OutFile.BeginObject();
						{
							OutFile.AddField("taskName", TaskName);
							OutFile.AddField("group", "build");
							if (InProject.IsDotNETCoreProject())
							{
								OutFile.AddField("command", "dotnet");
							}
							else
							{
								if (Utils.IsRunningOnMono)
								{
									OutFile.AddField("command", "xbuild");
								}
								else
								{
									OutFile.AddField("command", "${workspaceRoot}/Engine/Build/BatchFiles/MSBuild.bat");
								}
							}
							OutFile.BeginArray("args");
							{
								if (InProject.IsDotNETCoreProject())
								{
									OutFile.AddUnnamedField(Command.ToLower());
								}
								else
								{
									OutFile.AddUnnamedField("/t:" + Command.ToLower());
								}
								OutFile.AddUnnamedField(PathToProjectFile);
								OutFile.AddUnnamedField("/p:GenerateFullPaths=true");
								if (HostPlatform == UnrealTargetPlatform.Win64)
								{
									OutFile.AddUnnamedField("/p:DebugType=portable");
								}
								OutFile.AddUnnamedField("/verbosity:minimal");

								if (InProject.IsDotNETCoreProject())
								{
									OutFile.AddUnnamedField("--configuration");
									OutFile.AddUnnamedField(Config.ToString());

									if (OutputDir != null)
									{
										string OutputDirString = "${workspaceRoot}/" + OutputDir.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace('\\', '/');
										OutFile.AddUnnamedField("--output");
										OutFile.AddUnnamedField(OutputDirString);
									}
								}
								else
								{
									OutFile.AddUnnamedField("/p:Configuration=" + Config.ToString());
								}
							}
							OutFile.EndArray();
						}
						OutFile.AddField("problemMatcher", "$msCompile");

						if (!BuildingForDotNetCore)
						{
							OutFile.AddField("type", "shell");
						}
						OutFile.EndObject();
					}
				}
			}
		}

		private void WriteTasksFile(List<ProjectFile> Projects)
		{
			JsonFile OutFile = new JsonFile();

			OutFile.BeginRootObject();
			{
				OutFile.AddField("version", "2.0.0");

				OutFile.BeginArray("tasks");
				{
					string[] Commands = { "Build", "Clean" };

					foreach (ProjectFile Project in Projects)
					{
						if (Project is VSCodeProject)
						{
							WriteNativeTask(Project as VSCodeProject, OutFile);
						}
						else if (Project is VCSharpProjectFile)
						{
							WriteCSharpTask(Project as VCSharpProjectFile, OutFile);
						}
					}

					OutFile.EndArray();
				}
			}
			OutFile.EndRootObject();

			OutFile.Write(FileReference.Combine(VSCodeDir, "tasks.json"));
		}

		public string EscapePath(string InputPath)
		{
			string Result = InputPath;
			if (Result.Contains(" "))
			{
				Result = "\"" + Result + "\"";
			}
			return Result;
		}

		private FileReference GetExecutableFilename(ProjectFile Project, ProjectTarget Target, UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration, TargetRules.LinkEnvironmentConfiguration LinkEnvironment)
		{
			TargetRules TargetRulesObject = Target.TargetRules;
			FileReference TargetFilePath = Target.TargetFilePath;
			string TargetName = TargetFilePath == null ? Project.ProjectFilePath.GetFileNameWithoutExtension() : TargetFilePath.GetFileNameWithoutAnyExtensions();
			string UBTPlatformName = Platform.ToString();
			string UBTConfigurationName = Configuration.ToString();

			string ProjectName = Project.ProjectFilePath.GetFileNameWithoutExtension();

			// Setup output path
			UEBuildPlatform BuildPlatform = UEBuildPlatform.GetBuildPlatform(Platform);

			// Figure out if this is a monolithic build
			bool bShouldCompileMonolithic = BuildPlatform.ShouldCompileMonolithicBinary(Platform);

			if (TargetRulesObject != null)
			{
				bShouldCompileMonolithic |= (Target.CreateRulesDelegate(Platform, Configuration).GetLegacyLinkType(Platform, Configuration) == TargetLinkType.Monolithic);
			}

			TargetType TargetRulesType = Target.TargetRules == null ? TargetType.Program : Target.TargetRules.Type;

			// Get the output directory
			DirectoryReference RootDirectory = UnrealBuildTool.EngineDirectory;
			if (TargetRulesType != TargetType.Program && (bShouldCompileMonolithic || TargetRulesObject.BuildEnvironment == TargetBuildEnvironment.Unique) && !TargetRulesObject.bOutputToEngineBinaries)
			{
				if(Target.UnrealProjectFilePath != null)
				{
					RootDirectory = Target.UnrealProjectFilePath.Directory;
				}
			}

			if (TargetRulesType == TargetType.Program && (TargetRulesObject == null || !TargetRulesObject.bOutputToEngineBinaries))
			{
				if(Target.UnrealProjectFilePath != null)
				{
					RootDirectory = Target.UnrealProjectFilePath.Directory;
				}
			}

			// Get the output directory
			DirectoryReference OutputDirectory = DirectoryReference.Combine(RootDirectory, "Binaries", UBTPlatformName);

			// Get the executable name (minus any platform or config suffixes)
			string BaseExeName = TargetName;
			if (!bShouldCompileMonolithic && TargetRulesType != TargetType.Program)
			{
				// Figure out what the compiled binary will be called so that we can point the IDE to the correct file
				string TargetConfigurationName = TargetRulesType.ToString();
				if (TargetConfigurationName != TargetType.Game.ToString() && TargetConfigurationName != TargetType.Program.ToString())
				{
					BaseExeName = "UE4" + TargetConfigurationName;
				}
			}

			// Make the output file path
			string ExecutableFilename = FileReference.Combine(OutputDirectory, BaseExeName).ToString();

			if ((Configuration != UnrealTargetConfiguration.Development) && ((Configuration !=  UnrealTargetConfiguration.DebugGame) || bShouldCompileMonolithic))
			{
				ExecutableFilename += "-" + UBTPlatformName + "-" + UBTConfigurationName;
			}
			ExecutableFilename += TargetRulesObject.Architecture;
			ExecutableFilename += BuildPlatform.GetBinaryExtension(UEBuildBinaryType.Executable);

			// Include the path to the actual executable for a Mac app bundle
			if (Platform == UnrealTargetPlatform.Mac && !LinkEnvironment.bIsBuildingConsoleApplication)
			{
				ExecutableFilename += ".app/Contents/MacOS/" + Path.GetFileName(ExecutableFilename);
			}

			return FileReference.MakeFromNormalizedFullPath(ExecutableFilename);
		}

		private void WriteNativeLaunchFile(VSCodeProject Project, JsonFile OutFile)
		{
			foreach (ProjectTarget Target in Project.ProjectTargets)
			{
				List<UnrealTargetConfiguration> Configs = new List<UnrealTargetConfiguration>();
				Target.TargetRules.GetSupportedConfigurations(ref Configs, true);

				TargetRules.CPPEnvironmentConfiguration CppEnvironment = new TargetRules.CPPEnvironmentConfiguration(Target.TargetRules);
				TargetRules.LinkEnvironmentConfiguration LinkEnvironment = new TargetRules.LinkEnvironmentConfiguration(Target.TargetRules);
				Target.TargetRules.SetupGlobalEnvironment(new TargetInfo(new ReadOnlyTargetRules(Target.TargetRules)), ref LinkEnvironment, ref CppEnvironment);

				foreach (UnrealTargetConfiguration Config in Configs)
				{
					if (SupportedConfigurations.Contains(Config))
					{
						FileReference Executable = GetExecutableFilename(Project, Target, HostPlatform, Config, LinkEnvironment);
						string Name = Target.TargetRules == null ? Project.ProjectFilePath.GetFileNameWithoutExtension() : Target.TargetRules.Name;
						string LaunchTaskName = String.Format("{0} {1} {2} Build", Name, HostPlatform, Config);
						string ExecutableDirectory = "${workspaceRoot}/" + Executable.Directory.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace("\\", "/");

						OutFile.BeginObject();
						{
							OutFile.AddField("name", Target.TargetRules.Name + " (" + Config.ToString() + ")");
							OutFile.AddField("request", "launch");
							OutFile.AddField("preLaunchTask", LaunchTaskName);
							OutFile.AddField("program", ExecutableDirectory + "/" + Executable.GetFileName());
							OutFile.BeginArray("args");
							{
								if (Target.TargetRules.Type == TargetRules.TargetType.Editor)
								{
									OutFile.AddUnnamedField(Project.ProjectFilePath.GetFileNameWithoutAnyExtensions());

									if (Config == UnrealTargetConfiguration.Debug || Config == UnrealTargetConfiguration.DebugGame)
									{
										OutFile.AddUnnamedField("-debug");
									}
								}

							}
							OutFile.EndArray();
							OutFile.AddField("stopAtEntry", false);
							OutFile.AddField("cwd", ExecutableDirectory);
							OutFile.BeginArray("environment");
							{

							}
							OutFile.EndArray();
							OutFile.AddField("externalConsole", true);

							switch (HostPlatform)
							{
								case UnrealTargetPlatform.Win64:
									{
										OutFile.AddField("type", "cppvsdbg");
										break;
									}

								default:
									{
										OutFile.AddField("type", "cppdbg");
										OutFile.AddField("MIMode", "lldb");
										break;
									}
							}
						}
						OutFile.EndObject();
					}
				}
			}
		}

		private void WriteCSharpLaunchConfig(VCSharpProjectFile Project, JsonFile OutFile)
		{
			if (BuildingForDotNetCore != Project.IsDotNETCoreProject())
			{
				return;
			}

			string FrameworkExecutableExtension = BuildingForDotNetCore ? ".dll" : ".exe";

			foreach (ProjectTarget Target in Project.ProjectTargets)
			{
				UnrealTargetConfiguration[] Configs = { UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development };
				foreach (UnrealTargetConfiguration Config in Configs)
				{
					CsProjectInfo ProjectInfo = Project.GetProjectInfo(Config);
					string AssemblyName;
					if (!ProjectInfo.Properties.TryGetValue("AssemblyName", out AssemblyName))
					{
						AssemblyName = Project.ProjectFilePath.GetFileNameWithoutExtension();

					}
					string TaskName = String.Format("{0} ({1})", AssemblyName, Config);
					string BuildTaskName = String.Format("{0} {1} {2} Build", AssemblyName, HostPlatform, Config);

					if (ProjectInfo.Properties.ContainsKey("OutputPath"))
					{
						DirectoryReference OutputPath = DirectoryReference.Combine(Project.ProjectFilePath.Directory, ProjectInfo.Properties["OutputPath"]);
						OutputPath.MakeRelativeTo(UnrealBuildTool.RootDirectory);
						HashSet<FileReference> BuildProducts = new HashSet<FileReference>();
						ProjectInfo.AddBuildProducts(OutputPath, BuildProducts);

						foreach (FileReference BuildProduct in BuildProducts)
						{
							if (BuildProduct.GetExtension() == FrameworkExecutableExtension)
							{
								OutFile.BeginObject();
								{
									OutFile.AddField("name", TaskName);

									if (Project.IsDotNETCoreProject())
									{
										OutFile.AddField("type", "coreclr");
									}
									else
									{
										if (HostPlatform == UnrealTargetPlatform.Win64)
										{
											OutFile.AddField("type", "clr");
										}
										else
										{
											OutFile.AddField("type", "mono");
										}
									}

									OutFile.AddField("request", "launch");
									OutFile.AddField("preLaunchTask", BuildTaskName);

									if (Project.IsDotNETCoreProject())
									{
										OutFile.AddField("program", "dotnet");
										OutFile.BeginArray("args");
										{
											OutFile.AddUnnamedField("${workspaceRoot}/" + BuildProduct.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace('\\', '/'));
										}
										OutFile.EndArray();
										OutFile.AddField("externalConsole", true);
										OutFile.AddField("stopAtEntry", false);
									}
									else
									{
										OutFile.AddField("program", "${workspaceRoot}/" + BuildProduct.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace('\\', '/'));
										OutFile.BeginArray("args");
										{
										}
										OutFile.EndArray();

										if (HostPlatform == UnrealTargetPlatform.Win64)
										{
											OutFile.AddField("console", "externalTerminal");
										}
										else
										{
											OutFile.AddField("console", "internalConsole");
										}

										OutFile.AddField("internalConsoleOptions", "openOnSessionStart");
									}

									OutFile.AddField("cwd", "${workspaceRoot}");
								}
								OutFile.EndObject();
							}
						}
					}
				}
			}
		}

		private void WriteLaunchFile(List<ProjectFile> Projects)
		{
			JsonFile OutFile = new JsonFile();

			OutFile.BeginRootObject();
			{
				OutFile.AddField("version", "0.2.0");
				OutFile.BeginArray("configurations");
				{
					foreach (ProjectFile Project in Projects)
					{
						if (Project is VSCodeProject)
						{
							WriteNativeLaunchFile(Project as VSCodeProject, OutFile);
						}
						else if (Project is VCSharpProjectFile)
						{
							WriteCSharpLaunchConfig(Project as VCSharpProjectFile, OutFile);
						}
					}
				}
				OutFile.EndArray();
			}
			OutFile.EndRootObject();

			OutFile.Write(FileReference.Combine(VSCodeDir, "launch.json"));
		}

		protected override void ConfigureProjectFileGeneration(string[] Arguments, ref bool IncludeAllPlatforms)
		{
			base.ConfigureProjectFileGeneration(Arguments, ref IncludeAllPlatforms);
		}

		private void WriteWorkspaceSettingsFile(List<ProjectFile> Projects)
		{
			JsonFile OutFile = new JsonFile();

			List<string> PathsToExclude = new List<string>();

			PathsToExclude.Add(".vs");
			PathsToExclude.Add("*.p4*");
			PathsToExclude.Add("**/*.png");
			PathsToExclude.Add("**/*.bat");
			PathsToExclude.Add("**/*.sh");
			PathsToExclude.Add("**/*.command");
			PathsToExclude.Add("**/*.sln");
			PathsToExclude.Add("**/*.db");
			PathsToExclude.Add("**/*.csproj.user");
			PathsToExclude.Add("**/*.relinked_action_ran");
			PathsToExclude.Add("**/*.vsscc");
			PathsToExclude.Add("**/*.link");
			PathsToExclude.Add("**/*.uprojectdirs");
			PathsToExclude.Add("packages");

			foreach (ProjectFile Project in Projects)
			{
				bool bFoundTarget = false;
				foreach (ProjectTarget Target in Project.ProjectTargets)
				{
					if (Target.TargetFilePath != null)
					{
						DirectoryReference ProjDir = Target.TargetFilePath.Directory.GetDirectoryName() == "Source" ? Target.TargetFilePath.Directory.ParentDirectory : Target.TargetFilePath.Directory;
						GetExcludePathsCPP(ProjDir, PathsToExclude);
						bFoundTarget = true;
					}
				}

				if (!bFoundTarget)
				{
					GetExcludePathsCSharp(Project.ProjectFilePath.Directory.ToString(), PathsToExclude);
				}
			}

			OutFile.BeginRootObject();
			{
				OutFile.BeginObject("files.exclude");
				{
					string WorkspaceRoot = UnrealBuildTool.RootDirectory.ToString().Replace('\\', '/') + "/";
					foreach (string PathToExclude in PathsToExclude)
					{
						OutFile.AddField(PathToExclude.Replace('\\', '/').Replace(WorkspaceRoot, ""), true);
					}
				}
				OutFile.EndObject();
			}
			OutFile.EndRootObject();

			OutFile.Write(FileReference.Combine(VSCodeDir, "settings.json"));
		}

		private void GetExcludePathsCPP(DirectoryReference BaseDir, List<string> PathsToExclude)
		{
			string[] DirWhiteList = { "Build", "Config", "Plugins", "Source", "Private", "Public", "Classes", "Resources" };
			foreach (DirectoryReference SubDir in DirectoryReference.EnumerateDirectories(BaseDir, "*", SearchOption.TopDirectoryOnly))
			{
				if (Array.Find(DirWhiteList, Dir => Dir == SubDir.GetDirectoryName()) == null)
				{
					string NewSubDir = SubDir.ToString();
					if (!PathsToExclude.Contains(NewSubDir))
					{
						PathsToExclude.Add(NewSubDir);
					}
				}
			}
		}

		private void GetExcludePathsCSharp(string BaseDir, List<string> PathsToExclude)
		{
			string[] BlackList =
			{
				"obj",
				"bin"
			};

			foreach (string BlackListDir in BlackList)
			{
				string ExcludePath = Path.Combine(BaseDir, BlackListDir);
				if (!PathsToExclude.Contains(ExcludePath))
				{
					PathsToExclude.Add(ExcludePath);
				}
			}
		}
	}
}