// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using System.Linq;
using System.Text.RegularExpressions;
using Tools.DotNETCommon;

[Help("Lists TPS files associated with any source used to build a specified target(s). Grabs TPS files associated with source modules, content, and engine shaders.")]
[Help("Target", "One or more UBT command lines to enumerate associated TPS files for (eg. UE4Game Win64 Development).")]
class ListThirdPartySoftware : BuildCommand
{
	public override void ExecuteBuild()
	{
		CommandUtils.Log("************************* List Third Party Software");

		string ProjectPath = ParseParamValue("Project", String.Empty);

		//Add quotes to avoid issues with spaces in project path
		if (ProjectPath != String.Empty)
			ProjectPath = "\"" + ProjectPath + "\"";

		// Parse the list of targets to list TPS for. Each target is specified by -Target="Name|Configuration|Platform" on the command line.
		HashSet<FileReference> TpsFiles = new HashSet<FileReference>();
		foreach(string Target in ParseParamValues(Params, "Target"))
		{
			// Get the path to store the exported JSON target data
			FileReference OutputFile = FileReference.Combine(CommandUtils.EngineDirectory, "Intermediate", "Build", "ThirdParty.json");

			IProcessResult Result;

			Result = Run(UE4Build.GetUBTExecutable(), String.Format("{0} {1} -jsonexport=\"{2}\" -skipbuild", Target.Replace('|', ' '),  ProjectPath, OutputFile.FullName), Options: ERunOptions.Default);

			if (Result.ExitCode != 0)
			{
				throw new AutomationException("Failed to run UBT");
			}

			// Read the exported target info back in
			JsonObject Object = JsonObject.Read(OutputFile);

			// Get the project file if there is one
			FileReference ProjectFile = null;
			string ProjectFileName;
			if(Object.TryGetStringField("ProjectFile", out ProjectFileName))
			{
				ProjectFile = new FileReference(ProjectFileName);
			}

			// Get the default paths to search
			HashSet<DirectoryReference> DirectoriesToScan = new HashSet<DirectoryReference>();
			DirectoriesToScan.Add(DirectoryReference.Combine(CommandUtils.EngineDirectory, "Shaders"));
			DirectoriesToScan.Add(DirectoryReference.Combine(CommandUtils.EngineDirectory, "Content"));
			if(ProjectFile != null)
			{
				DirectoriesToScan.Add(DirectoryReference.Combine(ProjectFile.Directory, "Content"));
			}

			// Get the variables to be expanded in any runtime dependencies variables
			Dictionary<string, string> Variables = new Dictionary<string, string>();
			Variables.Add("EngineDir", CommandUtils.EngineDirectory.FullName);
			if(ProjectFile != null)
			{
				Variables.Add("ProjectDir", ProjectFile.Directory.FullName);
			}

			// Add all the paths for each module, and its runtime dependencies
			JsonObject Modules = Object.GetObjectField("Modules");
			foreach(string ModuleName in Modules.KeyNames)
			{
				JsonObject Module = Modules.GetObjectField(ModuleName);
				DirectoriesToScan.Add(new DirectoryReference(Module.GetStringField("Directory")));

				foreach(JsonObject RuntimeDependency in Module.GetObjectArrayField("RuntimeDependencies"))
				{
					string RuntimeDependencyPath = RuntimeDependency.GetStringField("Path");
					RuntimeDependencyPath = Utils.ExpandVariables(RuntimeDependencyPath, Variables);
					DirectoriesToScan.Add(new FileReference(RuntimeDependencyPath).Directory);
				}
			}

			// Remove any directories that are under other directories, and sort the output list
			List<DirectoryReference> SortedDirectoriesToScan = new List<DirectoryReference>();
			foreach(DirectoryReference DirectoryToScan in DirectoriesToScan.OrderBy(x => x.FullName))
			{
				if(SortedDirectoriesToScan.Count == 0 || !DirectoryToScan.IsUnderDirectory(SortedDirectoriesToScan[SortedDirectoriesToScan.Count - 1]))
				{
					SortedDirectoriesToScan.Add(DirectoryToScan);
				}
			}

			// Get the platforms to exclude
			List<UnrealTargetPlatform> SupportedPlatforms = new List<UnrealTargetPlatform> { (UnrealTargetPlatform)Enum.Parse(typeof(UnrealTargetPlatform), Object.GetStringField("Platform")) };
			FileSystemName[] ExcludePlatformNames = Utils.MakeListOfUnsupportedPlatforms(SupportedPlatforms).Select(x => new FileSystemName(x)).ToArray();

			// Find all the TPS files under the engine directory which match
			foreach(DirectoryReference DirectoryToScan in SortedDirectoriesToScan)
			{
				foreach(FileReference TpsFile in DirectoryReference.EnumerateFiles(DirectoryToScan, "*.tps", SearchOption.AllDirectories))
				{
					if(!TpsFile.ContainsAnyNames(ExcludePlatformNames, DirectoryToScan))
					{
						TpsFiles.Add(TpsFile);
					}
				}
			}
		}

		// Also add any redirects
		List<string> OutputMessages = new List<string>();
		foreach(FileReference TpsFile in TpsFiles)
		{
			string Message = TpsFile.FullName;

			string[] Lines = FileReference.ReadAllLines(TpsFile);
			foreach(string Line in Lines)
			{
				const string RedirectPrefix = "Redirect:";

				int Idx = Line.IndexOf(RedirectPrefix, StringComparison.InvariantCultureIgnoreCase);
				if(Idx >= 0)
				{
					FileReference RedirectTpsFile = FileReference.Combine(TpsFile.Directory, Line.Substring(Idx + RedirectPrefix.Length).Trim());
					Message = String.Format("{0} (redirect from {1})", RedirectTpsFile.FullName, TpsFile.FullName);
					break;
				}
			}

			OutputMessages.Add(Message);
		}
		OutputMessages.Sort();

		// Print them all out
		foreach(string OutputMessage in OutputMessages)
		{
			CommandUtils.Log(OutputMessage);
		}
	}
}
