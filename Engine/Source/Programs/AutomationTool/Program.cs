// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// This software is provided "as-is," without any express or implied warranty. 
// In no event shall the author, nor Epic Games, Inc. be held liable for any damages arising from the use of this software.
// This software will not be supported.
// Use at your own risk.
using System;
using System.Threading;
using System.Diagnostics;
using UnrealBuildTool;
using System.Reflection;

namespace AutomationTool
{
	class Program
	{
		// This needs to be static, otherwise SetConsoleCtrlHandler will result in a crash on exit.
		static ProcessManager.CtrlHandlerDelegate ProgramCtrlHandler = new ProcessManager.CtrlHandlerDelegate(CtrlHandler);

		static int Main()
		{
			var CommandLine = SharedUtils.ParseCommandLine();
			HostPlatform.Initialize();
			
			int Result = 1;
			LogUtils.InitLogging(CommandLine);
			Log.WriteLine(TraceEventType.Information, "Running on {0}", HostPlatform.Current.GetType().Name);

			// Log if we're running from the launcher
			var ExecutingAssemblyLocation = CommandUtils.CombinePaths(Assembly.GetExecutingAssembly().Location);
			if (String.Compare(ExecutingAssemblyLocation, CommandUtils.CombinePaths(InternalUtils.ExecutingAssemblyLocation), true) != 0)
			{
				Log.WriteLine(TraceEventType.Information, "Executed from AutomationToolLauncher ({0})", ExecutingAssemblyLocation);
			}
			Log.WriteLine(TraceEventType.Information, "CWD={0}", Environment.CurrentDirectory);

			// Hook up exit callbacks
			var Domain = AppDomain.CurrentDomain;
			Domain.ProcessExit += Domain_ProcessExit;
			Domain.DomainUnload += Domain_ProcessExit;
			HostPlatform.Current.SetConsoleCtrlHandler(ProgramCtrlHandler);

			var Version = InternalUtils.ExecutableVersion;
			Log.WriteLine(TraceEventType.Verbose, "{0} ver. {1}", Version.ProductName, Version.ProductVersion);

			try
			{
				// Don't allow simultaneous execution of AT (in the same branch)
				Result = InternalUtils.RunSingleInstance(MainProc, CommandLine);
			}
			catch (Exception Ex)
			{
				Log.WriteLine(TraceEventType.Error, "AutomationTool terminated with exception:");
				Log.WriteLine(TraceEventType.Error, LogUtils.FormatException(Ex));
				Log.WriteLine(TraceEventType.Error, Ex.Message);
			}

			// Make sure there's no directiories on the stack.
			CommandUtils.ClearDirStack();
			Environment.ExitCode = Result;

			return Result;
		}

		static bool CtrlHandler(CtrlTypes EventType)
		{
			Domain_ProcessExit(null, null);
			if (EventType == CtrlTypes.CTRL_C_EVENT)
			{
				// Force exit
				Environment.Exit(3);
			}
			return true;
		}

		static void Domain_ProcessExit(object sender, EventArgs e)
		{
			// Kill all spawned processes.
			if (ShouldKillProcesses)
			{
				ProcessManager.KillAll();
			}
			// Close logging
			Trace.Close();
		}

		static int MainProc(object Param)
		{
			Automation.Process((string[])Param);
			ShouldKillProcesses = Automation.ShouldKillProcesses;
			return 0;
		}

		static bool ShouldKillProcesses = true;
	}
}
