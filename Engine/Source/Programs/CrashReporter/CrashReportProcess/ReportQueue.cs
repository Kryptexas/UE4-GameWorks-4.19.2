// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;

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
	public class ReportQueue
	{
		/// <summary>
		/// Constructor taking the landing zone
		/// </summary>
		public ReportQueue(string LandingZonePath)
		{
			LandingZone = LandingZonePath;
		}

		/// <summary>
		/// Look for new report folders and add them to the publicly available thread-safe queue
		/// </summary>
		public void CheckForNewReports()
		{	
			try
			{
				var NewReportPath = "";
				var ReportsInLandingZone = new ReportIdSet();

				// Check the landing zones for new reports
				DirectoryInfo DirInfo = new DirectoryInfo(LandingZone);

				// Couldn't find a landing zone, skip and try again later.
				// Crash receiver will recreate them if needed.
				if( !DirInfo.Exists )
				{
					CrashReporterProcessServicer.WriteFailure( "LandingZone not found: " + LandingZone );
					return;
				}

				// Add any new reports
				foreach (var SubDirInfo in DirInfo.GetDirectories())
				{
					try
					{
						if( Directory.Exists( SubDirInfo.FullName ) )
						{
							NewReportPath = SubDirInfo.FullName;
							ReportsInLandingZone.Add( NewReportPath );
							if( !ReportsInLandingZoneLastTimeWeChecked.Contains( NewReportPath ) )
							{
								EnqueueNewReport( NewReportPath );
							}
						}
					}
					catch( System.Exception Ex )
					{
						CrashReporterProcessServicer.WriteException( "CheckForNewReportsInner: " + Ex.ToString() );
						ReportProcessor.MoveReportToInvalid( NewReportPath, "NEWRECORD_FAIL_" + DateTime.Now.Ticks );
					}
				}
				//CrashReporterProcessServicer.WriteEvent( string.Format( "ReportsInLandingZone={0}, ReportsInLandingZoneLastTimeWeChecked={1}", ReportsInLandingZone.Count, ReportsInLandingZoneLastTimeWeChecked.Count ) );
				ReportsInLandingZoneLastTimeWeChecked = ReportsInLandingZone;
			}
			catch( Exception Ex )
			{
				CrashReporterProcessServicer.WriteException( "CheckForNewReportsOuter: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Tidy up the landing zone folder
		/// </summary>
		public void CleanLandingZone()
		{
			var DirInfo = new DirectoryInfo(LandingZone);
			foreach(var SubDirInfo in DirInfo.GetDirectories())
			{
				if (SubDirInfo.LastWriteTimeUtc.AddDays(Properties.Settings.Default.DaysToSunsetReport) < DateTime.UtcNow)
				{
					SubDirInfo.Delete(true);
				}
			}
		}

		/// <summary> Looks for the CrashContext.runtime-xml, if found, will return a new instance of the FGenericCrashContext. </summary>
		private FGenericCrashContext FindCrashContext( string NewReportPath )
		{
			string CrashContextPath = Path.Combine( NewReportPath, FGenericCrashContext.CrashContextRuntimeXMLName );
			bool bExist = File.Exists( CrashContextPath );

			if( bExist )
			{
				return FGenericCrashContext.FromFile( CrashContextPath );
			}

			return null;
		}

		/// <summary> Converts WER metadata xml file to the crash context. </summary>
		private FGenericCrashContext ConvertMetadataToCrashContext( WERReportMetadata Metadata, string NewReportPath )
		{
			FGenericCrashContext CrashContext = new FGenericCrashContext();

			FReportData ReportData = new FReportData( Metadata, NewReportPath );

			CrashContext.PrimaryCrashProperties.CrashVersion = (int)ECrashDescVersions.VER_1_NewCrashFormat;

			CrashContext.PrimaryCrashProperties.ProcessId = 0;
			CrashContext.PrimaryCrashProperties.CrashGUID = new DirectoryInfo( NewReportPath ).Name;  
// 			CrashContext.PrimaryCrashProperties.IsInternalBuild
// 			CrashContext.PrimaryCrashProperties.IsPerforceBuild
// 			CrashContext.PrimaryCrashProperties.IsSourceDistribution
// 			CrashContext.PrimaryCrashProperties.SecondsSinceStart
			CrashContext.PrimaryCrashProperties.GameName = ReportData.GameName;
// 			CrashContext.PrimaryCrashProperties.ExecutableName
// 			CrashContext.PrimaryCrashProperties.BuildConfiguration
// 			CrashContext.PrimaryCrashProperties.PlatformName
// 			CrashContext.PrimaryCrashProperties.PlatformNameIni
			CrashContext.PrimaryCrashProperties.PlatformFullName = ReportData.Platform;
			CrashContext.PrimaryCrashProperties.EngineMode = ReportData.EngineMode;
			CrashContext.PrimaryCrashProperties.EngineVersion = ReportData.GetEngineVersion();
			CrashContext.PrimaryCrashProperties.CommandLine = ReportData.CommandLine;
// 			CrashContext.PrimaryCrashProperties.LanguageLCID

			// Create a locate and get the language.
			int LanguageCode = 0;
			int.TryParse( ReportData.Language, out LanguageCode );
			try
			{
				if (LanguageCode > 0)
				{
					CultureInfo LanguageCI = new CultureInfo( LanguageCode );
					CrashContext.PrimaryCrashProperties.AppDefaultLocale = LanguageCI.Name;
				}
			}
			catch (System.Exception)
			{
				// Default to en-US
				CrashContext.PrimaryCrashProperties.AppDefaultLocale = "en-US";
			}


// 			CrashContext.PrimaryCrashProperties.IsUE4Release
			CrashContext.PrimaryCrashProperties.UserName = ReportData.UserName;
			CrashContext.PrimaryCrashProperties.BaseDir = ReportData.BaseDir;
// 			CrashContext.PrimaryCrashProperties.RootDir
			CrashContext.PrimaryCrashProperties.MachineId = ReportData.MachineId;
			CrashContext.PrimaryCrashProperties.EpicAccountId = ReportData.EpicAccountId;
// 			CrashContext.PrimaryCrashProperties.CallStack
// 			CrashContext.PrimaryCrashProperties.SourceContext
			CrashContext.PrimaryCrashProperties.UserDescription = string.Join( "\n", ReportData.UserDescription );
			CrashContext.PrimaryCrashProperties.ErrorMessage = string.Join( "\n", ReportData.ErrorMessage );
// 			CrashContext.PrimaryCrashProperties.CrashDumpMode
// 			CrashContext.PrimaryCrashProperties.Misc.NumberOfCores
// 			CrashContext.PrimaryCrashProperties.Misc.NumberOfCoresIncludingHyperthreads
// 			CrashContext.PrimaryCrashProperties.Misc.Is64bitOperatingSystem
// 			CrashContext.PrimaryCrashProperties.Misc.CPUVendor
// 			CrashContext.PrimaryCrashProperties.Misc.CPUBrand
// 			CrashContext.PrimaryCrashProperties.Misc.PrimaryGPUBrand
// 			CrashContext.PrimaryCrashProperties.Misc.OSVersionMajor
// 			CrashContext.PrimaryCrashProperties.Misc.OSVersionMinor
// 			CrashContext.PrimaryCrashProperties.Misc.AppDiskTotalNumberOfBytes
// 			CrashContext.PrimaryCrashProperties.Misc.AppDiskNumberOfFreeBytes
// 			CrashContext.PrimaryCrashProperties.MemoryStats.TotalPhysical
// 			CrashContext.PrimaryCrashProperties.MemoryStats.TotalVirtual
// 			CrashContext.PrimaryCrashProperties.MemoryStats.PageSize
// 			CrashContext.PrimaryCrashProperties.MemoryStats.TotalPhysicalGB
// 			CrashContext.PrimaryCrashProperties.MemoryStats.AvailablePhysical
// 			CrashContext.PrimaryCrashProperties.MemoryStats.AvailableVirtual
// 			CrashContext.PrimaryCrashProperties.MemoryStats.UsedPhysical
// 			CrashContext.PrimaryCrashProperties.MemoryStats.PeakUsedPhysical
// 			CrashContext.PrimaryCrashProperties.MemoryStats.UsedVirtual
// 			CrashContext.PrimaryCrashProperties.MemoryStats.PeakUsedVirtual
// 			CrashContext.PrimaryCrashProperties.MemoryStats.bIsOOM
// 			CrashContext.PrimaryCrashProperties.MemoryStats.OOMAllocationSize
// 			CrashContext.PrimaryCrashProperties.MemoryStats.OOMAllocationAlignment
			CrashContext.PrimaryCrashProperties.TimeofCrash = new DateTime( ReportData.Ticks );
			CrashContext.PrimaryCrashProperties.bAllowToBeContacted = ReportData.AllowToBeContacted;

			return CrashContext;
		}

		/// <summary> Looks for the WER metadata xml file, if found, will return a new instance of the WERReportMetadata. </summary>
		private WERReportMetadata FindMetadata( string NewReportPath )
		{
			WERReportMetadata MetaData = null;

			// Just to verify that the report is still there.
			DirectoryInfo DirInfo = new DirectoryInfo( NewReportPath );
			if( !DirInfo.Exists )
			{
				CrashReporterProcessServicer.WriteFailure( "FindMetadata: Directory not found " + NewReportPath );
			}
			else
			{
				// Check to see if we wish to suppress processing of this report.	
				foreach( var Info in DirInfo.GetFiles( "*.xml" ) )
				{
					var MetaDataToCheck = XmlHandler.ReadXml<WERReportMetadata>( Info.FullName );
					if( CheckMetaData( MetaDataToCheck ) )
					{
						MetaData = MetaDataToCheck;
						break;
					}
				}
			}
			
			return MetaData;
		}

		/// <summary> Looks for the Diagnostics.txt file and returns true if exists. </summary>
		private bool FindDiagnostics( string NewReportPath )
		{
			bool bFound = false;

			DirectoryInfo DirInfo = new DirectoryInfo( NewReportPath );
			foreach( var Info in DirInfo.GetFiles( CrashReporterConstants.DiagnosticsFileName ) )
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
		private bool CheckMetaData( WERReportMetadata WERData )
		{
			if( WERData == null )
			{
				return false;
			}

			// Reject any crashes with the invalid metadata.
			if( WERData.ProblemSignatures == null || WERData.DynamicSignatures == null || WERData.OSVersionInformation == null )
			{
				return false;
			}

			// Reject any crashes from the minidump processor.
			if( WERData.ProblemSignatures.Parameter0 != null && WERData.ProblemSignatures.Parameter0.ToLower() == "MinidumpDiagnostics".ToLower() )
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
	
		[DllImport( "CompressionHelper.dll", CallingConvention = CallingConvention.Cdecl )]
		private extern static bool UE4UncompressMemoryZLIB( /*void**/IntPtr UncompressedBuffer, Int32 UncompressedSize, /*const void**/IntPtr CompressedBuffer, Int32 CompressedSize );

		/// <summary> 
		/// Decompresses a compressed crash report. 
		/// </summary>
		unsafe private bool DecompressReport( string CompressedReportPath, FCompressedCrashInformation Meta )
		{
			string ReportPath = Path.GetDirectoryName( CompressedReportPath );
			using (FileStream FileStream = File.OpenRead( CompressedReportPath ))
			{
				Int32 UncompressedSize = Meta.GetUncompressedSize();
				Int32 CompressedSize = Meta.GetCompressedSize();

				if (FileStream.Length != CompressedSize)
				{
					return false;
				}

				byte[] UncompressedBufferArray = new byte[UncompressedSize];
				byte[] CompressedBufferArray = new byte[CompressedSize];

				FileStream.Read( CompressedBufferArray, 0, CompressedSize );

				fixed (byte* UncompressedBufferPtr = UncompressedBufferArray, CompressedBufferPtr = CompressedBufferArray)
				{
					bool bResult = UE4UncompressMemoryZLIB( (IntPtr)UncompressedBufferPtr, UncompressedSize, (IntPtr)CompressedBufferPtr, CompressedSize );
					if (!bResult)
					{
						CrashReporterProcessServicer.WriteFailure( "! ZLibFail: Path=" + ReportPath );
						return false;
					}
				}

				MemoryStream MemoryStream = new MemoryStream( UncompressedBufferArray, false );
				BinaryReader BinaryData = new BinaryReader( MemoryStream );

				for (int FileIndex = 0; FileIndex < Meta.GetNumberOfFiles(); FileIndex++)
				{
					Int32 CurrentFileIndex = BinaryData.ReadInt32();
					if (CurrentFileIndex != FileIndex)
					{
						CrashReporterProcessServicer.WriteFailure( "! ReadFail: Required=" + FileIndex + " Read=" + CurrentFileIndex );
						return false;
					}

					string Filename = FBinaryReaderHelper.ReadFixedSizeString( BinaryData );
					Int32 FiledataLength = BinaryData.ReadInt32();
					byte[] Filedata = BinaryData.ReadBytes( FiledataLength );

					File.WriteAllBytes( Path.Combine( ReportPath, Filename ), Filedata );
				}
			}

			return true;
		}

		/// <summary> Enqueues a new crash. </summary>
		void EnqueueNewReport( string NewReportPath )
		{
			string ReportName = Path.GetFileName( NewReportPath );

			string CompressedReportPath = Path.Combine( NewReportPath, ReportName + ".ue4crash" );
			string MetadataPath = Path.Combine( NewReportPath, ReportName + ".xml" );
			bool bIsCompressed = File.Exists( CompressedReportPath ) && File.Exists( MetadataPath );
			if (bIsCompressed)
			{
				FCompressedCrashInformation CompressedCrashInformation = XmlHandler.ReadXml<FCompressedCrashInformation>( MetadataPath );
				bool bResult = DecompressReport( CompressedReportPath, CompressedCrashInformation );
				if (!bResult)
				{
					ReportProcessor.CleanReport( new DirectoryInfo( NewReportPath ) );
					ReportProcessor.MoveReportToInvalid( NewReportPath, "DECOMPRESSION_FAIL_" + DateTime.Now.Ticks + "_" + ReportName );
					return;
				}
				else
				{
					// Rename metadata file to not interfere with the WERReportMetadata.
					File.Move( MetadataPath, Path.ChangeExtension( MetadataPath, "processed_xml" ) );
				}
			}

			// Unified crash reporting
			FGenericCrashContext GenericContext = FindCrashContext( NewReportPath );
			FGenericCrashContext Context = GenericContext;

			bool bFromWER = false;
			if (Context == null || !Context.HasProcessedData())
			{
				WERReportMetadata MetaData = FindMetadata( NewReportPath );
				if (MetaData != null)
				{
					FReportData ReportData = new FReportData( MetaData, NewReportPath );
					Context = ConvertMetadataToCrashContext( MetaData, NewReportPath );
					bFromWER = true;
				}
			}

			if (Context == null)
			{
				CrashReporterProcessServicer.WriteFailure( "! NoCntx  : Path=" + NewReportPath );
				ReportProcessor.CleanReport( new DirectoryInfo( NewReportPath ) );
			}
			else
			{
				if (GenericContext != null && GenericContext.PrimaryCrashProperties.ErrorMessage.Length > 0)
				{
					// Get error message from the crash context and fix value in the metadata.
					Context.PrimaryCrashProperties.ErrorMessage = GenericContext.PrimaryCrashProperties.ErrorMessage;
				}

				Context.CrashDirectory = NewReportPath;
				Context.PrimaryCrashProperties.SetPlatformFullName();

				// If based on WER, save to the file.
				if (bFromWER && GenericContext == null)
				{
					Context.ToFile();
				}
				
				FEngineVersion EngineVersion = new FEngineVersion( Context.PrimaryCrashProperties.EngineVersion ); 

				uint BuiltFromCL = EngineVersion.Changelist;

				string BranchName = EngineVersion.Branch;
				if (string.IsNullOrEmpty( BranchName ))
				{
					CrashReporterProcessServicer.WriteFailure( "% NoBranch: BuiltFromCL=" + string.Format( "{0,7}", BuiltFromCL ) + " Path=" + NewReportPath );
					ReportProcessor.MoveReportToInvalid( NewReportPath, Context.GetAsFilename() );
					return;
				}

				if (BranchName.Equals( CrashReporterConstants.LicenseeBranchName, StringComparison.InvariantCultureIgnoreCase ))
				{
					CrashReporterProcessServicer.WriteFailure( "% UE4-QA  : BuiltFromCL=" + string.Format( "{0,7}", BuiltFromCL ) + " Path=" + NewReportPath );
					ReportProcessor.CleanReport( NewReportPath );
					return;
				}

				// Look for the Diagnostics.txt, if we have a diagnostics file we can continue event if the CL is marked as broken.
				bool bHasDiagnostics = FindDiagnostics( NewReportPath );

				if (BuiltFromCL == 0 && ( !bHasDiagnostics && !Context.HasProcessedData() ))
				{
					// For now ignore all locally made crashes.
					CrashReporterProcessServicer.WriteFailure( "! BROKEN0 : BuiltFromCL=" + string.Format( "{0,7}", BuiltFromCL ) + " Path=" + NewReportPath );
					ReportProcessor.CleanReport( NewReportPath );
					return;
				}


				lock (NewReportsLock)
				{
					NewCrashContexts.Add( Context );
				}
				CrashReporterProcessServicer.WriteEvent( "+ Enqueued: BuiltFromCL=" + string.Format( "{0,7}", BuiltFromCL ) + " Path=" + NewReportPath );
			}
			
		}

		/// <summary> Tries to dequeue a report from the list. </summary>
		public bool TryDequeueReport( out FGenericCrashContext Context )
		{
			lock( NewReportsLock )
			{
				int LastIndex = NewCrashContexts.Count - 1;

				if( LastIndex >= 0 )
				{
					Context = NewCrashContexts[LastIndex];
					NewCrashContexts.RemoveAt( LastIndex );
					CrashReporterProcessServicer.WriteEvent( "- Dequeued: BuiltFromCL=" + string.Format( "{0,7}", Context.PrimaryCrashProperties.EngineVersion ) + " Path=" + Context.CrashDirectory );
					return true;
				}
				else
				{
					Context = null;
					return false;
				}
			}
		}


		/// <summary> A list of freshly landed crash reports. </summary>
		private List<FGenericCrashContext> NewCrashContexts = new List<FGenericCrashContext>();

		/// <summary> Object used to synchronize the access to the NewReport. </summary>
		private Object NewReportsLock = new Object();

		/// <summary>Incoming report path</summary>
		string LandingZone;

		/// <summary>Report IDs that were in the folder on the last pass</summary>
		ReportIdSet ReportsInLandingZoneLastTimeWeChecked = new ReportIdSet();
	}
}
