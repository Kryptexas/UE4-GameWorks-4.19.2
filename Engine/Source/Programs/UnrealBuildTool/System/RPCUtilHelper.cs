// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using RPCUtility;
using System.Net;
using System.Net.NetworkInformation;
using System.Reflection;
using System.Runtime.Remoting;
using System.Threading;
using System.Net.Sockets;
using Ionic.Zip;

namespace UnrealBuildTool
{
	public class RPCUtilHelper
	{
		/** The Mac we are compiling on */
		private static string MacName;

		/** A socket per command thread */
		private static Hashtable CommandThreadSockets = new Hashtable();

		/** Random number generator */
		private static Random RandomStream = new Random();

		static RPCUtilHelper()
		{
			AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler(CurrentDomain_AssemblyResolve);
		}

		/**
		 * A callback function to find RPCUtility.exe
		 */
		static Assembly CurrentDomain_AssemblyResolve(Object sender, ResolveEventArgs args)
		{
			// Name is fully qualified assembly definition - e.g. "p4dn, Version=1.0.0.0, Culture=neutral, PublicKeyToken=ff968dc1933aba6f"
			string[] AssemblyInfo = args.Name.Split(",".ToCharArray());
			string AssemblyName = AssemblyInfo[0];

			if (AssemblyName.ToLowerInvariant() == "rpcutility")
			{
				AssemblyName = Path.GetFullPath(@"..\Binaries\DotNET\RPCUtility.exe");
				Debug.WriteLineIf(System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName);

				if (File.Exists(AssemblyName))
				{
					Assembly A = Assembly.LoadFile(AssemblyName);
					return A;
				}
			}
			else if (AssemblyName.ToLowerInvariant() == "ionic.zip.reduced")
			{
				AssemblyName = Path.GetFullPath(@"..\Binaries\DotNET\" + AssemblyName + ".dll");

				Debug.WriteLineIf(System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName);

				if (File.Exists(AssemblyName))
				{
					Assembly A = Assembly.LoadFile(AssemblyName);
					return A;
				}
			}

			return (null);
		}
	
		static public void Initialize(string InMacName)
		{
			MacName = InMacName;

			if (CommandHelper.PingRemoteHost(MacName))
			{
				if (BuildConfiguration.bFlushBuildDirOnRemoteMac)
				{
					Command("/", "rm", "-rf /UE4/Builds/" + Environment.MachineName, null);
				}
			}
			else
			{
				throw new BuildException("Failed to ping Mac named " + MacName);
			}
		}

		/**
		 * Handle a thread ending
		 */
		public static void OnThreadComplete()
		{
			lock (CommandThreadSockets)
			{
				// close and remove the socket
				Socket ThreadSocket = CommandThreadSockets[Thread.CurrentThread] as Socket;
				if (ThreadSocket != null)
				{
					ThreadSocket.Close();
				}
				CommandThreadSockets.Remove(Thread.CurrentThread);
			}
		}

		private static Socket GetSocket()
		{
			Socket ThreadSocket = null;

			lock (CommandThreadSockets)
			{
				ThreadSocket = CommandThreadSockets[Thread.CurrentThread] as Socket;
				if (ThreadSocket == null)
				{
					try
					{
						ThreadSocket = RPCUtility.CommandHelper.ConnectToUnrealRemoteTool(MacName);
					}
					catch (Exception Ex)
					{
						Log.TraceInformation("Failed to connect to UnrealRemoteTool running on {0}.", MacName);
						throw new BuildException(Ex, "Failed to connect to UnrealRemoteTool running on {0}.", MacName);
					}
					CommandThreadSockets[Thread.CurrentThread] = ThreadSocket;
				}
			}

			return ThreadSocket;
		}

		/**
		 * This function should be used as the ActionHandler delegate method for Actions that
		 * need to run over RPCUtility. It will block until the remote command completes
		 */
		static public void RPCActionHandler(Action Action, out int ExitCode, out string Output)
		{
			Hashtable Results = RPCUtilHelper.Command(Action.WorkingDirectory, Action.CommandPath, Action.CommandArguments, 
				Action.ProducedItems.Count > 0 ? Action.ProducedItems[0].AbsolutePath : null);
			if (Results == null)
			{
				ExitCode = -1;
				Output = null;
				Log.TraceInformation("Command failed to execute! {0} {1}", Action.CommandPath, Action.CommandArguments);
			}
			else
			{
				// capture the exit code
				if (Results["ExitCode"] != null)
				{
					ExitCode = (int)(Int64)Results["ExitCode"];
				}
				else
				{
					ExitCode = 0;
				}

				// pass back the string
				Output = Results["CommandOutput"] as string;
			}
		}

		/** 
		 * @return the modification time on the remote machine, accounting for rough difference in time between the two machines
		 */
		public static bool GetRemoteFileInfo(string RemotePath, out DateTime ModificationTime, out long Length)
		{
			return RPCUtility.CommandHelper.GetFileInfo(GetSocket(), RemotePath, DateTime.UtcNow, out ModificationTime, out Length);
		}

		public static void MakeDirectory(string Directory)
		{
			RPCUtility.CommandHelper.MakeDirectory(GetSocket(), Directory);
		}

		[Flags]
		public enum ECopyOptions
		{
			None = 0,
			IsUpload = 1 << 0,
			DoNotReplace = 1 << 1, // if used, will merge a directory
			DoNotUnpack = 1 << 2
		}

		public static void CopyFile(string Source, string Dest, bool bIsUpload)
		{
			if (bIsUpload)
			{
				Hashtable CommandResult = RPCUtility.CommandHelper.RPCUpload(GetSocket(), Source, Dest);
//				Log.TraceInformation(CommandResult["CommandOutput"] as string);
			}
			else
			{
				RPCUtility.CommandHelper.RPCDownload(GetSocket(), Source, Dest);
			}
		}

		// @todo: use temp, random names for zip files
		public static void CopyDirectory(string Source, string Dest, ECopyOptions Options)
		{
			string SourceDirName = Path.GetFileName(Source);
			string DestDirName = Path.GetFileName(Dest);

			if (Options.HasFlag(ECopyOptions.IsUpload))
			{
				if (!Directory.Exists(Source))
				{
					return;
				}

				// Zip source directory
				string SourceZipPath = Path.Combine(Path.GetFullPath(Path.GetDirectoryName(Source)), SourceDirName + ".zip");
				File.Delete(SourceZipPath);
				ZipFile Zip = new ZipFile(SourceZipPath);
				Zip.CompressionLevel = Ionic.Zlib.CompressionLevel.Level9;
				Zip.BufferSize = 0x10000;
				Zip.AddDirectory(Source, DestDirName);
				Zip.Save();

				// Upload the zip file
				string DestWorkingDir = Path.GetDirectoryName(Dest).Replace("\\", "/");
				string DestZipName = DestDirName + ".zip";
				CopyFile(SourceZipPath, DestWorkingDir + "/" + DestZipName, true);

				if (!Options.HasFlag(ECopyOptions.DoNotReplace))
				{
					Command(DestWorkingDir, "rm -rf \"" + DestDirName + "\"", "", null);
				}

				if (!Options.HasFlag(ECopyOptions.DoNotUnpack))
				{
					// Unpack, if requested
					Command(DestWorkingDir, "unzip \"" + DestZipName + "\"", "", null);
					Command(DestWorkingDir, "rm \"" + DestZipName + "\"", "", null);
				}

				File.Delete(SourceZipPath);
			}
			else
			{
				// Zip source directory
				string SourceWorkingDir = Path.GetDirectoryName(Source).Replace("\\", "/");
				string ZipCommand = "zip -0 -r -y -T " + SourceDirName + ".zip " + SourceDirName;
				Command(SourceWorkingDir, ZipCommand, "", null);

				// Download the zip file
				string SourceZipPath = Path.Combine(Path.GetDirectoryName(Source), SourceDirName + ".zip").Replace("\\", "/");
				string DestZipPath = Path.Combine(Path.GetFullPath(Path.GetDirectoryName(Dest)), DestDirName + ".zip");
				CopyFile(SourceZipPath, DestZipPath, false);

				if (!Options.HasFlag(ECopyOptions.DoNotReplace) && Directory.Exists(Dest))
				{
					Directory.GetFiles(Dest, "*", SearchOption.AllDirectories).ToList().ForEach(Entry => { File.SetAttributes(Entry, FileAttributes.Normal); });
					Directory.Delete(Dest, true);
				}

				if (!Options.HasFlag(ECopyOptions.DoNotUnpack))
				{
					// Unpack, if requested
					using (ZipFile Zip = ZipFile.Read(DestZipPath))
					{
						Zip.ToList().ForEach(Entry =>
						{
							Entry.FileName = DestDirName + Entry.FileName.Substring(SourceDirName.Length);
							Entry.Extract(Path.GetDirectoryName(Dest), ExtractExistingFileAction.OverwriteSilently);
						});
					}

					File.Delete(DestZipPath);
				}

				Command(SourceWorkingDir, "rm \"" + SourceDirName + ".zip\"", "", null);
			}
		}

		public static void BatchUpload(string[] Commands)
		{
			// batch upload
			RPCUtility.CommandHelper.RPCBatchUpload(GetSocket(), Commands);
		}

		public static void BatchFileInfo(FileItem[] Files)
		{
			// build a list of file paths to get info about
			StringBuilder FileList = new StringBuilder();
			foreach (FileItem File in Files)
			{
				FileList.AppendFormat("{0}\n", File.AbsolutePath);
			}

			// execute the command!
			Int64[] FileSizeAndDates = RPCUtility.CommandHelper.RPCBatchFileInfo(GetSocket(), FileList.ToString());

			// now update the source times
			for (int Index = 0; Index < Files.Length; Index++)
			{
				Files[Index].Length = FileSizeAndDates[Index * 2 + 0];
				Files[Index].LastWriteTime = new DateTimeOffset(RPCUtility.CommandHelper.FromRemoteTime(FileSizeAndDates[Index * 2 + 1]));
				Files[Index].bExists = FileSizeAndDates[Index * 2 + 0] >= 0;
			}
		}

		public static int GetCommandSlots()
		{
			return RPCUtility.CommandHelper.GetCommandSlots(GetSocket());
		}

		public static Hashtable Command(string WorkingDirectory, string Command, string CommandArgs, string RemoteOutputPath)
		{
	
			int RetriesRemaining = 6;
			do
			{
				// a $ on the commandline will actually be converted, so we need to quote it
				CommandArgs = CommandArgs.Replace("$", "\\$");

				try
				{
					Hashtable Results = RPCUtility.CommandHelper.RPCCommand(GetSocket(), WorkingDirectory, Command, CommandArgs, RemoteOutputPath);
					return Results;
				}
				catch (Exception Ex)
				{
					if (RetriesRemaining > 0)
					{
						Int32 RetryTimeoutMS = 1000;
						Debug.WriteLine("Retrying command after sleeping for " + RetryTimeoutMS + " milliseconds. Command is:" + Command + " " + CommandArgs);
						Thread.Sleep(RetryTimeoutMS);
					}
					else
					{
						Log.TraceInformation("Out of retries, too many exceptions:" + Ex.ToString());
						// We've tried enough times, just throw the error
						throw new Exception("Deep Exception, retries exhausted... ", Ex);
					}
					RetriesRemaining--;
				}
			}
			while (RetriesRemaining > 0);

			return null;
		}
	}
}
