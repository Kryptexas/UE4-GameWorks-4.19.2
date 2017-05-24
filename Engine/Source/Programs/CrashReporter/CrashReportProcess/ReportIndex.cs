// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;

namespace Tools.CrashReporter.CrashReportProcess
{
	class ReportIndex
	{
		public void WriteToFile()
		{
			if (!IsEnabled) return;

			lock (FileLock)
			{
				try
				{
					if (File.Exists(Filepath))
					{
						File.Delete(Filepath + ".backup");
						File.Move(Filepath, Filepath + ".backup");
					}

					using (MemoryStream MemStream = new MemoryStream())
					{
						using (StreamWriter Writer = new StreamWriter(MemStream))
						{
							var Today = DateTime.UtcNow.Date;

							lock (IndexLock)
							{
								foreach (var Item in Index)
								{
									if (Today - Item.Value.Date <= Retention)
									{
										Writer.WriteLine(ItemToString(Item));
									}
								}
								Writer.Flush();
							}

							File.WriteAllBytes(Filepath, MemStream.ToArray());
						}
					}
					LastFlush = DateTime.UtcNow;
				}
				catch (Exception Ex)
				{
					CrashReporterProcessServicer.WriteException(string.Format("Failed to write ReportIndex to {0}. Exception was: {1}", Filepath, Ex), Ex);
					CrashReporterProcessServicer.StatusReporter.Alert("ReportIndex.WriteToFile", string.Format("Failed to write ReportIndex to {0}", Filepath),
					                                                  Config.Default.SlackAlertRepeatMinimumMinutes);
				}
			}
		}

		public void ReadFromFile()
		{
			if (!IsEnabled) return;

			try
			{
				lock (IndexLock)
				{
					Index = new Dictionary<string, DateTime>();

					lock (FileLock)
					{
						if (!File.Exists(Filepath))
						{
							if (File.Exists(Filepath + ".backup"))
							{
								CrashReporterProcessServicer.WriteFailure(string.Format("Failed to read ReportIndex from {0}. Attempting to read from {1}", Filepath,
																						Filepath + ".backup"));
								CrashReporterProcessServicer.StatusReporter.Alert("ReportIndex.ReadFromFile.UsingBackup",
																				  string.Format("Failed to read ReportIndex from {0}. Using backup.", Filepath),
																				  Config.Default.SlackAlertRepeatMinimumMinutes);
								File.Move(Filepath + ".backup", Filepath);
							}
							else
							{
								CrashReporterProcessServicer.WriteFailure(string.Format("Failed to read ReportIndex from {0}. Generating new one.", Filepath));
								File.Create(Filepath).Close();
							}
						}

						using (var Reader = File.OpenText(Filepath))
						{
							string ItemStringRaw;
							while ((ItemStringRaw = Reader.ReadLine()) != null)
							{
								string ItemString = ItemStringRaw.Trim();
								if (!string.IsNullOrWhiteSpace(ItemString))
								{
									KeyValuePair<string, DateTime> NewItem;
									if (!TryParseItemString(ItemString, out NewItem))
									{
										CrashReporterProcessServicer.WriteFailure(string.Format("Failed to read line from ReportIndex: {0}.", ItemString));
										continue;
									}
									Index.Add(NewItem.Key, NewItem.Value);
								}
							}
						}

						LastFlush = DateTime.UtcNow;
					} // lock (FileLock)
				} // lock (IndexLock)
			}
			catch (Exception Ex)
			{
				CrashReporterProcessServicer.WriteException(string.Format("Failed to read ReportIndex from {0}. Exception was: {1}", Filepath, Ex), Ex);
				CrashReporterProcessServicer.StatusReporter.Alert("ReportIndex.ReadFromFile", string.Format("Failed to read ReportIndex from {0}", Filepath), Config.Default.SlackAlertRepeatMinimumMinutes);
			}
		}

		public bool TryAddNewReport(string ReportKey)
		{
			if (!IsEnabled) return true;

			lock (IndexLock)
			{
				if (Index.ContainsKey(ReportKey))
				{
					return false;
				}

				Index.Add(ReportKey, DateTime.UtcNow);
			}

			lock (FileLock)
			{
				if (LastFlush.Date < DateTime.UtcNow.Date)
				{
					CrashReporterProcessServicer.WriteEvent("ReportIndex.TryAddNewReport flushing index to disk on schedule.");
					WriteToFile();
				}
			}

			return true;
		}

		public bool ContainsReport(string ReportKey)
		{
			if (!IsEnabled) return false;

			lock (IndexLock)
			{
				return Index.ContainsKey(ReportKey);
			}
		}

		public bool TryRemoveReport(string ReportKey)
		{
			if (!IsEnabled) return true;

			lock (IndexLock)
			{
				return Index.Remove(ReportKey);
			}
		}

		private static string ItemToString(KeyValuePair<string, DateTime> Item)
		{
			return string.Format("\"{0}\",{1}", Item.Key, Item.Value.ToString("yyyy-MM-dd"));
		}

		private static bool TryParseItemString(string ItemString, out KeyValuePair<string, DateTime> OutItem)
		{
			OutItem = new KeyValuePair<string, DateTime>();

			var ItemStringParts = ItemString.Split(',');
			if (ItemStringParts.Length != 2) return false;

			var KeyString = ItemStringParts[0].Trim('\"');
			if (KeyString.Length != ItemStringParts[0].Length - 2) return false;

			var DateStringParts = ItemStringParts[1].Split('-');
			if (DateStringParts.Length != 3) return false;

			int Year, Month, Day;
			if (!int.TryParse(DateStringParts[0], out Year) ||
			    !int.TryParse(DateStringParts[1], out Month) ||
			    !int.TryParse(DateStringParts[2], out Day))
			{
				return false;
			}

			DateTime ValueDate = new DateTime(Year, Month, Day, 0, 0, 0, DateTimeKind.Utc);

			OutItem = new KeyValuePair<string, DateTime>(KeyString, ValueDate);
			return true;
		}

		public bool IsEnabled { get; set; }
		public string Filepath { get; set; }
		public TimeSpan Retention { get; set; }

		private Dictionary<string, DateTime> Index = new Dictionary<string, DateTime>();
		private DateTime LastFlush = DateTime.MinValue;
		private Object IndexLock = new Object();
		private Object FileLock = new Object();
	}
}
