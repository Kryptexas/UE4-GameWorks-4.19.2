// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
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
	public class Program
	{
		// This needs to be static, otherwise SetConsoleCtrlHandler will result in a crash on exit.
		static ProcessManager.CtrlHandlerDelegate ProgramCtrlHandler = new ProcessManager.CtrlHandlerDelegate(CtrlHandler);

		[STAThread]
		public static int Main()
		{
            var CommandLine = SharedUtils.ParseCommandLine();
            LogUtils.InitLogging(CommandLine);

            ErrorCodes ReturnCode = ErrorCodes.Error_Success;

            try
            {
                HostPlatform.Initialize();

                Log.WriteLine(TraceEventType.Information, "Running on {0}", HostPlatform.Current.GetType().Name);

                XmlConfigLoader.Init();

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

                // Don't allow simultaneous execution of AT (in the same branch)
                InternalUtils.RunSingleInstance(MainProc, CommandLine);

            }
            catch (Exception Ex)
            {
                // Catch all exceptions and propagate the ErrorCode if we are given one.
                Log.WriteLine(TraceEventType.Error, "AutomationTool terminated with exception:");
                Log.WriteLine(TraceEventType.Error, LogUtils.FormatException(Ex));
                // set the exit code of the process
                if (Ex is AutomationException)
                {
                    ReturnCode = (Ex as AutomationException).ErrorCode;
                }
                else
                {
                    ReturnCode = ErrorCodes.Error_Unknown;
                }
            }
            finally
            {
                // In all cases, do necessary shut down stuff, but don't let any additional exceptions leak out while trying to shut down.

                // Make sure there's no directories on the stack.
                NoThrow(() => CommandUtils.ClearDirStack(), "Clear Dir Stack");

                // Try to kill process before app domain exits to leave the other KillAll call to extreme edge cases
                NoThrow(() => { if (ShouldKillProcesses && !Utils.IsRunningOnMono) ProcessManager.KillAll(); }, "Kill All Processes");

                Log.WriteLine(TraceEventType.Information, "AutomationTool exiting with ExitCode={0}", ReturnCode);

                // Can't use NoThrow here because the code logs exceptions. We're shutting down logging!
                LogUtils.ShutdownLogging();
            }

            // STOP: No code beyond the return statement should go beyond this point!
            // Nothing should happen after the finally block above is finished. 
            return (int)ReturnCode;
        }

        /// <summary>
        /// Wraps an action in an exception block.
        /// Ensures individual actions can be performed and exceptions won't prevent further actions from being executed.
        /// Useful for shutdown code where shutdown may be in several stages and it's important that all stages get a chance to run.
        /// </summary>
        /// <param name="Action"></param>
        private static void NoThrow(System.Action Action, string ActionDesc)
        {
            try
            {
                Action();
            }
            catch (Exception Ex)
            {
                Log.WriteLine(TraceEventType.Error, "Exception performing nothrow action \"{0}\": {1}", ActionDesc, LogUtils.FormatException(Ex));
            }
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
			// Kill all spawned processes (Console instead of Log because logging is closed at this time anyway)
			Console.WriteLine("Domain_ProcessExit");
			if (ShouldKillProcesses && !Utils.IsRunningOnMono)
			{			
				ProcessManager.KillAll();
			}
			Trace.Close();
		}

		static void MainProc(object Param)
		{
			Automation.Process((string[])Param);
			ShouldKillProcesses = Automation.ShouldKillProcesses;
		}

		static bool ShouldKillProcesses = true;
	}
}
