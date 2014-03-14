// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Web.Mvc;

namespace Tools.CrashReporter.CrashReportWebSite.Models
{
	/// <summary>
	/// The view model for the Bugg index page.
	/// </summary>
	public class BuggsViewModel
	{
		/// <summary>A container of sorted Buggs.</summary>
		public IEnumerable<Bugg> Results { get; set; }
		/// <summary>Information to paginate the list of Buggs.</summary>
		public PagingInfo PagingInfo { get; set; }
		/// <summary>The criterion to sort by e.g. Game.</summary>
		public string Term { get; set; }
		/// <summary>Either ascending or descending.</summary>
		public string Order { get; set; }
		/// <summary>The current user group name.</summary>
		public string UserGroup { get; set; }
		/// <summary>The query that filtered the results.</summary>
		public string Query { get; set; }
		/// <summary>The date of the earliest crash in a Bugg.</summary>
		public long DateFrom { get; set; }
		/// <summary>The date of the most recent crash in a Bugg.</summary>
		public long DateTo { get; set; }
		/// <summary>A dictionary of the number of Buggs per user group.</summary>
		public Dictionary<string, int> GroupCounts { get; set; }

		/// <summary>The set of statuses a Bugg could have its status set to.</summary>
		public IEnumerable<string> SetStatus { get { return new List<string>( new string[] { "Unset", "Reviewed", "New", "Coder", "EngineQA", "GameQA" } ); } }
		/// <summary>User input from the client.</summary>
		public FormCollection FormCollection { get; set; }
	}
}