// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using Tools.CrashReporter.CrashReportWebSite.DataModels;

namespace Tools.CrashReporter.CrashReportWebSite.ViewModels
{
	/// <summary>
	/// The view model for the Crash show page.
	/// </summary>
	public class CrashViewModel
	{
		/// <summary>An instance of a Crash to interrogate.</summary>
		public Crash Crash { get; set; }

        /// <summary>An instance of a Crash to interrogate.</summary>
        public User User { get; set; }

        public UserGroup UserGroup { get; set; }

        /// <summary>An instance of a Crash to interrogate.</summary>
        public Bugg Bugg { get; set; }

		/// <summary>The callstack associated with the Crash.</summary>
		public CallStackContainer CallStack { get; set; }

		/// <summary>Time spent in generating this site, formatted as a string.</summary>
		public string GenerationTime { get; set; }
	}
}