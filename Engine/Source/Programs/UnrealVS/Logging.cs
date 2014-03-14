using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealVS
{
	static class Logging
	{
		private static readonly string LogFolderPath = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData) +
														Path.DirectorySeparatorChar +
														"Epic Games" +
														Path.DirectorySeparatorChar +
														"UnrealVS";

		private const string LogFileNameBase = "UnrealVS-log";
		private const string LogFileNameExt = ".txt";

		private static StreamWriter LogFile;
		private static bool bLoggingReady = false;

		private const int MaxFileSuffix = 64;

		public static void Initialize()
		{
			Initialize(0);
		}

		private static void Initialize(int FileSuffix)
		{
			try
			{
				if (!Directory.Exists(LogFolderPath))
				{
					Directory.CreateDirectory(LogFolderPath);
				}

				string LogFilePath = GetLogFilePath(FileSuffix);

				try
				{
					if (File.Exists(LogFilePath))
					{
						File.Delete(LogFilePath);
					}

					LogFile = new StreamWriter(LogFilePath);
					bLoggingReady = true;
				}
				catch (IOException)
				{
					if (MaxFileSuffix == FileSuffix) throw;

					Initialize(FileSuffix + 1);
				}
			}
			catch (Exception ex)
			{
				if (ex is ApplicationException) throw;

				bLoggingReady = false;
				throw new ApplicationException("Failed to init logging in UnrealVS", ex);
			}
		}

		private static string GetLogFilePath(int FileSuffix)
		{
			string Suffix = string.Empty;
			if (FileSuffix > 0)
			{
				Suffix = string.Format("({0})", FileSuffix);
			}

			return LogFolderPath +
					Path.DirectorySeparatorChar +
					LogFileNameBase +
					Suffix +
					LogFileNameExt;
		}

		public static void Close()
		{
			if (!bLoggingReady) return;

			LogFile.Close();
			LogFile = null;
			bLoggingReady = false;
		}

		public static void WriteLine(string Text)
		{
			Trace.WriteLine(Text);

			if (!bLoggingReady) return;

			LogFile.WriteLine(Text);
			LogFile.Flush();
		}
	}
}
