// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

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

		public VSCodeProjectFileGenerator(FileReference InOnlyGameProject)
			: base(InOnlyGameProject)
		{
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
			return false;
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

			WriteCppPropertiesFile();
			WriteTasksFile();
			WriteLaunchFile();

			return true;
		}

		private void WriteCppPropertiesFile()
		{
			JsonFile OutFile = new JsonFile();

			List<string> PathList = new List<string>();
			foreach (ProjectFile Project in GeneratedProjectFiles)
			{
				foreach (ProjectFile.SourceFile SourceFile in Project.SourceFiles)
				{
					if (SourceFile.Reference.GetExtension() == ".h")
					{
						string Path = "${workspaceRoot}/" + SourceFile.Reference.Directory.MakeRelativeTo(UnrealBuildTool.RootDirectory).Replace('\\', '/');
						if (!PathList.Contains(Path))
						{
							PathList.Add(Path);
						}
					}
				}
			}

			PathList.Sort();

			string[] Paths = PathList.ToArray();

			OutFile.BeginRootObject();
			{
				OutFile.BeginArray("configurations");
				{
					OutFile.BeginObject();
					{
						OutFile.AddField("name", "Win32");

						OutFile.BeginArray("includePath");
						{
							foreach (var Path in Paths)
							{
								OutFile.AddUnnamedField(Path);
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

		private void WriteTasksFile()
		{
			JsonFile OutFile = new JsonFile();

			OutFile.BeginRootObject();
			{
				OutFile.AddField("version", "2.0.0");

				OutFile.BeginArray("tasks");
				{
					string[] Commands = { "Build", "Clean" };

					foreach (ProjectFile Project in AllProjectFiles)
					{
						if (Project.ProjectFilePath.GetExtension().Contains(ProjectFileExtension))
						{
							List<PlatformAndConfigToBuild> ToBuildList = new List<PlatformAndConfigToBuild>();
							GatherBuildTargets(Project, ToBuildList);

							foreach (PlatformAndConfigToBuild ToBuild in ToBuildList)
							{
								foreach (string Command in Commands)
								{
									string TaskName = $"{ToBuild.ProjectName} {ToBuild.Platform.ToString()} {ToBuild.Config} {Command}";
									List<string> ExtraParams = new List<string>();
									string BatchFilename = null;

									switch (HostPlatform)
									{
										case UnrealTargetPlatform.Win64:
											{
												BatchFilename = "${workspaceRoot}/Engine/Build/BatchFiles/" + Command + ".bat";
												break;
											}
										case UnrealTargetPlatform.Mac:
											{

												BatchFilename = "${workspaceRoot}/Engine/Build/BatchFiles/Mac/Buiild.sh";
												break;
											}
										case UnrealTargetPlatform.Linux:
											{
												BatchFilename = "${workspaceRoot}/Engine/Build/BatchFiles/Linux/Build.sh";
												break;
											}
									}

									OutFile.BeginObject();
									{
										OutFile.AddField("taskName", TaskName);
										OutFile.AddField("command", BatchFilename);
										OutFile.BeginArray("args");
										{
											OutFile.AddUnnamedField(ToBuild.ProjectName);
											OutFile.AddUnnamedField(ToBuild.Platform.ToString());
											OutFile.AddUnnamedField(ToBuild.Config.ToString());
											OutFile.AddUnnamedField("-waitmutex");

											foreach (string ExtraParam in ExtraParams)
											{
												OutFile.AddUnnamedField(ExtraParam);
											}
										}
										OutFile.EndArray();
										OutFile.AddField("problemMatcher", "$msCompile");
									}
									OutFile.EndObject();
								}
							}
						}
						else if (Project.ProjectFilePath.GetExtension() == ".csproj")
						{
							string CSProjectPath = Project.ProjectFilePath.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace('\\', '/');
							foreach (ProjectTarget Target in Project.ProjectTargets)
							{
								UnrealTargetConfiguration[] CSharpConfigs = { UnrealTargetConfiguration.Debug, UnrealTargetConfiguration.Development };
								foreach (UnrealTargetConfiguration Config in CSharpConfigs)
								{
									foreach (string Command in Commands)
									{
										string TaskName = $"{Project.ProjectFilePath.GetFileNameWithoutExtension()} {HostPlatform} {Config} {Command}";

										OutFile.BeginObject();
										{
											OutFile.AddField("taskName", TaskName);
											if (HostPlatform == UnrealTargetPlatform.Win64)
											{
												OutFile.AddField("command", "dotnet");												
											}
											else
											{
												OutFile.AddField("command", "xbuild");
											}
											OutFile.BeginArray("args");
											{
												if (HostPlatform == UnrealTargetPlatform.Win64)
												{
													OutFile.AddUnnamedField(Command.ToLower());
												}
												else
												{
													OutFile.AddUnnamedField("/t:" + Command.ToLower());
												}
												OutFile.AddUnnamedField(CSProjectPath);
											}
											OutFile.EndArray();
										}
										OutFile.AddField("problemMatcher", "$msCompile");
										OutFile.EndObject();
									}
								}
							}
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

		private FileReference GetExecutableFilename(ProjectFile Project, ProjectTarget Target, UnrealTargetPlatform Platform, UnrealTargetConfiguration Configuration)
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
				if (OnlyGameProject != null && TargetFilePath.IsUnderDirectory(OnlyGameProject.Directory))
				{
					RootDirectory = OnlyGameProject.Directory;
				}
				else
				{
					FileReference ProjectFileName;
					if (UProjectInfo.TryGetProjectFileName(ProjectName, out ProjectFileName))
					{
						RootDirectory = ProjectFileName.Directory;
					}
				}
			}

			if (TargetRulesType == TargetType.Program && (TargetRulesObject == null || !TargetRulesObject.bOutputToEngineBinaries))
			{
				FileReference ProjectFileName;
				if (UProjectInfo.TryGetProjectForTarget(TargetName, out ProjectFileName))
				{
					RootDirectory = ProjectFileName.Directory;
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
			if ((Configuration != UnrealTargetConfiguration.DebugGame || bShouldCompileMonolithic))
			{
				ExecutableFilename += "-" + UBTPlatformName + "-" + UBTConfigurationName;
			}
			ExecutableFilename += TargetRulesObject.Architecture;
			ExecutableFilename += BuildPlatform.GetBinaryExtension(UEBuildBinaryType.Executable);

			return FileReference.MakeFromNormalizedFullPath(ExecutableFilename);
		}

		private enum EDebugMode
		{
			GDB,
			VS
		}
		
		private void WriteLaunchFile()
		{
			JsonFile OutFile = new JsonFile();
			EDebugMode DebugMode = (HostPlatform == UnrealTargetPlatform.Win64) ? EDebugMode.VS : EDebugMode.GDB;

			OutFile.BeginRootObject();
			{
				OutFile.AddField("version", "0.2.0");
				OutFile.BeginArray("configurations");
				{
					List<ProjectFile> Projects = new List<ProjectFile>(GeneratedProjectFiles);
					Projects.Sort((A, B) => { return A.ProjectFilePath.GetFileNameWithoutExtension().CompareTo(B.ProjectFilePath.GetFileNameWithoutExtension()); });

					foreach (ProjectFile Project in AllProjectFiles)
					{
						if (Project.ProjectFilePath.GetExtension() == ProjectFileExtension)
						{
							foreach (ProjectTarget Target in Project.ProjectTargets)
							{
								List<UnrealTargetConfiguration> Configs = new List<UnrealTargetConfiguration>();
								Target.TargetRules.GetSupportedConfigurations(ref Configs, true);

								foreach (UnrealTargetConfiguration Config in Configs)
								{
									if (SupportedConfigurations.Contains(Config))
									{
										FileReference Executable = GetExecutableFilename(Project, Target, HostPlatform, Config);
										string Name = Target.TargetRules == null ? Project.ProjectFilePath.GetFileNameWithoutExtension() : Target.TargetRules.Name;
										string LaunchTaskName = $"{Name} {HostPlatform} {Config} Build";
										string ExecutableDirectory = "${workspaceRoot}/" + Executable.Directory.MakeRelativeTo(UnrealBuildTool.RootDirectory).ToString().Replace("\\", "/");

										OutFile.BeginObject();
										{
											OutFile.AddField("name", Target.TargetRules.Name + " (" + Config.ToString() + ")");										
											OutFile.AddField("request", "launch");
											OutFile.AddField("preLaunchTask", LaunchTaskName);
											OutFile.AddField("program", ExecutableDirectory + "/" + Executable.GetFileName());

											OutFile.BeginArray("args");
											{

											}
											OutFile.EndArray();
											OutFile.AddField("stopAtEntry", true);
											OutFile.AddField("cwd", ExecutableDirectory);
											OutFile.BeginArray("environment");
											{

											}
											OutFile.EndArray();
											OutFile.AddField("externalConsole", true);

											switch (DebugMode)
											{
												case EDebugMode.VS:
													{
														OutFile.AddField("type", "cppvsdbg");
														break;
													}

												case EDebugMode.GDB:
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
						else if (Project.ProjectFilePath.GetExtension() == ".csproj")
						{
							/*
							foreach (ProjectTarget Target in Project.ProjectTargets)
							{
								foreach (UnrealTargetConfiguration Config in Target.ExtraSupportedConfigurations)
								{
									string TaskName = $"{Project.ProjectFilePath.GetFileNameWithoutExtension()} ({Config})";
									string BuildTaskName = $"{Project.ProjectFilePath.GetFileNameWithoutExtension()} {HostPlatform} {Config} Build";
									FileReference Executable = GetExecutableFilename(Project, Target, HostPlatform, Config);

									OutFile.BeginObject();
									{
										OutFile.AddField("name", TaskName);
										OutFile.AddField("type", "coreclr");
										OutFile.AddField("request", "launch");
										OutFile.AddField("preLaunchTask", BuildTaskName);
										OutFile.AddField("program", Executable.ToString());
										OutFile.BeginArray("args");
										{

										}
										OutFile.EndArray();
									}
									OutFile.EndObject();
								}
							}
							 */
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
	}
}