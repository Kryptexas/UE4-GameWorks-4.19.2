// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Linq;
using System.Security.AccessControl;
using System.Xml;
using System.Text;
using System.Text.RegularExpressions;

namespace UnrealBuildTool
{
    abstract class RemoteToolChain : UEToolChain
    {
		protected void RegisterRemoteToolChain(UnrealTargetPlatform InPlatform, CPPTargetPlatform CPPPlatform)
		{
			RemoteToolChainPlatform = InPlatform;

			// Register this tool chain for IOS
			Log.TraceVerbose("        Registered for {0}", CPPPlatform.ToString());
			UEToolChain.RegisterPlatformToolChain(CPPPlatform, this);
		}
		

        /***********************************************************************
         * NOTE:
         *  Do NOT change the defaults to set your values, instead you should set the environment variables
         *  properly in your system, as other tools make use of them to work properly!
         *  The defaults are there simply for examples so you know what to put in your env vars...
         ***********************************************************************/

        /** The name of the MacOS machine to talk to compile the game */
		public static string RemoteServerName = Utils.GetStringEnvironmentVariable("ue.RemoteCompileServerName", "a1011"); // MacPro4,1, 16 logical cores, 16 GB
		public static string[] PotentialServerNames =
		{
			// always on
			"a1488", // Xserve3,1, 16 logical cores, 16 GB

			// developers
			"a2852", // MacPro5,1, 24 logical cores, 16 GB
			"a2853", // MacPro5,1, 24 logical cores, 16 GB
			"a3066", // MacPro5,1, 24 logical cores, 16 GB
			"a3067", // MacPro5,1, 24 logical cores, 16 GB
			//"a1719", // Unreachable
			//"a3071", // MacPro5,1, 8 logical cores, 16 GB - testing if removing it improves promotable build times

			// a1011 - dedicated to people
			// a1012 - dedicated to Trepka
			// a3072 - dedicated to Jenkins
			//"a3070", // MacPro5,1, 8 logical cores, 16 GB - dedicated to Electric Commander
			//"a3068", // MacPro5,1, 24 logical cores, 24 GB - dedicated to Electric Commander

		};

		/** Keep a list of remote files that are potentially copied from local to remote */
		private static Dictionary<FileItem, FileItem> CachedRemoteFileItems = new Dictionary<FileItem, FileItem>();

		/** The path (on the Mac) to the your particular development directory, where files will be copied to from the PC */
		public static string UserDevRootMac = "/UE4/Builds/";

		/** The directory that this local branch is in, without drive information (strip off X:\ from X:\UE4\iOS) */
		public static string BranchDirectory = Environment.MachineName + "\\" + Path.GetFullPath("..\\..\\").Substring(3);
		public static string BranchDirectoryMac = BranchDirectory.Replace("\\", "/");


        /** Substrings that indicate a line contains an error */
        protected static List<string> ErrorMessageTokens;

		/** The platform this toolchain is compiling for */
		protected UnrealTargetPlatform RemoteToolChainPlatform;

		static RemoteToolChain()
        {
            ErrorMessageTokens = new List<string>();
            ErrorMessageTokens.Add("ERROR ");
            ErrorMessageTokens.Add("** BUILD FAILED **");
            ErrorMessageTokens.Add("[BEROR]");
            ErrorMessageTokens.Add("IPP ERROR");
            ErrorMessageTokens.Add("System.Net.Sockets.SocketException");
        }

        // Do any one-time, global initialization for the tool chain
        public override void SetUpGlobalEnvironment()
        {
            if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
            {
            	// If we don't care which machine we're going to build on, query and
            	// pick the one with the most free command slots available
				if (RemoteServerName == "best_available")
            	{
					int AvailableSlots = 0;
					int Attempts = 0;
					if (!ProjectFileGenerator.bGenerateProjectFiles)
					{
						Log.TraceInformation("Picking a random Mac builder...");
					}
					while (AvailableSlots < 2 && Attempts < 20)
					{
						RemoteServerName = PotentialServerNames.OrderBy(x => Guid.NewGuid()).FirstOrDefault();
						
						// make sure it's ready to take commands
						AvailableSlots = GetAvailableCommandSlotCount(RemoteServerName);

						Attempts++;
					}

					// make sure it succeeded
					if (AvailableSlots <= 1)
					{
						throw new BuildException("Failed to find a Mac available to take commands!");
					}
					else if (!ProjectFileGenerator.bGenerateProjectFiles)
					{
						Log.TraceInformation("Chose {0} after {1} attempts to find a Mac, with {2} slots", RemoteServerName, Attempts, AvailableSlots);
					}
/*
 * this does not work right, because it pushes a lot of tasks to machines that have substantially more slots than others
					Log.TraceInformation("Picking the best available Mac builder...");
                	Int32 MostAvailableCount = Int32.MinValue;
					foreach (string NextMacName in PotentialServerNames)
                	{
                    	Int32 NextAvailableCount = GetAvailableCommandSlotCount(NextMacName);
                    	if (NextAvailableCount > MostAvailableCount)
                    	{
                        	MostAvailableName = NextMacName;
                        	MostAvailableCount = NextAvailableCount;
                    	}

						Log.TraceVerbose("... " + NextMacName + " has " + NextAvailableCount + " slots available");
                	}
                	Log.TraceVerbose("Picking the compile server with the most available command slots: " + MostAvailableName);

                	// Finally, assign the name of the Mac we're going to use
					RemoteServerName = MostAvailableName;
 */
            	}
				else if (!ProjectFileGenerator.bGenerateProjectFiles)
				{
					Log.TraceInformation("Picking the default remote server " + RemoteServerName);
				}

				if (!Utils.IsRunningOnMono)
				{
					// crank up RPC communications
					RPCUtilHelper.Initialize(RemoteServerName);
				}
            }
            else
            {
				RemoteServerName = Environment.MachineName;
            }
        }


        /** Converts the passed in path from UBT host to compiler native format. */
        public override String ConvertPath(String OriginalPath)
        {
            if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
            {
                // Path of directory on Mac
                string MacPath = GetMacDevSrcRoot();

                // In the case of paths from the PC to the Mac over a UNC path, peel off the possible roots
                string StrippedPath = OriginalPath.Replace(BranchDirectory, "");

                // Now, reduce the path down to just relative to UE4 and add file location
                MacPath += "../../" + StrippedPath.Substring(RootDirectoryLocation(StrippedPath));

				try
				{
					MacPath = Path.GetFullPath(MacPath);
				}
				catch (Exception Ex)
				{
					throw new BuildException(Ex, "Error getting full path for: {0} (Exception: {1})", MacPath, Ex.Message);
				}

                // Replace back slashes with forward for the Mac
                MacPath = MacPath.Replace("\\", "/");

				if (MacPath.IndexOf(':') == 1)
				{
					MacPath = MacPath.Substring(2);
				}

				return MacPath;
            }
            else
            {
				return OriginalPath.Replace("\\", "/");
            }
        }

        static int RootDirectoryLocation(string LocalPath)
        {
            // By default, assume the filename is already stripped down and the root is at zero
            int RootDirLocation = 0;

            string UBTRootPath = Path.GetFullPath(AppDomain.CurrentDomain.BaseDirectory + "..\\..\\..\\");
            if (LocalPath.ToUpperInvariant().Contains(UBTRootPath.ToUpperInvariant()))
            {
                // If the file is a full path name and rooted at the same location as UBT,
                // use that location as the root and simply return the length
                RootDirLocation = UBTRootPath.Length;
            }

            return RootDirLocation;
        }

		private static List<string> RsyncDirs = new List<string>();
		private static List<string> RsyncExtensions = new List<string>();

        public void QueueFileForBatchUpload(FileItem LocalFileItem)
        {
			// Now, we actually just remember unique directories with any files, and upload all files in them to the remote machine
			// (either via rsync, or RPCUtil acting like rsync)
			string Entry = Path.GetDirectoryName(LocalFileItem.AbsolutePath);
			if (!RsyncDirs.Contains(Entry))
			{
				RsyncDirs.Add(Entry);
			}
			
			
// 			// If not, create it now
//             string RemoteFilePath = ConvertPath(LocalFileItem.AbsolutePath);
// 
//             // add this file to the list of files we will RPCBatchUpload later
//             string Entry = LocalFileItem.AbsolutePath + " " + RemoteFilePath;
//             if (!BatchUploadCommands.Contains(Entry))
//             {
//                 BatchUploadCommands.Add(Entry);
//             }

// 			// strip it down to the root (rsync wants it to end in a / for -a to work)
// 			Entry = Utils.MakePathRelativeTo(Path.GetDirectoryName(LocalFileItem.AbsolutePath), "../..").Replace('\\', '/') + "/";
// 			if (!RsyncDirs.Contains(Entry))
// 			{
// 				RsyncDirs.Add(Entry);
// 			}

			string Ext = Path.GetExtension(LocalFileItem.AbsolutePath);
			if (!RsyncExtensions.Contains(Ext))
            {
				RsyncExtensions.Add(Ext);
            }
        }

        public FileItem LocalToRemoteFileItem(FileItem LocalFileItem, bool bShouldUpload)
        {
            FileItem RemoteFileItem = null;

            // Look to see if we've already made a remote FileItem for this local FileItem
            if (!CachedRemoteFileItems.TryGetValue(LocalFileItem, out RemoteFileItem))
            {
                // If not, create it now
                string RemoteFilePath = ConvertPath(LocalFileItem.AbsolutePath);
				RemoteFileItem = FileItem.GetRemoteItemByPath(RemoteFilePath, RemoteToolChainPlatform);

                // Is shadowing requested?
                if (bShouldUpload)
                {
                    QueueFileForBatchUpload(LocalFileItem);
                }

                CachedRemoteFileItems.Add(LocalFileItem, RemoteFileItem);
            }

            return RemoteFileItem;
        }

        /**
		 * Helper function to sync source files to and from the local system and a remote Mac
		 */
        //This chunk looks to be required to pipe output to VS giving information on the status of a remote build.
        public static bool OutputReceivedDataEventHandlerEncounteredError = false;
        public static string OutputReceivedDataEventHandlerEncounteredErrorMessage = "";
        public static void OutputReceivedDataEventHandler(Object Sender, DataReceivedEventArgs Line)
        {
            if ((Line != null) && (Line.Data != null))
            {
                Log.TraceInformation(Line.Data);

                foreach (string ErrorToken in ErrorMessageTokens)
                {
                    if (Line.Data.Contains(ErrorToken))
                    {
                        OutputReceivedDataEventHandlerEncounteredError = true;
                        OutputReceivedDataEventHandlerEncounteredErrorMessage += Line.Data;
                        break;
                    }
                }
            }
        }

		public override void PostCodeGeneration(UEBuildTarget Target, UHTManifest Manifest)
		{
			if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
			{
				// @todo UHT: Temporary workaround for UBT no longer being able to follow includes from generated headers unless
				//  the headers already existed before the build started.  We're working on a proper fix.

				// Make sure all generated headers are synced.  If we had to generate code, we need to assume that not all of the
				// header files existed on disk at the time that UBT scanned include statements looking for prerequisite files.  Those
				// files are created during code generation and must exist on disk by the time this function is called.  We'll scan
				// for generated code files and make sure they are enqueued for copying to the remote machine.
				foreach( var UObjectModule in Manifest.Modules )
				{
					// @todo uht: Ideally would only copy exactly the files emitted by UnrealHeaderTool, rather than scanning directory (could copy stale files; not a big deal though)
					var GeneratedCodeDirectory = UEBuildModuleCPP.GetGeneratedCodeDirectoryForModule(Target, UObjectModule.BaseDirectory, UObjectModule.Name);
					var GeneratedCodeFiles     = Directory.GetFiles( GeneratedCodeDirectory, "*", SearchOption.AllDirectories );
					foreach( var GeneratedCodeFile in GeneratedCodeFiles )
					{
						// Skip copying "Timestamp" files (UBT temporary files)
						if( !Path.GetFileName( GeneratedCodeFile ).Equals( @"Timestamp", StringComparison.InvariantCultureIgnoreCase ) )
						{
							var GeneratedCodeFileItem = FileItem.GetExistingItemByPath( GeneratedCodeFile );
							QueueFileForBatchUpload( GeneratedCodeFileItem );
						}
					}

					// For source files in legacy "Classes" directories, we need to make sure they all get copied over too, since
					// they may not have been directly included in any C++ source files (only generated headers), and the initial
					// header scan wouldn't have picked them up if they hadn't been generated yet!
					{
						var SourceFiles = Directory.GetFiles( UObjectModule.BaseDirectory, "*", SearchOption.AllDirectories );
						foreach( var SourceFile in SourceFiles )
						{
							var SourceFileItem = FileItem.GetExistingItemByPath( SourceFile );
							QueueFileForBatchUpload( SourceFileItem );
						}
					}
				}
			}
		}

		static public void OutputReceivedForRsync(Object Sender, DataReceivedEventArgs Line)
		{
			if ((Line != null) && (Line.Data != null) && (Line.Data != ""))
			{
				Log.TraceInformation(Line.Data);
			}
		}
		
		public override void PreBuildSync()
        {
			// no need to sync on the Mac!
			if (ExternalExecution.GetRuntimePlatform() == UnrealTargetPlatform.Mac)
			{
				return;
			}

			bool bUseRPCUtil = true;

			if (bUseRPCUtil)
			{
				List<string> BatchUploadCommands = new List<string>();

				// for each directory we visited, add all the files in that directory
				foreach (string Dir in RsyncDirs)
				{
					// look only for useful extensions
					foreach (string Ext in RsyncExtensions)
					{
						string[] Files = Directory.GetFiles(Dir, "*" + Ext);
						foreach (string SyncFile in Files)
						{
							string RemoteFilePath = ConvertPath(SyncFile);
							// an upload command is local name and remote name
							BatchUploadCommands.Add(SyncFile + " " + RemoteFilePath);
						}
					}
				}

				// batch upload
                RPCUtilHelper.BatchUpload(BatchUploadCommands.ToArray());
			}
			else
			{
				List<string> RelativeRsyncDirs = new List<string>();
				foreach (string Dir in RsyncDirs)
				{
					RelativeRsyncDirs.Add(Utils.CleanDirectorySeparators(Utils.MakePathRelativeTo(Dir, "../.."), '/') + "/");
				}

				// write out directories to copy
				File.WriteAllLines("D:\\dev\\CarefullyRedist\\DeltaCopy\\RSyncPaths.txt", RelativeRsyncDirs.ToArray());
				File.WriteAllLines("D:\\dev\\CarefullyRedist\\DeltaCopy\\IncludeFrom.txt", RsyncExtensions);

				// source and destination paths in the format rsync wants
				string CygPath = "/cygdrive/" + Utils.CleanDirectorySeparators(Path.GetFullPath("../../").Replace(":", ""), '/');
				string RemotePath = ConvertPath(Path.GetFullPath("../../"));

				Process RsyncProcess = new Process();
				RsyncProcess.StartInfo.WorkingDirectory = Path.GetFullPath("D:\\dev\\CarefullyRedist\\DeltaCopy");
				RsyncProcess.StartInfo.FileName = "D:\\dev\\CarefullyRedist\\DeltaCopy\\rsync.exe";
				RsyncProcess.StartInfo.Arguments = string.Format(
					"-vzae \"ssh -i id_rsa\" --delete --files-from=RsyncPaths.txt " +
					"--include-from=IncludeFrom.txt --include='*/' --exclude='*' {0} josh.adams@{2}:{1}",
					CygPath,
					RemotePath,
					RemoteServerName);

				RsyncProcess.OutputDataReceived += new DataReceivedEventHandler(OutputReceivedForRsync);
				RsyncProcess.ErrorDataReceived += new DataReceivedEventHandler(OutputReceivedForRsync);

				// run rsync
				Utils.RunLocalProcess(RsyncProcess);
			}

			// we can now clear out the set of files
            RsyncDirs.Clear();
			RsyncExtensions.Clear();
		}

		public override void PostBuildSync(UEBuildTarget Target)
        {

        }

		static public Double GetAdjustedProcessorCountMultiplier()
		{
            if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
            {
                Int32 RemoteCPUCount = RPCUtilHelper.GetCommandSlots();
                if (RemoteCPUCount == 0)
                {
                    RemoteCPUCount = Environment.ProcessorCount;
                }

                Double AdjustedMultiplier = (Double)RemoteCPUCount / (Double)Environment.ProcessorCount;
                Log.TraceVerbose("Adjusting the remote Mac compile process multiplier to " + AdjustedMultiplier.ToString());
                return AdjustedMultiplier;
            }
            else
            {
                return 1.0;
            }
		}

        static public Int32 GetAvailableCommandSlotCount(string TargetMacName)
        {
            // ask how many slots are available, and increase by 1 (not sure why)
            Int32 RemoteAvailableCommandSlotCount = 1 + QueryRemoteMachine(TargetMacName, "rpc:command_slots_available");

            Log.TraceVerbose("Available command slot count for " + TargetMacName + " is " + RemoteAvailableCommandSlotCount.ToString());
            return RemoteAvailableCommandSlotCount;
        }

		/**
		 * Translates clang output warning/error messages into vs-clickable messages
		 * 
		 * @param	sender		Sending object
		 * @param	e			Event arguments (In this case, the line of string output)
		 */
		protected void RemoteOutputReceivedEventHandler(object sender, DataReceivedEventArgs e)
		{
			var Output = e.Data;
			if (Output == null)
			{
				return;
			}

			if (Utils.IsRunningOnMono)
			{
				Log.TraceInformation(Output);
			}
			else
			{
				// Need to match following for clickable links
				string RegexFilePath = @"^(\/[A-Za-z0-9_\-\.]*)+\.(cpp|c|mm|m|hpp|h)";
				string RegexLineNumber = @"\:\d+\:\d+\:";
				string RegexDescription = @"(\serror:\s|\swarning:\s).*";

				// Get Matches
				string MatchFilePath = Regex.Match(Output, RegexFilePath).Value.Replace("Engine/Source/../../", "");
				string MatchLineNumber = Regex.Match(Output, RegexLineNumber).Value;
				string MatchDescription = Regex.Match(Output, RegexDescription).Value;

				// If any of the above matches failed, do nothing
				if (MatchFilePath.Length == 0 ||
					MatchLineNumber.Length == 0 ||
					MatchDescription.Length == 0)
				{
					Log.TraceInformation(Output);
					return;
				}

				// Convert Path
				string RegexStrippedPath = @"\/Engine\/.*"; //@"(Engine\/|[A-Za-z0-9_\-\.]*\/).*";
				string ConvertedFilePath = Regex.Match(MatchFilePath, RegexStrippedPath).Value;
				ConvertedFilePath = Path.GetFullPath("..\\.." + ConvertedFilePath);

				// Extract Line + Column Number
				string ConvertedLineNumber = Regex.Match(MatchLineNumber, @"\d+").Value;
				string ConvertedColumnNumber = Regex.Match(MatchLineNumber, @"(?<=:\d+:)\d+").Value;

				// Write output
				string ConvertedExpression = "  " + ConvertedFilePath + "(" + ConvertedLineNumber + "," + ConvertedColumnNumber + "):" + MatchDescription;
				Log.TraceInformation(ConvertedExpression); // To create clickable vs link
	//			Log.TraceInformation(Output);				// To preserve readable output log
			}
		}
		
		/**
		 * Queries the remote compile server for CPU information
		 * and computes the proper ProcessorCountMultiplier.
		 */
        static private Int32 QueryResult = 0;
        static public void OutputReceivedForQuery(Object Sender, DataReceivedEventArgs Line)
        {
            if ((Line != null) && (Line.Data != null) && (Line.Data != ""))
            {
                Int32 TestValue = 0;
                if (Int32.TryParse(Line.Data, out TestValue))
                {
                    QueryResult = TestValue;
                }
                else
                {
                  Log.TraceVerbose("Info: Unexpected output from remote Mac system info query, skipping");                  
                }
            }
        }

        static public Int32 QueryRemoteMachine(string MachineName, string Command)
        {
            // we must run the commandline RPCUtility, because we could run this before we have opened up the RemoteRPCUtlity
            Process QueryProcess = new Process();
            QueryProcess.StartInfo.WorkingDirectory = Path.GetFullPath("..\\Binaries\\DotNET");
            QueryProcess.StartInfo.FileName = QueryProcess.StartInfo.WorkingDirectory + "\\RPCUtility.exe";
            QueryProcess.StartInfo.Arguments = string.Format("{0} {1} sysctl -n hw.ncpu",
                MachineName,
                UserDevRootMac);
            QueryProcess.OutputDataReceived += new DataReceivedEventHandler(OutputReceivedForQuery);
            QueryProcess.ErrorDataReceived += new DataReceivedEventHandler(OutputReceivedForQuery);

            // Try to launch the query's process, and produce a friendly error message if it fails.
            Utils.RunLocalProcess(QueryProcess);

            return QueryResult;
        }

        public static string GetMacDevSrcRoot()
        {
            if (ExternalExecution.GetRuntimePlatform() != UnrealTargetPlatform.Mac)
            {
                return UserDevRootMac + BranchDirectoryMac + "Engine/Source/";
            }
            else
            {
                return ".";
            }
        }
    };
}
