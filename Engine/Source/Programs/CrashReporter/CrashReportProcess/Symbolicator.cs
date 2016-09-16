using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Tools.CrashReporter.CrashReportCommon;
using Tools.DotNETCommon.LaunchProcess;

namespace Tools.CrashReporter.CrashReportProcess
{
	class Symbolicator
	{
		/// <summary>
		/// Builds MDD command line args, waits for a MDD task slot, runs MDD and blocks for the result. Writes diag text into DiagnosticsPath folder if successful.
		/// </summary>
		/// <param name="DiagnosticsPath">Path of the minidump file</param>
		/// <param name="Context">The crash context</param>
		/// <param name="ProcessorIndex">Processor thread index for logging purposes</param>
		/// <returns>True, if successful</returns>
		public bool Run(string DiagnosticsPath, FGenericCrashContext Context, int ProcessorIndex)
		{
			// Use the latest MinidumpDiagnostics from the main branch.
			CrashReporterProcessServicer.StatusReporter.AlertOnLowDisk(Config.Default.DepotRoot, Config.Default.DiskSpaceAlertPercent);
			string Win64BinariesDirectory = Path.Combine(Config.Default.DepotRoot, Config.Default.MDDBinariesFolderInDepot);
#if DEBUG
			// Note: the debug executable must be built locally or synced from Perforce manually
			string MinidumpDiagnosticsName = Path.Combine(Win64BinariesDirectory, "MinidumpDiagnostics-Win64-Debug.exe");
#else
			string MinidumpDiagnosticsName = Path.Combine(Win64BinariesDirectory, "MinidumpDiagnostics.exe");
#endif
			// Don't purge logs
			// TODO: make this clear to logs once a day or something (without letting MDD check on every run!)
			string PurgeLogsDays = "-1";

			FEngineVersion EngineVersion = new FEngineVersion(Context.PrimaryCrashProperties.EngineVersion);

			// Pass Windows variants (Win32/64) to MinidumpDiagnostics
			string PlatformVariant = Context.PrimaryCrashProperties.PlatformName;
			if (PlatformVariant != null && Context.PrimaryCrashProperties.PlatformFullName != null && PlatformVariant.ToUpper().Contains("WINDOWS"))
			{
				if (Context.PrimaryCrashProperties.PlatformFullName.Contains("Win32") ||
					Context.PrimaryCrashProperties.PlatformFullName.Contains("32b"))
				{
					PlatformVariant = "Win32";
				}
				else if (Context.PrimaryCrashProperties.PlatformFullName.Contains("Win64") ||
						Context.PrimaryCrashProperties.PlatformFullName.Contains("64b"))
				{
					PlatformVariant = "Win64";
				}
			}

			// Build the absolute log file path for MinidumpDiagnostics
			string BaseFolder = CrashReporterProcessServicer.SymbolicatorLogFolder;
			string SubFolder = DateTime.UtcNow.ToString("yyyy_MM_dd");
			string Folder = Path.Combine(BaseFolder, SubFolder);
			string AbsLogPath = Path.Combine(Folder, Context.GetAsFilename() + ".log");
			Directory.CreateDirectory(Folder);

			List<string> MinidumpDiagnosticsParams = new List<string>
			(
				new string[]
				{
					"\""+DiagnosticsPath+"\"",
					"-BranchName=" + EngineVersion.Branch,          // Backward compatibility
					"-BuiltFromCL=" + EngineVersion.Changelist,     // Backward compatibility
					"-GameName=" + Context.PrimaryCrashProperties.GameName,
					"-EngineVersion=" + Context.PrimaryCrashProperties.EngineVersion,
					"-BuildVersion=" + (string.IsNullOrWhiteSpace(Context.PrimaryCrashProperties.BuildVersion) ?
										string.Format("{0}-CL-{1}", EngineVersion.Branch, EngineVersion.Changelist).Replace('/', '+') :
										Context.PrimaryCrashProperties.BuildVersion),
					"-PlatformName=" + Context.PrimaryCrashProperties.PlatformName,
					"-PlatformVariantName=" + PlatformVariant,
					"-bUsePDBCache=true",
					"-PDBCacheDepotRoot=" + Config.Default.DepotRoot,
					"-PDBCachePath=" + Config.Default.MDDPDBCachePath,
					"-PDBCacheSizeGB=" + Config.Default.MDDPDBCacheSizeGB,
					"-PDBCacheMinFreeSpaceGB=" + Config.Default.MDDPDBCacheMinFreeSpaceGB,
					"-PDBCacheFileDeleteDays=" + Config.Default.MDDPDBCacheFileDeleteDays,
					"-MutexPDBCache",
					"-PDBCacheLock=CrashReportProcessPDBCacheLock",
					"-NoTrimCallstack",
					"-SyncSymbols",
					"-NoP4Symbols",
					"-ForceUsePDBCache",
					"-MutexSourceSync",
					"-SourceSyncLock=CrashReportProcessSourceSyncLock",
					"-SyncMicrosoftSymbols",
					"-unattended",
					"-AbsLog=" + AbsLogPath,
					"-DepotIndex=" + Config.Default.DepotIndex,
					"-P4User=" + Config.Default.P4User,
					"-P4Client=" + Config.Default.P4Client,
					"-ini:Engine:[LogFiles]:PurgeLogsDays=" + PurgeLogsDays + ",[LogFiles]:MaxLogFilesOnDisk=-1",
					"-LOGTIMESINCESTART"
				}
			);

			LaunchProcess.CaptureMessageDelegate CaptureMessageDelegate = null;
			if (Environment.UserInteractive)
			{
				CaptureMessageDelegate = CrashReporterProcessServicer.WriteMDD;
			}
			else
			{
				MinidumpDiagnosticsParams.AddRange
				(
					new[]
					{
						"-buildmachine",
						"-forcelogflush"
					}
				);

				// Write some debugging message.
				CrashReporterProcessServicer.WriteMDD(MinidumpDiagnosticsName + " Params: " + String.Join(" ", MinidumpDiagnosticsParams));
			}

			Task<bool> NewSymbolicatorTask = Task.FromResult(false);
			Stopwatch WaitSW = Stopwatch.StartNew();
			Double WaitForLockTime = 0.0;

			lock (Tasks)
			{
				int TaskIdx = Task.WaitAny(Tasks);

				Tasks[TaskIdx] = NewSymbolicatorTask = Task<bool>.Factory.StartNew(() =>
				{
					LaunchProcess ReportParser = new LaunchProcess(MinidumpDiagnosticsName, Path.GetDirectoryName(MinidumpDiagnosticsName), CaptureMessageDelegate,
																	MinidumpDiagnosticsParams.ToArray());

					return ReportParser.WaitForExit(Config.Default.MDDTimeoutMinutes*1000*60) == EWaitResult.Ok;
				});

				WaitForLockTime = WaitSW.Elapsed.TotalSeconds;
			}

			NewSymbolicatorTask.Wait();

			Double TotalMDDTime = WaitSW.Elapsed.TotalSeconds;
			CrashReporterProcessServicer.WriteEvent(string.Format("PROC-{0} ", ProcessorIndex) + string.Format("Symbolicator.Run: Thread blocked for {0:N1}s then MDD ran for {1:N1}s", WaitForLockTime, TotalMDDTime - WaitForLockTime));

			return NewSymbolicatorTask.Result;
		}

		private Task<bool>[] Tasks;

		public Symbolicator()
		{
			Tasks = Enumerable
				.Repeat(Task.FromResult(true), Config.Default.MaxConcurrentMDDs)
				.ToArray();
		}
	}
}
