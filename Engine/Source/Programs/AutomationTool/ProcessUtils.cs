// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading;
using System.Diagnostics;
using System.Management;
using System.Runtime.InteropServices;

namespace AutomationTool
{
	public enum CtrlTypes
	{
		CTRL_C_EVENT = 0,
		CTRL_BREAK_EVENT,
		CTRL_CLOSE_EVENT,
		CTRL_LOGOFF_EVENT = 5,
		CTRL_SHUTDOWN_EVENT
	}

	/// <summary>
	/// Tracks all active processes.
	/// </summary>
	public sealed class ProcessManager
	{
		public delegate bool CtrlHandlerDelegate(CtrlTypes EventType);

		// @todo: Add mono support
		[DllImport("Kernel32")]
		public static extern bool SetConsoleCtrlHandler(CtrlHandlerDelegate Handler, bool Add);

		/// <summary>
		/// List of active (running) processes.
		/// </summary>
		private static List<ProcessResult> ActiveProcesses = new List<ProcessResult>();
		/// <summary>
		/// Synchronization object
		/// </summary>
		private static object SyncObject = new object();

		/// <summary>
		/// Creates a new process and adds it to the tracking list.
		/// </summary>
		/// <returns>New Process objects</returns>
		public static ProcessResult CreateProcess(bool bAllowSpew, string LogName, Dictionary<string, string> Env = null)
		{
			var NewProcess = HostPlatform.Current.CreateProcess(LogName);
			if (Env != null)
			{
				foreach (var EnvPair in Env)
				{
					if (NewProcess.StartInfo.EnvironmentVariables.ContainsKey(EnvPair.Key))
					{
						NewProcess.StartInfo.EnvironmentVariables.Remove(EnvPair.Key);
					}
					if (!String.IsNullOrEmpty(EnvPair.Value))
					{
						NewProcess.StartInfo.EnvironmentVariables.Add(EnvPair.Key, EnvPair.Value);
					}
				}
			}
			var Result = new ProcessResult(NewProcess, bAllowSpew, LogName);
			NewProcess.Exited += NewProcess_Exited;
			lock (SyncObject)
			{
				ActiveProcesses.Add(Result);
			}
			return Result;
		}

		/// <summary>
		/// Kills all running processes.
		/// </summary>
		public static void KillAll()
		{
			List<ProcessResult> ProcessesToKill = null;
			lock (SyncObject)
			{
				ProcessesToKill = new List<ProcessResult>(ActiveProcesses);
				ActiveProcesses.Clear();
			}
            if (CommandUtils.IsBuildMachine)
            {
                for (int Cnt = 0; Cnt < 9; Cnt++)
                {
                    bool AllDone = true;
                    foreach (var Proc in ProcessesToKill)
                    {
                        try
                        {
                            if (!Proc.HasExited)
                            {
                                AllDone = false;
                                CommandUtils.Log(TraceEventType.Information, "Waiting for process: {0}", Proc.ProcessObject.ProcessName);
                            }
                        }
                        catch (Exception)
                        {
                            CommandUtils.Log(TraceEventType.Information, "Exception Waiting for process");
                            AllDone = false;
                        }
                    }
                    try
                    {
                        if (ProcessResult.HasAnyDescendants(Process.GetCurrentProcess()))
                        {
                            AllDone = false;
                            CommandUtils.Log(TraceEventType.Information, "Waiting for descendants of main process...");
                        }
                    }
                    catch (Exception)
                    {
                        CommandUtils.Log(TraceEventType.Information, "Exception Waiting for descendants of main process");
                        AllDone = false;
                    }

                    if (AllDone)
                    {
                        break;
                    }
                    Thread.Sleep(10000);
                }
            }
			foreach (var Proc in ProcessesToKill)
			{
				try
				{
					if (!Proc.HasExited)
					{
						CommandUtils.Log(TraceEventType.Information, "Killing process: {0}", Proc.ProcessObject.ProcessName);
						Proc.StopProcess(false);
					}
				}
				catch (Exception Ex)
				{
					CommandUtils.Log(TraceEventType.Warning, "Exception while trying to kill process.");
					CommandUtils.Log(TraceEventType.Warning, Ex);
				}
			}
            try
            {
                if (CommandUtils.IsBuildMachine && ProcessResult.HasAnyDescendants(Process.GetCurrentProcess()))
                {
                    CommandUtils.Log(TraceEventType.Information, "current process still has descendants, trying to kill them...");
                    ProcessResult.KillAllDescendants(Process.GetCurrentProcess());
                }
            }
            catch (Exception)
            {
                CommandUtils.Log(TraceEventType.Information, "Exception killing descendants of main process");
            }

		}

		/// <summary>
		/// Removes a process from the list of tracked processes.
		/// </summary>
		/// <param name="sender"></param>
		/// <param name="e"></param>
		public static void NewProcess_Exited(object sender, EventArgs e)
		{
			var Proc = (Process)sender;
			lock (SyncObject)
			{
				for (int Index = ActiveProcesses.Count - 1; Index >= 0; --Index)
				{
					if (ActiveProcesses[Index].ProcessObject == Proc)
					{
						ActiveProcesses.RemoveAt(Index);
						break;
					}
				}
			}
		}
	}

	#region ProcessResult Helper Class

	/// <summary>
	/// Class containing the result of process execution.
	/// </summary>
	public class ProcessResult
	{
		string Source = "";
		int ProcessExitCode = -1;
		StringBuilder ProcessOutput = new StringBuilder();
		bool AllowSpew = true;
		private Process Proc = null;
		private AutoResetEvent OutputWaitHandle = new AutoResetEvent(false);
		private AutoResetEvent ErrorWaitHandle = new AutoResetEvent(false);
		private object ProcSyncObject;

		public ProcessResult(Process InProc, bool bAllowSpew, string LogName)
		{
			ProcSyncObject = new object();
			Proc = InProc;
			Source = LogName;
			AllowSpew = bAllowSpew;
		}

		private void LogOutput(TraceEventType Verbosity, string Message)
		{
			// Manually send message to trace listeners (skips Class.Method source formatting of Log.WriteLine)
			var EventCache = new TraceEventCache();
			foreach (TraceListener Listener in Trace.Listeners)
			{
				Listener.TraceEvent(EventCache, Source, Verbosity, (int)Verbosity, Message);
			}
		}

		/// <summary>
		/// Process.OutputDataReceived event handler.
		/// </summary>
		/// <param name="sender">Sender</param>
		/// <param name="e">Event args</param>
		public void StdOut(object sender, DataReceivedEventArgs e)
		{
			if (e.Data != null)
			{
				if (AllowSpew)
				{
					LogOutput(TraceEventType.Information, e.Data);
				}

				ProcessOutput.Append(e.Data);
				ProcessOutput.Append(Environment.NewLine);
			}
			else
			{
				OutputWaitHandle.Set();
			}
		}

		/// <summary>
		/// Process.ErrorDataReceived event handler.
		/// </summary>
		/// <param name="sender">Sender</param>
		/// <param name="e">Event args</param>
		public void StdErr(object sender, DataReceivedEventArgs e)
		{
			if (e.Data != null)
			{
				if (AllowSpew)
				{
                    LogOutput(TraceEventType.Information, e.Data);
				}

				ProcessOutput.Append(e.Data);
				ProcessOutput.Append(Environment.NewLine);
			}
			else
			{
				ErrorWaitHandle.Set();
			}
		}

		/// <summary>
		/// Convenience operator for getting the exit code value.
		/// </summary>
		/// <param name="Result"></param>
		/// <returns>Process exit code.</returns>
		public static implicit operator int(ProcessResult Result)
		{
			return Result.ExitCode;
		}

		/// <summary>
		/// Gets or sets the process exit code.
		/// </summary>
		public int ExitCode
		{
			get { return ProcessExitCode; }
			set { ProcessExitCode = value; }
		}

		/// <summary>
		/// Gets all std output the process generated.
		/// </summary>
		public string Output
		{
			get { return ProcessOutput.ToString(); }
		}

		public bool HasExited
		{
			get { return Proc != null ? Proc.HasExited : true; }
		}

		public Process ProcessObject
		{
			get { return Proc; }
		}

		/// <summary>
		/// Object iterface.
		/// </summary>
		/// <returns>String representation of this object.</returns>
		public override string ToString()
		{
			return ExitCode.ToString();
		}

		public void WaitForExit()
		{
			bool bProcTerminated = false;
			bool bStdOutSignalReceived = false;
			bool bStdErrSignalReceived = false;
			// Make sure the process objeect is valid.
			lock (ProcSyncObject)
			{
				bProcTerminated = (Proc == null);
			}
			// Keep checking if we got all output messages until the process terminates.
			if (!bProcTerminated)
			{
				// Check messages
				int MaxWaitUntilMessagesReceived = 5;
				while (MaxWaitUntilMessagesReceived > 0 && !(bStdOutSignalReceived && bStdErrSignalReceived))
				{
					if (!bStdOutSignalReceived)
					{
						bStdOutSignalReceived = OutputWaitHandle.WaitOne(500);
					}
					if (!bStdErrSignalReceived)
					{
						bStdErrSignalReceived = ErrorWaitHandle.WaitOne(500);
					}
					// Check if the process terminated
					lock (ProcSyncObject)
					{
						bProcTerminated = (Proc == null) || Proc.HasExited;
					}
					if (bProcTerminated)
					{
						// Process terminated but make sure we got all messages, don't wait forever though
						MaxWaitUntilMessagesReceived--;
					}
				}

				// Double-check if the process terminated
				lock (ProcSyncObject)
				{
					bProcTerminated = (Proc == null) || Proc.HasExited;
				}
				if (!bProcTerminated)
				{
					// The process did not terminate yet but we've read all output messages, wait until the process terminates
					Proc.WaitForExit();
				}
			}
		}

		/// <summary>
		/// Finds child processes of the current process.
		/// </summary>
		/// <param name="ProcessId"></param>
		/// <param name="PossiblyRelatedId"></param>
		/// <param name="VisitedPids"></param>
		/// <returns></returns>
		private static bool IsOurDescendant(Process ProcessToKill, int PossiblyRelatedId, HashSet<int> VisitedPids)
		{
			// check if we're the parent of it or its parent is our descendant
			try
			{
				VisitedPids.Add(PossiblyRelatedId);
				Process Parent = null;
				using (ManagementObject ManObj = new ManagementObject(string.Format("win32_process.handle='{0}'", PossiblyRelatedId)))
				{
					ManObj.Get();
					int ParentId = Convert.ToInt32(ManObj["ParentProcessId"]);
					if (ParentId == 0 || VisitedPids.Contains(ParentId))
					{
						return false;
					}
					Parent = Process.GetProcessById(ParentId);  // will throw an exception if not spawned by us or not running
				}
				if (Parent != null)
				{
					return Parent.Id == ProcessToKill.Id || IsOurDescendant(ProcessToKill, Parent.Id, VisitedPids);  // could use ParentId, but left it to make the above var used
				}
				else
				{
					return false;
				}
			}
			catch (Exception)
			{
				// This can happen if the pid is no longer valid which is ok.
				return false;
			}
		}

		/// <summary>
		/// Kills all child processes of the specified process.
		/// </summary>
		/// <param name="ProcessId">Process id</param>
		public static void KillAllDescendants(Process ProcessToKill)
		{
			bool bKilledAChild;
			do
			{
				bKilledAChild = false;
				Process[] AllProcs = Process.GetProcesses();
				foreach (Process KillCandidate in AllProcs)
				{
					HashSet<int> VisitedPids = new HashSet<int>();
					if (IsOurDescendant(ProcessToKill, KillCandidate.Id, VisitedPids))
					{
						CommandUtils.Log("Trying to kill descendant pid={0}, name={1}", KillCandidate.Id, KillCandidate.ProcessName);
						try
						{
							KillCandidate.Kill();
							bKilledAChild = true;
						}
						catch (Exception Ex)
						{
							CommandUtils.Log(TraceEventType.Error, "Failed to kill descendant:");
							CommandUtils.Log(TraceEventType.Error, Ex);
						}
						break;  // exit the loop as who knows what else died, so let's get processes anew
					}
				}
			} while (bKilledAChild);
		}

        /// <summary>
        /// returns true if this process has any descendants
        /// </summary>
        /// <param name="ProcessToCheck">Process to check</param>
        public static bool HasAnyDescendants(Process ProcessToCheck)
        {

            Process[] AllProcs = Process.GetProcesses();
            foreach (Process KillCandidate in AllProcs)
            {
                HashSet<int> VisitedPids = new HashSet<int>();
                if (IsOurDescendant(ProcessToCheck, KillCandidate.Id, VisitedPids))
                {
                    CommandUtils.Log("Descendant pid={0}, name={1}", KillCandidate.Id, KillCandidate.ProcessName);
                    return true;
                }
            }
            return false;
        }

		public void StopProcess(bool KillDescendants = true)
		{
			if (Proc != null)
			{
				Process ProcToKill = null;
				// At this point any call to Proc memebers will result in an exception
				// so null the object.
				lock (ProcSyncObject)
				{
					ProcToKill = Proc;
					Proc = null;
				}
				// Now actually kill the process and all its descendants if requested
				if (KillDescendants)
				{
					KillAllDescendants(ProcToKill);
				}
				try
				{
					ProcToKill.Kill();
					ProcToKill.Close();
				}
				catch (Exception Ex)
				{
					CommandUtils.Log(TraceEventType.Warning, "Exception while trying to kill process.");
					CommandUtils.Log(TraceEventType.Warning, Ex);
				}

			}
		}
	}

	#endregion

	public partial class CommandUtils
	{
		#region Statistics

		private static Dictionary<string, int> ExeToTimeInMs = new Dictionary<string, int>();

		public static void AddRunTime(string Exe, int TimeInMs)
		{
			string Base = Path.GetFileName(Exe);
			if (!ExeToTimeInMs.ContainsKey(Base))
			{
				ExeToTimeInMs.Add(Base, TimeInMs);
			}
			else
			{
				ExeToTimeInMs[Base] += TimeInMs;
			}
		}

		public static void PrintRunTime()
		{
			foreach (var Item in ExeToTimeInMs)
			{
				Log("Run: Total {0}s to run " + Item.Key, Item.Value / 1000);
			}
			ExeToTimeInMs.Clear();
		}

		#endregion

		[Flags]
		public enum ERunOptions
		{
			None = 0,
			AllowSpew = 1 << 0,
			AppMustExist = 1 << 1,
			NoWaitForExit = 1 << 2,
			NoStdOutRedirect = 1 << 3,
            NoLoggingOfRunCommand = 1 << 4,

			Default = AllowSpew | AppMustExist,
		}

		/// <summary>
		/// Runs external program.
		/// </summary>
		/// <param name="App">Program filename.</param>
		/// <param name="CommandLine">Commandline</param>
		/// <param name="Input">Optional Input for the program (will be provided as stdin)</param>
		/// <param name="Options">Defines the options how to run. See ERunOptions.</param>
		/// <param name="Env">Environment to pass to program.</param>
		/// <returns>Object containing the exit code of the program as well as it's stdout output.</returns>
		public static ProcessResult Run(string App, string CommandLine = null, string Input = null, ERunOptions Options = ERunOptions.Default, Dictionary<string, string> Env = null)
		{
			App = ConvertSeparators(PathSeparator.Default, App);
			HostPlatform.Current.SetupOptionsForRun(ref App, ref Options, ref CommandLine);
            if (App == "ectool" || App == "zip")
			{
				Options &= ~ERunOptions.AppMustExist;
			}

			if (Options.HasFlag(ERunOptions.AppMustExist) && !FileExists(Options.HasFlag(ERunOptions.NoLoggingOfRunCommand) ? true : false, App))
			{
				throw new AutomationException("BUILD FAILED: Couldn't find the executable to Run: {0}", App);
			}
			var StartTime = DateTime.UtcNow;

            if (!Options.HasFlag(ERunOptions.NoLoggingOfRunCommand))
            {
                Log("Run: " + App + " " + (String.IsNullOrEmpty(CommandLine) ? "" : CommandLine));

            }
			ProcessResult Result = ProcessManager.CreateProcess(Options.HasFlag(ERunOptions.AllowSpew), Path.GetFileNameWithoutExtension(App), Env);
			Process Proc = Result.ProcessObject;

			bool bRedirectStdOut = (Options & ERunOptions.NoStdOutRedirect) != ERunOptions.NoStdOutRedirect;
			Proc.EnableRaisingEvents = false;
			Proc.StartInfo.FileName = App;
			Proc.StartInfo.Arguments = String.IsNullOrEmpty(CommandLine) ? "" : CommandLine;
			Proc.StartInfo.UseShellExecute = false;
			if (bRedirectStdOut)
			{
				Proc.StartInfo.RedirectStandardOutput = true;
				Proc.StartInfo.RedirectStandardError = true;
				Proc.OutputDataReceived += Result.StdOut;
				Proc.ErrorDataReceived += Result.StdErr;
			}
			Proc.StartInfo.RedirectStandardInput = Input != null;
			Proc.StartInfo.CreateNoWindow = true;
			Proc.Start();

			if (bRedirectStdOut)
			{
				Proc.BeginOutputReadLine();
				Proc.BeginErrorReadLine();
			}

			if (String.IsNullOrEmpty(Input) == false)
			{
				Proc.StandardInput.WriteLine(Input);
				Proc.StandardInput.Close();
			}

			if (!Options.HasFlag(ERunOptions.NoWaitForExit))
			{
				Result.WaitForExit();
				var BuildDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds;
				AddRunTime(App, (int)(BuildDuration));
                if (!Options.HasFlag(ERunOptions.NoLoggingOfRunCommand))
                {
                    Log("Run: Took {0}s to run " + Path.GetFileName(App), BuildDuration / 1000);

                }
				Result.ExitCode = Proc.ExitCode;
				if (UnrealBuildTool.Utils.IsRunningOnMono)
				{
					// Mono's detection of process exit status is broken. It doesn't clean up after the process and it doesn't call Exited callback.
					Proc.Dispose();
					ProcessManager.NewProcess_Exited(Proc, null);
				}
			}
			else
			{
				Result.ExitCode = -1;
			}

			return Result;
		}

		/// <summary>
		/// Runs external program and writes the output to a logfile.
		/// </summary>
		/// <param name="Env">Environment to use.</param>
		/// <param name="App">Executable to run</param>
		/// <param name="CommandLine">Commandline to pass on to the executable</param>
		/// <param name="LogName">Name of the logfile ( if null, executable name is used )</param>
		/// <returns>Whether the program executed successfully or not.</returns>
		public static void RunAndLog(CommandEnvironment Env, string App, string CommandLine, string LogName = null, int MaxSuccessCode = 0)
		{
			if (LogName == null)
			{
				LogName = Path.GetFileNameWithoutExtension(App);
			}
			string LogFile = LogUtils.GetUniqueLogName(CombinePaths(Env.LogFolder, LogName));

			RunAndLog(App, CommandLine, LogFile, MaxSuccessCode);
		}
		/// <summary>
		/// Runs UAT recursively
		/// </summary>
		/// <param name="Env">Environment to use.</param>
		/// <param name="CommandLine">Commandline to pass on to the executable</param>
		public static string RunUAT(CommandEnvironment Env, string CommandLine)
		{
			// this doesn't do much, but it does need to make sure log folders are reasonable and don't collide
			string BaseLogSubdir = "Recur";
			if (!String.IsNullOrEmpty(CommandLine))
			{
				int Space = CommandLine.IndexOf(" ");
				if (Space > 0)
				{
					BaseLogSubdir = BaseLogSubdir + "_" + CommandLine.Substring(0, Space);
				}
				else
				{
					BaseLogSubdir = BaseLogSubdir + "_" + CommandLine;
				}
			}
			int Index = 0;

			BaseLogSubdir = BaseLogSubdir.Trim();
			string DirOnlyName = BaseLogSubdir;

			string LogSubdir = CombinePaths(CmdEnv.LogFolder, DirOnlyName, "");
            while (true)
			{
                var ExistingFiles = FindFiles(DirOnlyName + "*", false, CmdEnv.LogFolder);
                if (ExistingFiles.Length == 0)
                {
                    break;
                }
				Index++;
				if (Index == 200)
				{
					throw new AutomationException("Couldn't seem to create a log subdir {0}", LogSubdir);
				}
				DirOnlyName = String.Format("{0}_{1}_", BaseLogSubdir, Index);
				LogSubdir = CombinePaths(CmdEnv.LogFolder, DirOnlyName, "");
			}
			string LogFile = CombinePaths(CmdEnv.LogFolder, DirOnlyName + ".log");
			Log("Recursive UAT Run, in log folder {0}, main log file {1}", LogSubdir, LogFile);
			CreateDirectory(LogSubdir);

			CommandLine = CommandLine + " -NoCompile";

			string App = CmdEnv.UATExe;

			Log("Running {0} {1}", App, CommandLine);
			var OSEnv = new Dictionary<string, string>();

			OSEnv.Add(AutomationTool.EnvVarNames.LogFolder, LogSubdir);
			OSEnv.Add("uebp_UATMutexNoWait", "1");
			if (!IsBuildMachine)
			{
				OSEnv.Add(AutomationTool.EnvVarNames.LocalRoot, ""); // if we don't clear this out, it will think it is a build machine; it will rederive everything
			}

			ProcessResult Result = Run(App, CommandLine, null, ERunOptions.Default, OSEnv);
			if (Result.Output.Length > 0)
			{
				WriteToFile(LogFile, Result.Output);
			}
			else
			{
				WriteToFile(LogFile, "[None!, no output produced]");
			}

			Log("Flattening log folder {0}", LogSubdir);

			var Files = FindFiles("*", true, LogSubdir);
			string MyLogFolder = CombinePaths(CmdEnv.LogFolder, "");
			foreach (var ThisFile in Files)
			{
				if (!ThisFile.StartsWith(MyLogFolder, StringComparison.InvariantCultureIgnoreCase))
				{
					throw new AutomationException("Can't rebase {0} because it doesn't start with {1}", ThisFile, MyLogFolder);
				}
				string NewFilename = ThisFile.Substring(MyLogFolder.Length).Replace("/", "_").Replace("\\", "_");
				NewFilename = CombinePaths(CmdEnv.LogFolder, NewFilename);
				if (FileExists_NoExceptions(NewFilename))
				{
					throw new AutomationException("Destination log file already exists? {0}", NewFilename);
				}
				CopyFile(ThisFile, NewFilename);
				if (!FileExists_NoExceptions(NewFilename))
				{
					throw new AutomationException("Destination log file could not be copied {0}", NewFilename);
				}
				DeleteFile_NoExceptions(ThisFile);
			}
            DeleteDirectory_NoExceptions(LogSubdir);

			if (Result != 0)
			{
				throw new AutomationException(String.Format("Recursive UAT Command failed (Result:{3}): {0} {1}. See logfile for details: '{2}' ",
																				App, CommandLine, Path.GetFileName(LogFile), Result.ExitCode));
			}
			return LogFile;
		}

		/// <summary>
		/// Runs external program and writes the output to a logfile.
		/// </summary>
		/// <param name="App">Executable to run</param>
		/// <param name="CommandLine">Commandline to pass on to the executable</param>
		/// <param name="Logfile">Full path to the logfile, where the application output should be written to.</param>
		/// <returns>Whether the program executed successfully or not.</returns>
		public static void RunAndLog(string App, string CommandLine, string Logfile = null, int MaxSuccessCode = 0)
		{
			Log("Running {0} {1}", App, CommandLine);

			ProcessResult Result = Run(App, CommandLine);
			if (Result.Output.Length > 0 && Logfile != null)
			{
				WriteToFile(Logfile, Result.Output);
			}
			else if (Logfile == null)
			{
				Logfile = "[No logfile specified]";
			}
			else
			{
				Logfile = "[None!, no output produced]";
			}

			if (Result > MaxSuccessCode || Result < 0)
			{
				throw new AutomationException(String.Format("Command failed (Result:{3}): {0} {1}. See logfile for details: '{2}' ",
												App, CommandLine, Path.GetFileName(Logfile), Result.ExitCode));
			}
		}

		protected delegate bool ProcessLog(string LogText);
		/// <summary>
		/// Keeps reading a log file as it's being written to by another process until it exits.
		/// </summary>
		/// <param name="LogFilename">Name of the log file.</param>
		/// <param name="LogProcess">Process that writes to the log file.</param>
		/// <param name="OnLogRead">Callback used to process the recently read log contents.</param>
		protected static void LogFileReaderProcess(string LogFilename, ProcessResult LogProcess, ProcessLog OnLogRead = null)
		{
			while (!FileExists(LogFilename) && !LogProcess.HasExited)
			{
				Log("Waiting for logging process to start...");
				Thread.Sleep(2000);
			}
			Thread.Sleep(1000);

			using (FileStream ProcessLog = File.Open(LogFilename, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
			{
				StreamReader LogReader = new StreamReader(ProcessLog);
				bool bKeepReading = true;

				// Read until the process has exited.
				while (!LogProcess.HasExited && bKeepReading)
				{
					while (!LogReader.EndOfStream && bKeepReading)
					{
						string Output = LogReader.ReadToEnd();
						if (Output != null && OnLogRead != null)
						{
							bKeepReading = OnLogRead(Output);
						}
					}

					while (LogReader.EndOfStream && !LogProcess.HasExited)
					{
						Thread.Sleep(250);
						// Tick the callback so that it can respond to external events
						if (OnLogRead != null)
						{
							bKeepReading = OnLogRead(null);
						}
					}
				}
			}
		}

	}
}
