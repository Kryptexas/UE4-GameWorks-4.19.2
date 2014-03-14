// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UnrealBuildTool;

namespace AutomationTool
{
	/// <summary>
	/// Environment to allow access to commonly used environment variables.
	/// </summary>
	public class P4Environment
	{
		#region Perforce Environment Properties

		public string P4Port { get; protected set; }
		public string ClientRoot { get; protected set; }
		public string User { get; protected set; }
		public string ChangelistString { get; protected set; }
		public int Changelist { get; protected set; }
		public string Client { get; protected set; }
		public string BuildRootP4 { get; protected set; }
		public string BuildRootEscaped { get; protected set; }
		public string LabelToSync { get; protected set; }
		public string LabelPrefix { get; protected set; }
		public string BranchName { get; protected set; }

		#endregion

		internal P4Environment(CommandEnvironment CmdEnv)
		{
			InitEnvironment(CmdEnv);
		}

		protected virtual void InitEnvironment(CommandEnvironment CmdEnv)
		{
			//
			// P4 Environment
			//

			P4Port = CommandUtils.GetEnvVar(EnvVarNames.P4Port);
			ClientRoot = CommandUtils.GetEnvVar(EnvVarNames.ClientRoot);
			User = CommandUtils.GetEnvVar(EnvVarNames.User);
			ChangelistString = CommandUtils.GetEnvVar(EnvVarNames.Changelist);
			Client = CommandUtils.GetEnvVar(EnvVarNames.Client);
			BuildRootP4 = CommandUtils.GetEnvVar(EnvVarNames.BuildRootP4);
			if (BuildRootP4.EndsWith("/", StringComparison.InvariantCultureIgnoreCase) || BuildRootP4.EndsWith("\\", StringComparison.InvariantCultureIgnoreCase))
			{
				// We expect the build root to not end with a path separator
				BuildRootP4 = BuildRootP4.Substring(0, BuildRootP4.Length - 1);
				CommandUtils.SetEnvVar(EnvVarNames.BuildRootP4, BuildRootP4);
			}
			BuildRootEscaped = CommandUtils.GetEnvVar(EnvVarNames.BuildRootEscaped);
			LabelToSync = CommandUtils.GetEnvVar(EnvVarNames.LabelToSync);

			if (((CommandUtils.P4Enabled || CommandUtils.IsBuildMachine) && (ClientRoot == String.Empty || User == String.Empty ||
				ChangelistString == String.Empty || Client == String.Empty || BuildRootP4 == String.Empty)))
			{
				throw new AutomationException("BUILD FAILED Perforce Environment is not set up correctly. Please check your environment variables.");
			}

			LabelPrefix = BuildRootP4 + "/";

			if (CommandUtils.P4Enabled)
			{
				if (ChangelistString.Length < 2)
				{
					throw new AutomationException("BUILD FAILED bad CL {0}.", ChangelistString);
				}

				Changelist = int.Parse(ChangelistString);

				if (Changelist <= 10)
				{
					throw new AutomationException("BUILD FAILED bad CL {0} {1}.", ChangelistString, Changelist);
				}

				ChangelistString = Changelist.ToString();		// Make sure they match

				// Setup branch name
				string DepotSuffix = "//depot/";
				if (BuildRootP4.StartsWith(DepotSuffix))
				{
					BranchName = BuildRootP4.Substring(DepotSuffix.Length);
				}
				else
				{
					throw new AutomationException("Needs update to work with a stream");
				}

				if (String.IsNullOrWhiteSpace(BranchName))
				{
					throw new AutomationException("BUILD FAILED no branch name.");
				}
			}

			LogSettings();
		}

		void LogSettings()
		{
			// Log all vars
			const bool bQuiet = true;
			Log.TraceInformation("Perforce Environment settings:");
			Log.TraceInformation("{0}={1}", EnvVarNames.P4Port, InternalUtils.GetEnvironmentVariable(EnvVarNames.P4Port, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.User, InternalUtils.GetEnvironmentVariable(EnvVarNames.User, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.Client, InternalUtils.GetEnvironmentVariable(EnvVarNames.Client, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.BuildRootP4, InternalUtils.GetEnvironmentVariable(EnvVarNames.BuildRootP4, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.BuildRootEscaped, InternalUtils.GetEnvironmentVariable(EnvVarNames.BuildRootEscaped, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.ClientRoot, InternalUtils.GetEnvironmentVariable(EnvVarNames.ClientRoot, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.Changelist, InternalUtils.GetEnvironmentVariable(EnvVarNames.Changelist, "", bQuiet));
			Log.TraceInformation("{0}={1}", EnvVarNames.LabelToSync, InternalUtils.GetEnvironmentVariable(EnvVarNames.LabelToSync, "", bQuiet));
			Log.TraceInformation("{0}={1}", "P4USER", InternalUtils.GetEnvironmentVariable("P4USER", "", bQuiet));
			Log.TraceInformation("{0}={1}", "P4CLIENT", InternalUtils.GetEnvironmentVariable("P4CLIENT", "", bQuiet));
			Log.TraceInformation("LabelPrefix={0}", LabelPrefix);
		}
	}
}
