// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	public partial class CommandUtils
	{
		#region Commandlets

		/// <summary>
		/// Runs Cook commandlet.
		/// </summary>
		/// <param name="ProjectName">Project name.</param>
		/// <param name="UE4Exe">The name of the UE4 Editor executable to use.</param>
		/// <param name="Maps">List of maps to cook, can be null in which case -MapIniSection=AllMaps is used.</param>
		/// <param name="Dirs">List of directories to cook, can be null</param>
		/// <param name="TargetPlatform">Target platform.</param>
		/// <param name="Parameters">List of additional parameters.</param>
		public static void CookCommandlet(string ProjectName, string UE4Exe = "UE4Editor-Cmd.exe", string[] Maps = null, string[] Dirs = null, string TargetPlatform = "WindowsNoEditor", string Parameters = "-unattended -buildmachine -forcelogflush -AllowStdOutTraceEventType -Unversioned")
		{
			string MapsToCook = "";
			if (IsNullOrEmpty(Maps))
			{
				MapsToCook = "-MapIniSection=AllMaps";
			}
			else
			{
				MapsToCook = "-Map=" + CombineCommandletParams(Maps);
				MapsToCook.Trim();
			}

			string DirsToCook = "";
			if (!IsNullOrEmpty(Dirs))
			{
				DirsToCook = "-CookDir=" + CombineCommandletParams(Dirs);
				DirsToCook.Trim();		
			}

			RunCommandlet(ProjectName, UE4Exe, "Cook", String.Format("{0} {1} -TargetPlatform={2} {3}", MapsToCook, DirsToCook, TargetPlatform, Parameters));
		}

		/// <summary>
		/// Runs a commandlet using Engine/Binaries/Win64/UE4Editor-Cmd.exe.
		/// </summary>
		/// <param name="ProjectName">Project name.</param>
		/// <param name="UE4Exe">The name of the UE4 Editor executable to use.</param>
		/// <param name="Commandlet">Commandlet name.</param>
		/// <param name="Parameters">Command line parameters (without -run=)</param>
		public static void RunCommandlet(string ProjectName, string UE4Exe, string Commandlet, string Parameters = null)
		{
			Log("Running UE4Editor {0} for project {1}", Commandlet, ProjectName);
			
			string EditorExe = HostPlatform.Current.GetUE4ExePath(UE4Exe);
			PushDir(CombinePaths(CmdEnv.LocalRoot, HostPlatform.Current.RelativeBinariesFolder));

			string LogFile = LogUtils.GetUniqueLogName(CombinePaths(CmdEnv.LogFolder, Commandlet));
			Log("Commandlet log file is {0}", LogFile);
            var RunResult = Run(EditorExe, String.Format("\"{0}\" -run={1} {2} -abslog={3} -stdout -FORCELOGFLUSH -CrashForUAT", ProjectName, Commandlet, 
				String.IsNullOrEmpty(Parameters) ? "" : Parameters, 
				CommandUtils.MakePathSafeToUseWithCommandLine(LogFile)));
			PopDir();

			if (RunResult.ExitCode != 0)
			{
				throw new AutomationException("BUILD FAILED: Failed while running {0} for {1}; see log {2}", Commandlet, ProjectName, LogFile);
			}
		}

		/// <summary>
		/// Combines a list of parameters into a single commandline param separated with '+':
		/// For example: Map1+Map2+Map3
		/// </summary>
		/// <param name="ParamValues">List of parameters (must not be empty)</param>
		/// <returns>Combined param</returns>
		public static string CombineCommandletParams(string[] ParamValues)
		{
			if (!IsNullOrEmpty(ParamValues))
			{
				var CombinedParams = ParamValues[0];
				for (int Index = 1; Index < ParamValues.Length; ++Index)
				{
					CombinedParams += "+" + ParamValues[Index];
				}
				return CombinedParams;
			}
			else
			{
				return String.Empty;
			}
		}

		#endregion
	}
}
