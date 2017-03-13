// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Web.Mvc;
using Tools.CrashReporter.CrashReportWebSite.Controllers;
using System.IO;

namespace Tools.CrashReporter.CrashReportWebSite.Models
{
	/// <summary>
	/// The view model for the Crash summary page.
	/// </summary>
	public class CSV_ViewModel
	{
		/// <summary>A container of sorted CSV rows.</summary>
		public List<FCSVRow> CSVRows { get; set; }

		/// <summary>The date of the earliest Crash.</summary>
		public long DateFrom { get; set; }

		/// <summary>The date of the most recent Crash.</summary>
		public long DateTo { get; set; }

		/// <summary>The date of the earliest Crash.</summary>
		public DateTime DateTimeFrom { get; set; }

		/// <summary>The date of the most recent Crash.</summary>
		public DateTime DateTimeTo { get; set; }

		/// <summary>Unique Anonymous Crashes</summary>
		public int UniqueCrashesFiltered { get; set; }

		/// <summary>Total Affected Users</summary>
		public int AffectedUsersFiltered { get; set; }

		/// <summary>Total Crashes.</summary>
		public int TotalCrashes { get; set; }

		/// <summary>Total Crashes year to date.</summary>
		public int TotalCrashesYearToDate { get; set; }

		/// <summary>Total Crashes.</summary>
		public int CrashesFilteredWithDate { get; set; }

		/// <summary>Pathname where the CSV file is saved.</summary>
		public string CSVPathname { get; set; }

		/// <summary>Filename of the CSV file</summary>
		public string GetCSVFilename()
		{
			return Path.GetFileName( CSVPathname );
		}

		/// <summary>Directory where the CSV files are saved.</summary>
		public string GetCSVDirectory()
		{
			return Path.GetDirectoryName( CSVPathname );
		}

		/// <summary>Public directory where the CSV files are saved.</summary>
		public string GetCSVPublicDirectory()
		{
			return @"\\DEVWEB-02\CrashReporterCSV";
		}

		/// <summary>Time spent in generating this site, formatted as a string.</summary>
		public string GenerationTime { get; set; }

		/// <summary>User input from the client.</summary>
		public FormCollection FormCollection { get; set; }
	}
}