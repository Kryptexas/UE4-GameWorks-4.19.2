// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Reflection;
using Microsoft.Win32;
using System.Diagnostics;
using UnrealBuildTool;

namespace AutomationTool
{
	/// <summary>
	/// Defines the environment variable names that will be used to setup the environemt.
	/// </summary>
	static class EnvVarNames
	{
		// Command Environment
		static public readonly string LocalRoot = "uebp_LOCAL_ROOT";
		static public readonly string LogFolder = "uebp_LogFolder";
        static public readonly string CSVFile = "uebp_CSVFile";
		static public readonly string EngineSavedFolder = "uebp_EngineSavedFolder";
		static public readonly string NETFrameworkDir = "FrameworkDir";
		static public readonly string NETFrameworkVersion = "FrameworkVersion";

		// Perforce Environment
		static public readonly string P4Port = "uebp_PORT";		
		static public readonly string ClientRoot = "uebp_CLIENT_ROOT";
		static public readonly string Changelist = "uebp_CL";
		static public readonly string User = "uebp_USER";
		static public readonly string Client = "uebp_CLIENT";
		static public readonly string BuildRootP4 = "uebp_BuildRoot_P4";
		static public readonly string BuildRootEscaped = "uebp_BuildRoot_Escaped";		
		static public readonly string LabelToSync = "uebp_LabelToSync";
		static public readonly string P4Password = "uebp_PASS";
	}


	/// <summary>
	/// Environment to allow access to commonly used environment variables.
	/// </summary>
	public class CommandEnvironment
	{
		/// <summary>
		/// Path to a file we know to always exist under the UE4 root directory.
		/// </summary>
		public static readonly string KnownFileRelativeToRoot = @"Engine/Config/BaseEngine.ini";

		#region Command Environment properties

		public string LocalRoot { get; protected set; }
		public string EngineSavedFolder { get; protected set; }
		public string LogFolder { get; protected set; }
        public string CSVFile { get; protected set; }
		public string RobocopyExe { get; protected set; }
		public string MountExe { get; protected set; }
		public string CmdExe { get; protected set; }
		public string UATExe { get; protected set; }		
		public string TimestampAsString { get; protected set; }
		public bool HasCapabilityToCompile { get; protected set; }
		public string MsBuildExe { get; protected set; }
		public string MsDevExe { get; protected set; }

		#endregion

		internal CommandEnvironment()
		{
			InitEnvironment();
		}

		/// <summary>
		/// Sets the location of the exe.
		/// </summary>
		protected void SetUATLocation()
		{
			if (String.IsNullOrEmpty(UATExe))
			{
				UATExe = CommandUtils.CombinePaths(Path.GetFullPath(InternalUtils.ExecutingAssemblyLocation));
			}
			if (!CommandUtils.FileExists_NoExceptions(UATExe))
			{
				throw new AutomationException("Could not find AutomationTool.exe. Reflection indicated it was here: {0}", UATExe);
			}
		}

		/// <summary>
		/// Sets the location of the AutomationTool/Saved directory
		/// </summary>
		protected void SetUATSavedPath()
		{
			var LocalRootPath = CommandUtils.GetEnvVar(EnvVarNames.LocalRoot);
			var SavedPath = CommandUtils.CombinePaths(PathSeparator.Slash, LocalRootPath, "Engine", "Programs", "AutomationTool", "Saved");
			CommandUtils.SetEnvVar(EnvVarNames.EngineSavedFolder, SavedPath);
		}

		/// <summary>
		/// Initializes the environement.
		/// </summary>
		protected virtual void InitEnvironment()
		{
			SetUATLocation();

			LocalRoot = CommandUtils.GetEnvVar(EnvVarNames.LocalRoot);
			if (String.IsNullOrEmpty(CommandUtils.GetEnvVar(EnvVarNames.EngineSavedFolder)))
			{
				SetUATSavedPath();
			}

			if (LocalRoot.EndsWith(":"))
			{
				LocalRoot += Path.DirectorySeparatorChar;
			}

			EngineSavedFolder = CommandUtils.GetEnvVar(EnvVarNames.EngineSavedFolder);
            CSVFile = CommandUtils.GetEnvVar(EnvVarNames.CSVFile);            
			LogFolder = CommandUtils.GetEnvVar(EnvVarNames.LogFolder);
			RobocopyExe = CommandUtils.CombinePaths(Environment.SystemDirectory, "robocopy.exe");
			MountExe = CommandUtils.CombinePaths(Environment.SystemDirectory, "mount.exe");
			CmdExe = Utils.IsRunningOnMono ? "/bin/sh" : CommandUtils.CombinePaths(Environment.SystemDirectory, "cmd.exe");

			if (String.IsNullOrEmpty(LogFolder))
			{
				throw new AutomationException("Environment is not set up correctly: LogFolder is not set!");
			}

			if (String.IsNullOrEmpty(LocalRoot))
			{
				throw new AutomationException("Environment is not set up correctly: LocalRoot is not set!");
			}

			if (String.IsNullOrEmpty(EngineSavedFolder))
			{
				throw new AutomationException("Environment is not set up correctly: EngineSavedFolder is not set!");
			}

			// Make sure that the log folder exists
			var LogFolderInfo = new DirectoryInfo(LogFolder);
			if (!LogFolderInfo.Exists)
			{
				LogFolderInfo.Create();
			}

			// Setup the timestamp string
			DateTime LocalTime = DateTime.Now;

			string TimeStamp = LocalTime.Year + "-"
						+ LocalTime.Month.ToString("00") + "-"
						+ LocalTime.Day.ToString("00") + "_"
						+ LocalTime.Hour.ToString("00") + "."
						+ LocalTime.Minute.ToString("00") + "."
						+ LocalTime.Second.ToString("00");

			TimestampAsString = TimeStamp;

			SetupBuildEnvironment();

			LogSettings();
		}

		void LogSettings()
		{
			Log.TraceVerbose("Command Environment settings:");
			Log.TraceVerbose("CmdExe={0}", CmdExe);
			Log.TraceVerbose("EngineSavedFolder={0}", EngineSavedFolder);
			Log.TraceVerbose("HasCapabilityToCompile={0}", HasCapabilityToCompile);
			Log.TraceVerbose("LocalRoot={0}", LocalRoot);
			Log.TraceVerbose("LogFolder={0}", LogFolder);
			Log.TraceVerbose("MountExe={0}", MountExe);
			Log.TraceVerbose("MsBuildExe={0}", MsBuildExe);
			Log.TraceVerbose("MsDevExe={0}", MsDevExe);
			Log.TraceVerbose("RobocopyExe={0}", RobocopyExe);
			Log.TraceVerbose("TimestampAsString={0}", TimestampAsString);
			Log.TraceVerbose("UATExe={0}", UATExe);			
		}

		#region Compiler Setup

		/// <summary>
		/// Initializes build environemnt: finds the path to msbuild.exe
		/// </summary>
		void SetupBuildEnvironment()
		{
			// Assume we have the capability co compile.
			HasCapabilityToCompile = true;

			try
			{
				HostPlatform.Current.SetFrameworkVars();
			}
			catch (Exception)
			{
				// Something went wrong, we can't compile.
				Log.WriteLine(TraceEventType.Warning, "SetFrameworkVars failed. Assuming no compilation capability.");
				HasCapabilityToCompile = false;
			}

			if (HasCapabilityToCompile)
			{
				try
				{
					MsBuildExe = HostPlatform.Current.GetMsBuildExe();
				}
				catch (Exception Ex)
				{
					Log.WriteLine(TraceEventType.Warning, Ex.Message);
					Log.WriteLine(TraceEventType.Warning, "Assuming no compilation capability.");
					HasCapabilityToCompile = false;
					MsBuildExe = "";
				}
			}

			if (HasCapabilityToCompile)
			{
				try
				{
					MsDevExe = HostPlatform.Current.GetMsDevExe();
				}
				catch (Exception Ex)
				{
					Log.WriteLine(TraceEventType.Warning, Ex.Message);
					Log.WriteLine(TraceEventType.Warning, "Assuming no solution compilation capability.");
					MsDevExe = "";
				}
			}

			Log.TraceVerbose("CompilationEvironment.HasCapabilityToCompile={0}", HasCapabilityToCompile);
			Log.TraceVerbose("CompilationEvironment.MsBuildExe={0}", MsBuildExe);
			Log.TraceVerbose("CompilationEvironment.MsDevExe={0}", MsDevExe);
		}

		#endregion
	}

}
