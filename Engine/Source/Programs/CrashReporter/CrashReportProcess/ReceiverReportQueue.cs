using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using Tools.CrashReporter.CrashReportCommon;
using Tools.DotNETCommon.XmlHandler;

namespace Tools.CrashReporter.CrashReportProcess
{
	using ReportIdSet = HashSet<string>;
	using System.Runtime.InteropServices;
	using System.Globalization;

	/// <summary>
	/// A queue of pending reports in a specific folder
	/// </summary>
	class ReceiverReportQueue : ReportQueueBase
	{
		/// <summary>
		/// Limit over which the we don't attempt to refresh the queue from disk until we've processed and shrunk the queue.
		/// Should save cycles when busy.
		/// </summary>
		private const int SkipQueueRefreshMin = 50;

		/// <summary>
		/// Limit over which the we don't add to the queue
		/// </summary>
		private const int QueueMax = 250;

		/// <summary>
		/// Constructor taking the landing zone
		/// </summary>
		public ReceiverReportQueue(string InQueueName, string LandingZonePath)
		{
			QueueName = InQueueName;
			LandingZone = LandingZonePath;
		}

		/// <summary>
		/// Look for new report folders and add them to the publicly available thread-safe queue.
		/// </summary>
		public override int CheckForNewReports()
		{
			try
			{
				if (QueueCount >= SkipQueueRefreshMin)
				{
					CrashReporterProcessServicer.WriteEvent(string.Format("CheckForNewReports: skipped busy queue size {0} in {1}", QueueCount, LandingZone));
					return LastQueueSizeOnDisk;
				}

				var NewReportPath = "";
				var ReportsInLandingZone = new ReportIdSet();

				// Check the landing zones for new reports
				DirectoryInfo DirInfo = new DirectoryInfo(LandingZone);

				// Couldn't find a landing zone, skip and try again later.
				// Crash receiver will recreate them if needed.
				if (!DirInfo.Exists)
				{
					CrashReporterProcessServicer.WriteFailure("LandingZone not found: " + LandingZone);
					return LastQueueSizeOnDisk;
				}

				var DirectoriesInLandingZone = DirInfo.GetDirectories().OrderBy(dirinfo => dirinfo.CreationTimeUtc).ToArray();
				LastQueueSizeOnDisk = DirectoriesInLandingZone.Length;

				int EnqueuedCount = 0;
				CrashReporterProcessServicer.WriteEvent(string.Format("CheckForNewReports: {0} reports in {1}", DirectoriesInLandingZone.Length, LandingZone));

				// Add any new reports
				for (int DirIndex = 0; DirIndex < DirectoriesInLandingZone.Length && QueueCount < QueueMax; DirIndex++)
				{
					var SubDirInfo = DirectoriesInLandingZone[DirIndex];
					try
					{
						if (Directory.Exists(SubDirInfo.FullName))
						{
							NewReportPath = SubDirInfo.FullName;
							ReportsInLandingZone.Add(NewReportPath);
							if (!ReportsInLandingZoneLastTimeWeChecked.Contains(NewReportPath))
							{
								EnqueueNewReport(NewReportPath);
								EnqueuedCount++;
							}
						}
					}
					catch (System.Exception Ex)
					{
						CrashReporterProcessServicer.WriteException("CheckForNewReportsInner: " + Ex.ToString());
						ReportProcessor.MoveReportToInvalid(NewReportPath, "NEWRECORD_FAIL_" + DateTime.Now.Ticks);
					}
				}
				//CrashReporterProcessServicer.WriteEvent( string.Format( "ReportsInLandingZone={0}, ReportsInLandingZoneLastTimeWeChecked={1}", ReportsInLandingZone.Count, ReportsInLandingZoneLastTimeWeChecked.Count ) );
				ReportsInLandingZoneLastTimeWeChecked = ReportsInLandingZone;
				CrashReporterProcessServicer.WriteEvent(string.Format("CheckForNewReports: {0} enqueued to queue size {1} from {2}", EnqueuedCount, QueueCount, LandingZone));
				CrashReporterProcessServicer.WriteEvent(string.Format("CheckForNewReports: Enqueue rate {0:N1}/minute from {1}", EnqueueCounter.EventsPerSecond * 60, LandingZone));
			}
			catch (Exception Ex)
			{
				CrashReporterProcessServicer.WriteException("CheckForNewReportsOuter: " + Ex.ToString());
			}

			return LastQueueSizeOnDisk;
		}

		/// <summary>
		/// Tidy up the landing zone folder
		/// </summary>
		public override void CleanLandingZone()
		{
			var DirInfo = new DirectoryInfo(LandingZone);
			foreach (var SubDirInfo in DirInfo.GetDirectories())
			{
				if (SubDirInfo.LastWriteTimeUtc.AddDays(Config.Default.DeleteWaitingReportsDays) < DateTime.UtcNow)
				{
					SubDirInfo.Delete(true);
				}
			}
		}

		/// <summary> Tries to dequeue a report from the list. </summary>
		public override bool TryDequeueReport(out FGenericCrashContext Context)
		{
			lock (NewReportsLock)
			{
				if (NewCrashContexts.Count > 0)
				{
					Context = NewCrashContexts.Dequeue();
					DequeueCounter.AddEvent();
					CrashReporterProcessServicer.StatusReporter.IncrementCount(StatusReportingConstants.ProcessingStartedEvent);
					CrashReporterProcessServicer.WriteEvent(string.Format("- Dequeued: {0:N1}/minute BuiltFromCL={1,7} Path={2}", DequeueCounter.EventsPerSecond * 60, Context.PrimaryCrashProperties.EngineVersion, Context.CrashDirectory));
					return true;
				}
			}

			Context = null;
			return false;
		}

		public override string QueueId
		{
			get
			{
				return QueueName;
			}
		}

		public override string LandingZonePath
		{
			get
			{
				return LandingZone;
			}
		}

		/// <summary> Looks for the CrashContext.runtime-xml, if found, will return a new instance of the FGenericCrashContext. </summary>
		private FGenericCrashContext FindCrashContext(string NewReportPath)
		{
			string CrashContextPath = Path.Combine(NewReportPath, FGenericCrashContext.CrashContextRuntimeXMLName);
			bool bExist = File.Exists(CrashContextPath);

			if (bExist)
			{
				return FGenericCrashContext.FromFile(CrashContextPath);
			}

			return null;
		}

		/// <summary> Converts WER metadata xml file to the crash context. </summary>
		private void ConvertMetadataToCrashContext(WERReportMetadata Metadata, string NewReportPath, ref FGenericCrashContext OutCrashContext)
		{
			if (OutCrashContext == null)
			{
				OutCrashContext = new FGenericCrashContext();
			}

			FReportData ReportData = new FReportData(Metadata, NewReportPath);

			OutCrashContext.PrimaryCrashProperties.CrashVersion = (int)ECrashDescVersions.VER_1_NewCrashFormat;

			//OutCrashContext.PrimaryCrashProperties.ProcessId = 0; don't overwrite valid ids, zero is default anyway
			OutCrashContext.PrimaryCrashProperties.CrashGUID = new DirectoryInfo(NewReportPath).Name;
			// 			OutCrashContext.PrimaryCrashProperties.IsInternalBuild
			// 			OutCrashContext.PrimaryCrashProperties.IsPerforceBuild
			// 			OutCrashContext.PrimaryCrashProperties.IsSourceDistribution
			// 			OutCrashContext.PrimaryCrashProperties.SecondsSinceStart
			OutCrashContext.PrimaryCrashProperties.GameName = ReportData.GameName;
			// 			OutCrashContext.PrimaryCrashProperties.ExecutableName
			// 			OutCrashContext.PrimaryCrashProperties.BuildConfiguration
			// 			OutCrashContext.PrimaryCrashProperties.PlatformName
			// 			OutCrashContext.PrimaryCrashProperties.PlatformNameIni
			OutCrashContext.PrimaryCrashProperties.PlatformFullName = ReportData.Platform;
			OutCrashContext.PrimaryCrashProperties.EngineMode = ReportData.EngineMode;
			OutCrashContext.PrimaryCrashProperties.EngineVersion = ReportData.GetEngineVersion();
			OutCrashContext.PrimaryCrashProperties.CommandLine = ReportData.CommandLine;
			// 			OutCrashContext.PrimaryCrashProperties.LanguageLCID

			// Create a locate and get the language.
			int LanguageCode = 0;
			int.TryParse(ReportData.Language, out LanguageCode);
			try
			{
				if (LanguageCode > 0)
				{
					CultureInfo LanguageCI = new CultureInfo(LanguageCode);
					OutCrashContext.PrimaryCrashProperties.AppDefaultLocale = LanguageCI.Name;
				}
			}
			catch (Exception)
			{
				OutCrashContext.PrimaryCrashProperties.AppDefaultLocale = null;
			}

			if (string.IsNullOrEmpty(OutCrashContext.PrimaryCrashProperties.AppDefaultLocale))
			{
				// Default to en-US
				OutCrashContext.PrimaryCrashProperties.AppDefaultLocale = "en-US";
			}

			// 			OutCrashContext.PrimaryCrashProperties.IsUE4Release
			string WERUserName = ReportData.UserName;
			if (!string.IsNullOrEmpty(WERUserName) || string.IsNullOrEmpty(OutCrashContext.PrimaryCrashProperties.UserName))
			{
				OutCrashContext.PrimaryCrashProperties.UserName = WERUserName;
			}

			OutCrashContext.PrimaryCrashProperties.BaseDir = ReportData.BaseDir;
			// 			OutCrashContext.PrimaryCrashProperties.RootDir
			OutCrashContext.PrimaryCrashProperties.MachineId = ReportData.MachineId;
			OutCrashContext.PrimaryCrashProperties.EpicAccountId = ReportData.EpicAccountId;
			// 			OutCrashContext.PrimaryCrashProperties.CallStack
			// 			OutCrashContext.PrimaryCrashProperties.SourceContext
			OutCrashContext.PrimaryCrashProperties.UserDescription = string.Join("\n", ReportData.UserDescription);

			if (string.IsNullOrEmpty(OutCrashContext.PrimaryCrashProperties.ErrorMessage))
			{
				OutCrashContext.PrimaryCrashProperties.ErrorMessage = string.Join("\n", ReportData.ErrorMessage);
			}

			// 			OutCrashContext.PrimaryCrashProperties.CrashDumpMode
			// 			OutCrashContext.PrimaryCrashProperties.Misc.NumberOfCores
			// 			OutCrashContext.PrimaryCrashProperties.Misc.NumberOfCoresIncludingHyperthreads
			// 			OutCrashContext.PrimaryCrashProperties.Misc.Is64bitOperatingSystem
			// 			OutCrashContext.PrimaryCrashProperties.Misc.CPUVendor
			// 			OutCrashContext.PrimaryCrashProperties.Misc.CPUBrand
			// 			OutCrashContext.PrimaryCrashProperties.Misc.PrimaryGPUBrand
			// 			OutCrashContext.PrimaryCrashProperties.Misc.OSVersionMajor
			// 			OutCrashContext.PrimaryCrashProperties.Misc.OSVersionMinor
			// 			OutCrashContext.PrimaryCrashProperties.Misc.AppDiskTotalNumberOfBytes
			// 			OutCrashContext.PrimaryCrashProperties.Misc.AppDiskNumberOfFreeBytes
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.TotalPhysical
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.TotalVirtual
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.PageSize
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.TotalPhysicalGB
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.AvailablePhysical
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.AvailableVirtual
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.UsedPhysical
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.PeakUsedPhysical
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.UsedVirtual
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.PeakUsedVirtual
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.bIsOOM
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.OOMAllocationSize
			// 			OutCrashContext.PrimaryCrashProperties.MemoryStats.OOMAllocationAlignment
			OutCrashContext.PrimaryCrashProperties.TimeofCrash = new DateTime(ReportData.Ticks);
			OutCrashContext.PrimaryCrashProperties.bAllowToBeContacted = ReportData.AllowToBeContacted;
		}

		/// <summary> Looks for the WER metadata xml file, if found, will return a new instance of the WERReportMetadata. </summary>
		private WERReportMetadata FindMetadata(string NewReportPath)
		{
			WERReportMetadata MetaData = null;

			// Just to verify that the report is still there.
			DirectoryInfo DirInfo = new DirectoryInfo(NewReportPath);
			if (!DirInfo.Exists)
			{
				CrashReporterProcessServicer.WriteFailure("FindMetadata: Directory not found " + NewReportPath);
			}
			else
			{
				// Check to see if we wish to suppress processing of this report.	
				foreach (var Info in DirInfo.GetFiles("*.xml"))
				{
					var MetaDataToCheck = XmlHandler.ReadXml<WERReportMetadata>(Info.FullName);
					if (CheckMetaData(MetaDataToCheck))
					{
						MetaData = MetaDataToCheck;
						break;
					}
				}
			}

			return MetaData;
		}

		/// <summary> Looks for the Diagnostics.txt file and returns true if exists. </summary>
		private bool FindDiagnostics(string NewReportPath)
		{
			bool bFound = false;

			DirectoryInfo DirInfo = new DirectoryInfo(NewReportPath);
			foreach (var Info in DirInfo.GetFiles(CrashReporterConstants.DiagnosticsFileName))
			{
				bFound = true;
				break;
			}
			return bFound;
		}

		/// <summary>
		/// Optionally don't process some reports based on the Windows Error report meta data.
		/// </summary>
		/// <param name="WERData">The Windows Error Report meta data.</param>
		/// <returns>false to reject the report.</returns>
		private bool CheckMetaData(WERReportMetadata WERData)
		{
			if (WERData == null)
			{
				return false;
			}

			// Reject any crashes with the invalid metadata.
			if (WERData.ProblemSignatures == null || WERData.DynamicSignatures == null || WERData.OSVersionInformation == null)
			{
				return false;
			}

			// Reject any crashes from the minidump processor.
			if (WERData.ProblemSignatures.Parameter0 != null && WERData.ProblemSignatures.Parameter0.ToLower() == "MinidumpDiagnostics".ToLower())
			{
				return false;
			}

			return true;
		}

		// From CrashUpload.cpp
		/*
		struct FCompressedCrashFile : FNoncopyable
		{
			int32 CurrentFileIndex; // 4 bytes for file index
			FString Filename; // 4 bytes for length + 260 bytes for char data
			TArray<uint8> Filedata; // 4 bytes for length + N bytes for data
		}
		*/

		/// <summary> 
		/// Decompresses a compressed crash report. 
		/// </summary>
		unsafe private bool DecompressReport(string CompressedReportPath, FCompressedCrashInformation Meta)
		{
			string ReportPath = Path.GetDirectoryName(CompressedReportPath);
			using (FileStream FileStream = File.OpenRead(CompressedReportPath))
			{
				Int32 UncompressedSize = Meta.GetUncompressedSize();
				Int32 CompressedSize = Meta.GetCompressedSize();

				if (FileStream.Length != CompressedSize)
				{
					return false;
				}

				byte[] UncompressedBufferArray = new byte[UncompressedSize];
				byte[] CompressedBufferArray = new byte[CompressedSize];

				FileStream.Read(CompressedBufferArray, 0, CompressedSize);

				fixed (byte* UncompressedBufferPtr = UncompressedBufferArray, CompressedBufferPtr = CompressedBufferArray)
				{
					bool bResult = NativeMethods.UE4UncompressMemoryZLIB((IntPtr)UncompressedBufferPtr, UncompressedSize, (IntPtr)CompressedBufferPtr, CompressedSize);
					if (!bResult)
					{
						CrashReporterProcessServicer.WriteFailure("! ZLibFail: Path=" + ReportPath);
						return false;
					}
				}

				MemoryStream MemoryStream = new MemoryStream(UncompressedBufferArray, false);
				BinaryReader BinaryData = new BinaryReader(MemoryStream);

				for (int FileIndex = 0; FileIndex < Meta.GetNumberOfFiles(); FileIndex++)
				{
					Int32 CurrentFileIndex = BinaryData.ReadInt32();
					if (CurrentFileIndex != FileIndex)
					{
						CrashReporterProcessServicer.WriteFailure("! ReadFail: Required=" + FileIndex + " Read=" + CurrentFileIndex);
						return false;
					}

					string Filename = FBinaryReaderHelper.ReadFixedSizeString(BinaryData);
					Int32 FiledataLength = BinaryData.ReadInt32();
					byte[] Filedata = BinaryData.ReadBytes(FiledataLength);

					File.WriteAllBytes(Path.Combine(ReportPath, Filename), Filedata);
				}
			}

			return true;
		}

		/// <summary> Enqueues a new crash. </summary>
		private void EnqueueNewReport(string NewReportPath)
		{
			string ReportName = Path.GetFileName(NewReportPath);

			string CompressedReportPath = Path.Combine(NewReportPath, ReportName + ".ue4crash");
			string MetadataPath = Path.Combine(NewReportPath, ReportName + ".xml");
			bool bIsCompressed = File.Exists(CompressedReportPath) && File.Exists(MetadataPath);
			if (bIsCompressed)
			{
				FCompressedCrashInformation CompressedCrashInformation = XmlHandler.ReadXml<FCompressedCrashInformation>(MetadataPath);
				bool bResult = DecompressReport(CompressedReportPath, CompressedCrashInformation);
				if (!bResult)
				{
					ReportProcessor.CleanReport(new DirectoryInfo(NewReportPath));
					ReportProcessor.MoveReportToInvalid(NewReportPath, "DECOMPRESSION_FAIL_" + DateTime.Now.Ticks + "_" + ReportName);
					return;
				}
				else
				{
					// Rename metadata file to not interfere with the WERReportMetadata.
					File.Move(MetadataPath, Path.ChangeExtension(MetadataPath, "processed_xml"));
				}
			}

			// Unified crash reporting
			FGenericCrashContext GenericContext = FindCrashContext(NewReportPath);
			FGenericCrashContext Context = GenericContext;

			bool bFromWER = false;
			if (Context == null || !Context.HasProcessedData())
			{
				WERReportMetadata MetaData = FindMetadata(NewReportPath);
				if (MetaData != null)
				{
					FReportData ReportData = new FReportData(MetaData, NewReportPath);
					ConvertMetadataToCrashContext(MetaData, NewReportPath, ref Context);
					bFromWER = true;
				}
			}

			if (Context == null)
			{
				CrashReporterProcessServicer.WriteFailure("! NoCntx  : Path=" + NewReportPath);
				ReportProcessor.CleanReport(new DirectoryInfo(NewReportPath));
			}
			else
			{
				Context.CrashDirectory = NewReportPath;
				Context.PrimaryCrashProperties.SetPlatformFullName();

				// Added data from WER, save to the crash context file.
				if (bFromWER)
				{
					Context.ToFile();
				}

				FEngineVersion EngineVersion = new FEngineVersion(Context.PrimaryCrashProperties.EngineVersion);

				uint BuiltFromCL = EngineVersion.Changelist;

				string BranchName = EngineVersion.Branch;
				if (string.IsNullOrEmpty(BranchName))
				{
					CrashReporterProcessServicer.WriteEvent("% Warning NoBranch: BuiltFromCL=" + string.Format("{0,7}", BuiltFromCL) + " Path=" + NewReportPath + " EngineVersion=" + Context.PrimaryCrashProperties.EngineVersion);
					Context.PrimaryCrashProperties.ProcessorFailedMessage = "Engine version has no branch name. EngineVersion=" + Context.PrimaryCrashProperties.EngineVersion;
				}
				else if (BranchName.Equals(CrashReporterConstants.LicenseeBranchName, StringComparison.InvariantCultureIgnoreCase))
				{
					CrashReporterProcessServicer.WriteEvent("% Warning branch is UE4-QA  : BuiltFromCL=" + string.Format("{0,7}", BuiltFromCL) + " Path=" + NewReportPath);
					Context.PrimaryCrashProperties.ProcessorFailedMessage = "Branch was the forbidden LicenseeBranchName=" + BranchName;
				}

				// Look for the Diagnostics.txt, if we have a diagnostics file we can continue event if the CL is marked as broken.
				bool bHasDiagnostics = FindDiagnostics(NewReportPath);

				if (BuiltFromCL == 0 && (!bHasDiagnostics && !Context.HasProcessedData()))
				{
					// For now ignore all locally made crashes.
					CrashReporterProcessServicer.WriteEvent("% Warning CL=0 and no useful data : BuiltFromCL=" + string.Format("{0,7}", BuiltFromCL) + " Path=" + NewReportPath);
					Context.PrimaryCrashProperties.ProcessorFailedMessage = "Report from CL=0 has no diagnostics, callstack or error msg";
				}


				lock (NewReportsLock)
				{
					NewCrashContexts.Enqueue(Context);
				}
				EnqueueCounter.AddEvent();
				CrashReporterProcessServicer.StatusReporter.IncrementCount(StatusReportingConstants.QueuedEvent);
				CrashReporterProcessServicer.WriteEvent("+ Enqueued: BuiltFromCL=" + string.Format("{0,7}", BuiltFromCL) + " Path=" + NewReportPath);
			}

		}

		private int QueueCount
		{
			get
			{
				lock (NewReportsLock)
				{
					return NewCrashContexts.Count;
				}			
			}
		}

		/// <summary> A list of freshly landed crash reports. </summary>
		private readonly Queue<FGenericCrashContext> NewCrashContexts = new Queue<FGenericCrashContext>();

		/// <summary> Object used to synchronize the access to the NewReport. </summary>
		private readonly Object NewReportsLock = new Object();

		/// <summary>Friendly queue name</summary>
		private readonly string QueueName;

		/// <summary>Incoming report path</summary>
		private readonly string LandingZone;

		/// <summary>Report IDs that were in the folder on the last pass</summary>
		private ReportIdSet ReportsInLandingZoneLastTimeWeChecked = new ReportIdSet();

		private readonly EventCounter EnqueueCounter = new EventCounter(TimeSpan.FromMinutes(90), 20);
		private readonly EventCounter DequeueCounter = new EventCounter(TimeSpan.FromMinutes(10), 10);

		private int LastQueueSizeOnDisk = 0;
	}
}

static class NativeMethods
{
	[DllImport("CompressionHelper.dll", CallingConvention = CallingConvention.Cdecl)]
	public extern static bool UE4UncompressMemoryZLIB( /*void**/IntPtr UncompressedBuffer, Int32 UncompressedSize, /*const void**/IntPtr CompressedBuffer, Int32 CompressedSize);	
}