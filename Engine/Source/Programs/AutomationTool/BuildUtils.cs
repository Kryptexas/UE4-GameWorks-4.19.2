// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace AutomationTool
{

	public partial class CommandUtils
	{
		/// <summary>
		/// Runs msbuild.exe with the specified arguments. Automatically creates a logfile. When 
		/// no LogName is specified, the executable name is used as logfile base name.
		/// </summary>
		/// <param name="Env">BuildEnvironment to use.</param>
		/// <param name="Project">Path to the project to build.</param>
		/// <param name="Arguments">Arguments to pass to msbuild.exe.</param>
		/// <param name="LogName">Optional logfile name.</param>
		public static void MsBuild(CommandEnvironment Env, string Project, string Arguments, string LogName)
		{
			if (String.IsNullOrEmpty(Env.MsBuildExe))
			{
				throw new AutomationException("Unable to find msbuild.exe at: \"{0}\"", Env.MsBuildExe);
			}
			if (!FileExists(Project))
			{
				throw new AutomationException("Project {0} does not exist!", Project);
			}
			var RunArguments = MakePathSafeToUseWithCommandLine(Project);
			if (!String.IsNullOrEmpty(Arguments))
			{
				RunArguments += " " + Arguments;
			}
			RunAndLog(Env, Env.MsBuildExe, RunArguments, LogName);
		}

		/// <summary>
		/// Builds a Visual Studio solution with MsDevEnv. Automatically creates a logfile. When 
		/// no LogName is specified, the executable name is used as logfile base name.
		/// </summary>
		/// <param name="Env">BuildEnvironment to use.</param>
		/// <param name="SolutionFile">Path to the solution file</param>
		/// <param name="BuildConfig">Configuration to build.</param>
		/// <param name="LogName">Optional logfile name.</param>
		public static void BuildSolution(CommandEnvironment Env, string SolutionFile, string BuildConfig = "Development", string LogName = null)
		{
			if (!FileExists(SolutionFile))
			{
				throw new AutomationException(String.Format("Unabled to build Solution {0}. Solution file not found.", SolutionFile));
			}
			if (Env.MsDevExe == null)
			{
				throw new AutomationException(String.Format("Unabled to build Solution {0}. devenv.com not found.", SolutionFile));
			}
			string CmdLine = String.Format("\"{0}\" /build \"{1}\"", SolutionFile, BuildConfig);
			RunAndLog(Env, Env.MsDevExe, CmdLine, LogName);
		}

		/// <summary>
		/// Builds a CSharp project with msbuild.exe. Automatically creates a logfile. When 
		/// no LogName is specified, the executable name is used as logfile base name.
		/// </summary>
		/// <param name="Env">BuildEnvironment to use.</param>
		/// <param name="ProjectFile">Path to the project file. Must be a .csproj file.</param>
		/// <param name="BuildConfig">Configuration to build.</param>
		/// <param name="LogName">Optional logfile name.</param>
		public static void BuildCSharpProject(CommandEnvironment Env, string ProjectFile, string BuildConfig = "Development", string LogName = null)
		{
			if (!ProjectFile.EndsWith(".csproj"))
			{
				throw new AutomationException(String.Format("Unabled to build Project {0}. Not a valid .csproj file.", ProjectFile));
			}
			if (!FileExists(ProjectFile))
			{
				throw new AutomationException(String.Format("Unabled to build Project {0}. Project file not found.", ProjectFile));
			}

			string CmdLine = String.Format(@"/verbosity:normal /target:Rebuild /property:Configuration={0} /property:Platform=AnyCPU", BuildConfig);
			MsBuild(Env, ProjectFile, CmdLine, LogName);
		}

        /// <summary>
        /// Sets an executable bit for a Unix file.
        /// <param name="Filename">Filename</param>
        /// </summary>
        public static void SetExecutableBit(string Filename)
        {
            var Result = CommandUtils.Run("sh", string.Format("-c 'chmod +x \"{0}\"'", Filename));
            if (Result.ExitCode != 0)
            {
                throw new AutomationException(String.Format("Failed to chmod \"{0}\"", Filename));
            }
        }

        /// <summary>
        /// returns true if this is a mac executable using some awful conventions
        /// <param name="Filename">Filename</param>
        /// </summary>
        public static bool IsProbablyAMacOrIOSExe(string Filename)
        {
            return
                (
                    CommandUtils.CombinePaths(Filename).ToLower().Contains(CommandUtils.CombinePaths("Binaries", "Mac").ToLower()) ||
                    CommandUtils.CombinePaths(Filename).ToLower().Contains(CommandUtils.CombinePaths("Binaries", "IOS").ToLower())
                ) && (Path.GetExtension(Filename) == "" || Path.GetExtension(Filename) == ".");
        }
	}

}
