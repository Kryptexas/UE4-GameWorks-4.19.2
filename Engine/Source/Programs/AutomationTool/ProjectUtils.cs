// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using UnrealBuildTool;

namespace AutomationTool
{
	public struct SingleTargetProperties
	{
		public string TargetName;
		public TargetRules Rules;
	}

	/// <summary>
	/// Autodetected project properties.
	/// </summary>
	public class ProjectProperties
	{
		/// <summary>
		/// Full Project path. Must be a .uproject file
		/// </summary>
		public string RawProjectPath;

		/// <summary>
		/// True if the uproject contains source code.
		/// </summary>
		public bool bIsCodeBasedProject;

		/// <summary>
		/// Whether the project uses Steam (todo: substitute with more generic functionality)
		/// </summary>
		public bool bUsesSteam;

		/// <summary>
		/// Whether the project uses visual Slate UI (as opposed to the low level windowing/messaging which is always used)
		/// </summary>
		public bool bUsesSlate = true;

		/// <summary>
		/// Hack for legacy game styling isses.  No new project should ever set this to true
		/// Whether the project uses the Slate editor style in game.  
		/// </summary>
		public bool bUsesSlateEditorStyle = false;

		/// <summary>
		/// List of all targets detected for this project.
		/// </summary>
		public Dictionary<TargetRules.TargetType, SingleTargetProperties> Targets = new Dictionary<TargetRules.TargetType, SingleTargetProperties>();

		/// <summary>
		/// List of all programs detected for this project.
		/// </summary>
		public List<SingleTargetProperties> Programs = new List<SingleTargetProperties>();

		internal ProjectProperties()
		{
		}
	}

	/// <summary>
	/// Project related utility functions.
	/// </summary>
	public class ProjectUtils
	{
		private static CaselessDictionary<ProjectProperties> PropertiesCache = new CaselessDictionary<ProjectProperties>();

		/// <summary>
		/// Gets a short project name (QAGame, Elemental, etc)
		/// </summary>
		/// <param name="RawProjectPath">Full project path.</param>
		/// <param name="bIsUProjectFile">True if a uproject.</param>
		/// <returns>Short project name</returns>
		public static string GetShortProjectName(string RawProjectPath)
		{
			return CommandUtils.GetFilenameWithoutAnyExtensions(RawProjectPath);
		}

		/// <summary>
		/// Gets project properties.
		/// </summary>
		/// <param name="RawProjectPath">Full project path.</param>
		/// <returns>Properties of the project.</returns>
		public static ProjectProperties GetProjectProperties(string RawProjectPath)
		{
			string ProjectKey = "UE4";
			if (!String.IsNullOrEmpty(RawProjectPath))
			{
				ProjectKey = CommandUtils.ConvertSeparators(PathSeparator.Slash, Path.GetFullPath(RawProjectPath));
			}
			ProjectProperties Properties;
			if (PropertiesCache.TryGetValue(ProjectKey, out Properties) == false)
			{
				Properties = DetectProjectProperties(RawProjectPath);
				PropertiesCache.Add(ProjectKey, Properties);
			}
			return Properties;
		}

		/// <summary>
		/// Checks if the project is a UProject file with source code.
		/// </summary>
		/// <param name="RawProjectPath">Full project path.</param>
		/// <returns>True if the project is a UProject file with source code.</returns>
		public static bool IsCodeBasedUProjectFile(string RawProjectPath)
		{
			return GetProjectProperties(RawProjectPath).bIsCodeBasedProject;
		}

		/// <summary>
		/// Returns a path to the client binaries folder.
		/// </summary>
		/// <param name="RawProjectPath">Full project path.</param>
		/// <param name="Platform">Platform type.</param>
		/// <returns>Path to the binaries folder.</returns>
		public static string GetProjectClientBinariesFolder(string ProjectClientBinariesPath, UnrealTargetPlatform Platform = UnrealTargetPlatform.Unknown)
		{
			if (Platform != UnrealTargetPlatform.Unknown)
			{
				ProjectClientBinariesPath = CommandUtils.CombinePaths(ProjectClientBinariesPath, Platform.ToString());
			}
			return ProjectClientBinariesPath;
		}

		/// <summary>
		/// Attempts to autodetect project properties.
		/// </summary>
		/// <param name="RawProjectPath">Full project path.</param>
		/// <returns>Project properties.</returns>
		private static ProjectProperties DetectProjectProperties(string RawProjectPath)
		{
			var Properties = new ProjectProperties();
			Properties.RawProjectPath = RawProjectPath;

			if (CommandUtils.CmdEnv.HasCapabilityToCompile)
			{
				DetectTargetsForProject(Properties);
				Properties.bIsCodeBasedProject = !CommandUtils.IsNullOrEmpty(Properties.Targets);
			}
			else
			{
				// should never ask for engine targets if we can't compile
				if (String.IsNullOrEmpty(RawProjectPath))
				{
					throw new AutomationException("Cannot dtermine engine targets if we can't compile.");
				}

				Properties.bIsCodeBasedProject = false;
				// if there's a Source directory with source code in it, then mark us as having source code
				string SourceDir = CommandUtils.CombinePaths(Path.GetDirectoryName(RawProjectPath), "Source");
				if (Directory.Exists(SourceDir))
				{
					string[] CppFiles = Directory.GetFiles(SourceDir, "*.cpp", SearchOption.AllDirectories);
					string[] HFiles = Directory.GetFiles(SourceDir, "*.h", SearchOption.AllDirectories);
					Properties.bIsCodeBasedProject = CppFiles.Length > 0 || HFiles.Length > 0;
				}
			}

			return Properties;
		}


		/// <summary>
		/// Gets the project's root binaries folder.
		/// </summary>
		/// <param name="RawProjectPath">Full project path.</param>
		/// <param name="TargetType">Target type.</param>
		/// <param name="bIsUProjectFile">True if uproject file.</param>
		/// <returns>Binaries path.</returns>
		public static string GetClientProjectBinariesRootPath(string RawProjectPath, TargetRules.TargetType TargetType, bool bIsCodeBasedProject)
		{
			var BinPath = String.Empty;
			switch (TargetType)
			{
				case TargetRules.TargetType.Program:
					BinPath = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Binaries");
					break;
				case TargetRules.TargetType.Client:
				case TargetRules.TargetType.Game:
				case TargetRules.TargetType.RocketGame:
					if (!bIsCodeBasedProject)
					{
						BinPath = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Binaries");
					}
					else
					{
						BinPath = CommandUtils.CombinePaths(CommandUtils.GetDirectoryName(RawProjectPath), "Binaries");
					}
					break;
			}
			return BinPath;
		}

		/// <summary>
		/// Finds all targets for the project.
		/// </summary>
		/// <param name="Properties">Project properties.</param>
		/// <param name="ExtraSearchPaths">Additional search paths.</param>
		private static void DetectTargetsForProject(ProjectProperties Properties, List<string> ExtraSearchPaths = null)
		{
			Properties.Targets = new Dictionary<TargetRules.TargetType, SingleTargetProperties>();
			string TargetsDllFilename;
			string FullProjectPath = null;

			var GameFolders = new List<string>();
			if (!String.IsNullOrEmpty(Properties.RawProjectPath))
			{
				CommandUtils.Log("Looking for targets for project {0}", Properties.RawProjectPath);

				TargetsDllFilename = CommandUtils.CombinePaths(Path.GetTempPath(), String.Format("UATRules{0}.dll", Properties.RawProjectPath.GetHashCode()));

				FullProjectPath = CommandUtils.GetDirectoryName(Properties.RawProjectPath);
				GameFolders.Add(FullProjectPath);
				CommandUtils.Log("Searching for target rule files in {0}", FullProjectPath);
			}
			else
			{
				TargetsDllFilename = CommandUtils.CombinePaths(Path.GetTempPath(), String.Format("UATRules{0}.dll", "_BaseEngine_"));
			}
			RulesCompiler.SetAssemblyNameAndGameFolders(TargetsDllFilename, GameFolders);

			// the UBT code assumes a certain CWD, but artists don't have this CWD.
			var SourceDir = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Source");
			bool DirPushed = false;
			if (CommandUtils.DirectoryExists_NoExceptions(SourceDir))
			{
				CommandUtils.PushDir(SourceDir);
				DirPushed = true;
			}
			var TargetScripts = RulesCompiler.FindAllRulesSourceFiles(RulesCompiler.RulesFileType.Target, ExtraSearchPaths);
			if (DirPushed)
			{
				CommandUtils.PopDir();
			}

			if (!CommandUtils.IsNullOrEmpty(TargetScripts))
			{
				// We only care about project target script so filter out any scripts not in the project folder, or take them all if we are just doing engine stuff
				var ProjectTargetScripts = new List<string>();
				foreach (var Filename in TargetScripts)
				{
					var FullScriptPath = CommandUtils.CombinePaths(Path.GetFullPath(Filename));
					if (FullProjectPath == null || FullScriptPath.StartsWith(FullProjectPath, StringComparison.InvariantCultureIgnoreCase))
					{
						ProjectTargetScripts.Add(FullScriptPath);
					}
				}
				TargetScripts = ProjectTargetScripts;
			}

			if (!CommandUtils.IsNullOrEmpty(TargetScripts))
			{
				CommandUtils.LogVerbose("Found {0} target rule files:", TargetScripts.Count);
				foreach (var Filename in TargetScripts)
				{
					CommandUtils.LogVerbose("  {0}", Filename);
				}

				// Check if the scripts require compilation
				bool DoNotCompile = false;
				if (!CommandUtils.IsBuildMachine && !CheckIfScriptAssemblyIsOutOfDate(TargetsDllFilename, TargetScripts))
				{
					Log.TraceInformation("Targets DLL {0} is up to date.", TargetsDllFilename);
					DoNotCompile = true;
				}
				if (!DoNotCompile && CommandUtils.FileExists_NoExceptions(TargetsDllFilename))
				{
					if (!CommandUtils.DeleteFile_NoExceptions(TargetsDllFilename, true))
					{
						DoNotCompile = true;
						CommandUtils.Log("Could not delete {0} assuming it is up to date and reusable for a recursive UAT call.", TargetsDllFilename);
					}
				}

				CompileAndLoadTargetsAssembly(Properties, TargetsDllFilename, DoNotCompile, TargetScripts);
			}
		}

		/// <summary>
		/// Optionally compiles and loads target rules assembly.
		/// </summary>
		/// <param name="Properties"></param>
		/// <param name="TargetsDllFilename"></param>
		/// <param name="DoNotCompile"></param>
		/// <param name="TargetScripts"></param>
		private static void CompileAndLoadTargetsAssembly(ProjectProperties Properties, string TargetsDllFilename, bool DoNotCompile, List<string> TargetScripts)
		{
			CommandUtils.Log("Compiling targets DLL: {0}", TargetsDllFilename);
			var ReferencedAssemblies = new List<string>() 
					{ 
						"System.dll", 
						"System.Core.dll", 
						"System.Xml.dll", 
						typeof(UnrealBuildTool.UnrealBuildTool).Assembly.Location
					};
			var TargetsDLL = DynamicCompilation.CompileAndLoadAssembly(TargetsDllFilename, TargetScripts, ReferencedAssemblies, null, DoNotCompile);
			var DummyTargetInfo = new TargetInfo(UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
			var AllCompiledTypes = TargetsDLL.GetTypes();
			foreach (Type TargetType in AllCompiledTypes)
			{
				// Find TargetRules but skip all "UE4Editor", "UE4Game" targets.
				if (typeof(TargetRules).IsAssignableFrom(TargetType))
				{
					// Create an instance of this type
					CommandUtils.LogVerbose("Creating target rules object: {0}", TargetType.Name);
					var Rules = Activator.CreateInstance(TargetType, DummyTargetInfo) as TargetRules;
					CommandUtils.LogVerbose("Adding target: {0} ({1})", TargetType.Name, Rules.Type);

					SingleTargetProperties TargetData;
					TargetData.TargetName = GetTargetName(TargetType);
                    Rules.TargetName = TargetData.TargetName;
					TargetData.Rules = Rules;
					if (Rules.Type == TargetRules.TargetType.Program)
					{
						Properties.Programs.Add(TargetData);
					}
					else
					{
						Properties.Targets.Add(Rules.Type, TargetData);
					}

					Properties.bUsesSteam |= Rules.bUsesSteam;
					Properties.bUsesSlate |= Rules.bUsesSlate;
					Properties.bUsesSlateEditorStyle |= Rules.bUsesSlateEditorStyle;
				}
			}
		}

		/// <summary>
		/// Checks if any of the script files in newer than the generated assembly.
		/// </summary>
		/// <param name="TargetsDllFilename"></param>
		/// <param name="TargetScripts"></param>
		/// <returns>True if the generated assembly is out of date.</returns>
		private static bool CheckIfScriptAssemblyIsOutOfDate(string TargetsDllFilename, List<string> TargetScripts)
		{
			var bOutOfDate = false;
			var AssemblyInfo = new FileInfo(TargetsDllFilename);
			if (AssemblyInfo.Exists)
			{
				foreach (var ScriptFilename in TargetScripts)
				{
					var ScriptInfo = new FileInfo(ScriptFilename);
					if (ScriptInfo.Exists && ScriptInfo.LastWriteTimeUtc > AssemblyInfo.LastWriteTimeUtc)
					{
						bOutOfDate = true;
						break;
					}
				}
			}
			else
			{
				bOutOfDate = true;
			}
			return bOutOfDate;
		}

		/// <summary>
		/// Converts class type name (usually ends with Target) to a target name (without the postfix).
		/// </summary>
		/// <param name="TargetRulesType">Tagert class.</param>
		/// <returns>Target name</returns>
		private static string GetTargetName(Type TargetRulesType)
		{
			const string TargetPostfix = "Target";
			var Name = TargetRulesType.Name;
			if (Name.EndsWith(TargetPostfix, StringComparison.InvariantCultureIgnoreCase))
			{
				Name = Name.Substring(0, Name.Length - TargetPostfix.Length);
			}
			return Name;
		}
	}
}
