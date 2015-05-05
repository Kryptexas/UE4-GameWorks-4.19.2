// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Linq.Expressions;
using System.Net;
using System.Text;
using System.Web.Mvc;
using System.Xml;
using Tools.CrashReporter.CrashReportWebSite.Models;
using Tools.CrashReporter.CrashReportWebSite.Properties;
using Tools.DotNETCommon;


namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{
	// 
	// How hard would it be for you to create a CSV of:
	// Epic / Machine ID, 
	// Buggs ID, 
	// EngineVersion, 
	// 
	// # of crashes with that buggs ID for 
	//	this engine version and	user

	//E-32chars
	//M-32chars
	// 
	//Engine version: 4.8.1-cl+branch
	// 
	// Each time the controller is run, create a new CSV file in the specified folder and add the link and the top of the site,
	// so it could be easily downloaded
	// Removed files older that 7 days

	/// <summary>CSV data row.</summary>
	public class FCSVRow
	{
		/// <summary>Epic Id.</summary>
		public string EpicId = "";
		/// <summary>Bugg Id.</summary>
		public int BuggId = 0;
		/// <summary>Engine version.</summary>
		public string AffectedVersionsString = "";
		/// <summary># of crashes with that buggs ID for this engine version and user.</summary>
		public int NumberOfCrashes = 0;

		/// <summary>User's email.</summary>
		public string UserEmail = "";

		//// <summary>Reference to a bugg.</summary>
		//public Bugg BuggRef = null;
	}

	/// <summary>
	/// The controller to handle graphing of crashes per user group over time.
	/// </summary>
	public class CSVController : Controller
	{
		/// <summary>Fake id for all user groups</summary>
		public static readonly int AllUserGroupId = -1;

		static readonly int MinNumberOfCrashes = 2;

		/// <summary>
		/// Retrieve all Buggs matching the search criteria.
		/// </summary>
		/// <param name="FormData">The incoming form of search criteria from the client.</param>
		/// <returns>A view to display the filtered Buggs.</returns>
		public CSV_ViewModel GetResults( FormHelper FormData )
		{
			BuggRepository BuggsRepo = new BuggRepository();
			CrashRepository CrashRepo = new CrashRepository();

			// No Jira support at this moment
			// Enumerate JIRA projects if needed.
			// https://jira.ol.epicgames.net//rest/api/2/project
			var JC = JiraConnection.Get();
			var JiraComponents = JC.GetNameToComponents();
			var JiraVersions = JC.GetNameToVersions();

			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() ) )
			{
				string AnonymousGroup = "Anonymous";
				//List<String> Users = FRepository.Get().GetUserNamesFromGroupName( AnonumousGroup );
				int AnonymousGroupID = FRepository.Get( BuggsRepo ).FindOrAddGroup( AnonymousGroup );
				HashSet<int> AnonumousIDs = FRepository.Get( BuggsRepo ).GetUserIdsFromUserGroup( AnonymousGroup );
				int AnonymousID = AnonumousIDs.First();
				HashSet<string> UserNamesForUserGroup = FRepository.Get( BuggsRepo ).GetUserNamesFromGroupName( AnonymousGroup );

				FormData.DateFrom = FormData.DateTo.AddDays( -1 );

				var Crashes = CrashRepo
					.FilterByDate( CrashRepo.ListAll(), FormData.DateFrom, FormData.DateTo )
					// Only crashes and asserts
					.Where( Crash => Crash.CrashType == 1 || Crash.CrashType == 2 )
					// Only anonymous user
					.Where( Crash => Crash.UserNameId.Value == AnonymousID )
					// We need Epic ID
					// or Machine ID to identify the owner of a crash
					//.Where( Crash => !string.IsNullOrEmpty( Crash.EpicAccountId ) || !string.IsNullOrEmpty( Crash.MachineId ) )
					.Where( Crash => !string.IsNullOrEmpty( Crash.EpicAccountId ) )
					.Where( Crash => !string.IsNullOrEmpty( Crash.Pattern ) )
					.Select( Crash => new
					{
						ID = Crash.Id,
						TimeOfCrash = Crash.TimeOfCrash.Value,
						//UserID = Crash.UserNameId.Value, 
						BuildVersion = Crash.BuildVersion,
						JIRA = Crash.Jira,
						Platform = Crash.PlatformName,
						FixCL = Crash.FixedChangeList,
						BuiltFromCL = Crash.BuiltFromCL,
						Pattern = Crash.Pattern,
						MachineID = Crash.MachineId,
						Branch = Crash.Branch,
						Description = Crash.Description,
						RawCallStack = Crash.RawCallStack,
						EpicAccountId = Crash.EpicAccountId,
					} )
					.ToList();
				int NumCrashes = Crashes.Count;

				// Get all users
				// SLOW
				//var Users = FRepository.Get( BuggsRepo ).GetAnalyticsUsers().ToDictionary( X => X.EpicAccountId, Y => Y );

				// Total # of UNIQUE (Anonymous) crashes in timeframe
				Dictionary<string, int> PatternToCount = new Dictionary<string, int>();
				HashSet<string> UniqueEpicIds = new HashSet<string>();
				Dictionary<string, FCSVRow> EpicIdBuggToCSVRow = new Dictionary<string, FCSVRow>();

				// Iterate all crashes and generate all patterns, and EpicIds
				foreach( var Crash in Crashes )
				{
					if( string.IsNullOrEmpty( Crash.Pattern ) )
					{
						continue;
					}

					bool bAdd = !PatternToCount.ContainsKey( Crash.Pattern );
					if (bAdd)
					{
						PatternToCount.Add( Crash.Pattern, 1 );
					}
					else
					{
						PatternToCount[Crash.Pattern]++;
					}

					UniqueEpicIds.Add( Crash.EpicAccountId );
				}

				// Select only patterns with at least 2 crashes.
				var PatternToCountOrdered = PatternToCount.OrderByDescending( X => X.Value ).ToList();
				var PatternAndCount = PatternToCountOrdered.Where( X => X.Value > MinNumberOfCrashes ).ToDictionary( X => X.Key, Y => Y.Value );

				// Get all buggs associated with crashes.
				var RealBuggs = new Dictionary<string, Bugg>();
				foreach (var PatternIt in PatternAndCount)
				{
					var RealBugg = BuggsRepo.Context.Buggs.Where( Bugg => Bugg.Pattern == PatternIt.Key ).FirstOrDefault();
					if (RealBugg != null)
					{
						var CrashesForBugg = Crashes.Where( Crash => Crash.Pattern == PatternIt.Key ).ToList();

						// Convert anonymous to full type;
						var FullCrashesForBugg = new List<Crash>( CrashesForBugg.Count );
						foreach (var Anon in CrashesForBugg)
						{
							FullCrashesForBugg.Add( new Crash()
							{
								ID = Anon.ID,
								TimeOfCrash = Anon.TimeOfCrash,
								BuildVersion = Anon.BuildVersion,
								Jira = Anon.JIRA,
								Platform = Anon.Platform,
								FixCL = Anon.FixCL,
								BuiltFromCL = Anon.BuiltFromCL,
								Pattern = Anon.Pattern,
								MachineId = Anon.MachineID,
								Branch = Anon.Branch,
								Description = Anon.Description,
								RawCallStack = Anon.RawCallStack,
							} );
						}

						RealBugg.PrepareBuggForJira( FullCrashesForBugg );
						RealBuggs.Add( PatternIt.Key, RealBugg );
					}
					else
					{
						//FLogger.Global.WriteEvent( "Bugg for pattern " + PatternIt + " not found" );
					}
				}


				// Iterate all crashes and link EpicId with Bugg, and calculate #
				foreach( var Crash in Crashes )
				{
					Bugg Bugg = null;
					RealBuggs.TryGetValue( Crash.Pattern, out Bugg );

					if( Bugg != null )
					{
						string EpicIdBugg = Crash.EpicAccountId + "%" + Bugg.Id;
						bool bAdd = !EpicIdBuggToCSVRow.ContainsKey( EpicIdBugg );
						if (bAdd)
						{
							var Row = new FCSVRow 
							{ 
								EpicId = Crash.EpicAccountId,
								BuggId = Bugg.Id,
								AffectedVersionsString = Bugg.GetAffectedVersions(),
								NumberOfCrashes = 1,
								//UserEmail = Users[Crash.EpicAccountId].UserEmail
							};
							EpicIdBuggToCSVRow.Add( EpicIdBugg, Row );
						}
						else
						{
							EpicIdBuggToCSVRow[EpicIdBugg].NumberOfCrashes++;
						}
					}
				}

				// Export data to the file.
				string CSVPathname = Path.Combine( Settings.Default.CrashReporterCSV, DateTime.UtcNow.ToString( "yyyy-MM-dd.HH-mm-ss" ) ) + ".csv";
				string ServerPath = Server.MapPath( CSVPathname );
				var CSVFile = new StreamWriter( ServerPath, true, Encoding.UTF8 );

				CSVFile.WriteLine( "EpicId; BuggId; AffectedVersion; # of crashes;" );

				foreach (var Row in EpicIdBuggToCSVRow)
				{
					if (Row.Value.NumberOfCrashes > MinNumberOfCrashes)
					{
						CSVFile.WriteLine( "{0};{1};{2};{3};", Row.Value.EpicId, Row.Value.BuggId, Row.Value.AffectedVersionsString, Row.Value.NumberOfCrashes );
					}
				}

				CSVFile.Flush();
				CSVFile.Close();
				CSVFile = null;

				List<FCSVRow> CSVRows = EpicIdBuggToCSVRow
					.OrderByDescending( X => X.Value.NumberOfCrashes )
					.Select( X => X.Value )
					.Where( X => X.NumberOfCrashes > MinNumberOfCrashes )
					.ToList();
				
				// Total # of unique anonymous crashes.
				int TotalUniqueAnonymousCrashes = PatternToCount.Count;

				// Total # of AFFECTED USERS (Anonymous) in timeframe
				int TotalAffectedUsers = UniqueEpicIds.Count;

				// Total # of ALL (Anonymous) crashes in timeframe
				int TotalAnonymousCrashes = NumCrashes;

				return new CSV_ViewModel
				{
					CSVRows = CSVRows,
					CSVPathname = CSVPathname,
					DateFrom = (long)( FormData.DateFrom - CrashesViewModel.Epoch ).TotalMilliseconds,
					DateTo = (long)( FormData.DateTo - CrashesViewModel.Epoch ).TotalMilliseconds,
					TotalAffectedUsers = TotalAffectedUsers,
					TotalAnonymousCrashes = TotalAnonymousCrashes,
					TotalUniqueAnonymousCrashes = TotalUniqueAnonymousCrashes
				};
			}
		}


		/// <summary>
		/// 
		/// </summary>
		/// <returns></returns>
		public ActionResult Index( FormCollection Form )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString(), bCreateNewLog: true ) )
			{
				FormHelper FormData = new FormHelper( Request, Form, "JustReport" );
				CSV_ViewModel Results = GetResults( FormData );
				Results.GenerationTime = LogTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Index", Results );
			}
		}
	}
}