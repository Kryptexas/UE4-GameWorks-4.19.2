// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Web.Mvc;

using Tools.CrashReporter.CrashReportWebSite.Models;

namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{

	/// <summary>
	/// The controller to handle graphing of crashes per user group over time.
	/// </summary>
	public class ReportsController : Controller
	{
		/// <summary>Fake id for all user groups</summary>
		public static readonly int AllUserGroupId = -1;

		/// <summary></summary>
		private BuggRepository BuggRepository = new BuggRepository();
		/// <summary></summary>
		private CrashRepository CrashRepository = new CrashRepository();

		/// <summary></summary>
		public int GetIdFromUserGroup( string UserGroup )
		{
			var Group = CrashRepository.Context.UserGroups.Where( X => X.Name.Contains( UserGroup ) ).FirstOrDefault();
			return Group.Id;
		}

		/// <summary></summary>
		public HashSet<int> GetUserIdsFromUserGroup( string UserGroup )
		{
			int UserGroupId = GetIdFromUserGroup( UserGroup );
			return GetUserIdsFromUserGroupId( UserGroupId );
		}

		/// <summary></summary>
		public HashSet<int> GetUserIdsFromUserGroupId( int UserGroupId )
		{
			var UserIds = CrashRepository.Context.Users.Where( X => X.UserGroupId == UserGroupId ).Select( X => X.Id );
			return new HashSet<int>( UserIds );
		}

		/// <summary>
		/// Return a dictionary of crashes per group grouped by week.
		/// </summary>
		/// <param name="Crashes">A set of crashes to interrogate.</param>
		/// <param name="UserGroupId">The id of the user group to interrogate.</param>
		/// <returns>A dictionary of week vs. crash count.</returns>
		public Dictionary<DateTime, int> GetWeeklyCountsByGroup( List<FCrashMinimal> Crashes, int UserGroupId )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(UserGroupId=" + UserGroupId + ")" ) )
			{
				Dictionary<DateTime, int> Results = new Dictionary<DateTime, int>();

				var UsersIds = GetUserIdsFromUserGroupId( UserGroupId );

				// Trim crashes to user group.
				if( UserGroupId != DashboardController.AllUserGroupId )
				{
					Crashes = Crashes.Where( Crash => UsersIds.Contains( Crash.UserId ) ).ToList();
				}

				try
				{
					Results =
					(
						from CrashDetail in Crashes
						group CrashDetail by CrashDetail.TimeOfCrash.AddDays( -(int)CrashDetail.TimeOfCrash.DayOfWeek ).Date into GroupCount
						orderby GroupCount.Key
						select new { Count = GroupCount.Count(), Date = GroupCount.Key }
					).ToDictionary( x => x.Date, y => y.Count );
				}
				catch( Exception Ex )
				{
					Debug.WriteLine( "Exception in GetWeeklyCountsByGroup: " + Ex.ToString() );
				}

				return Results;
			}
		}

		/// <summary>
		/// Return a dictionary of crashes per group grouped by day.
		/// </summary>
		/// <param name="Crashes">A set of crashes to interrogate.</param>
		/// <param name="UserGroupId">The id of the user group to interrogate.</param>
		/// <returns>A dictionary of day vs. crash count.</returns>
		public Dictionary<DateTime, int> GetDailyCountsByGroup( List<FCrashMinimal> Crashes, int UserGroupId )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(UserGroupId=" + UserGroupId + ")" ) )
			{
				Dictionary<DateTime, int> Results = new Dictionary<DateTime, int>();

				var UsersIds = GetUserIdsFromUserGroupId( UserGroupId );

				// Trim crashes to user group.
				if( UserGroupId != DashboardController.AllUserGroupId )
				{
					Crashes = Crashes.Where( Crash => UsersIds.Contains( Crash.UserId ) ).ToList();
				}

				try
				{
					Results =
					(
						from CrashDetail in Crashes
						group CrashDetail by CrashDetail.TimeOfCrash.Date into GroupCount
						orderby GroupCount.Key
						select new { Count = GroupCount.Count(), Date = GroupCount.Key }
					).ToDictionary( x => x.Date, y => y.Count );
				}
				catch( Exception Ex )
				{
					Debug.WriteLine( "Exception in GetDailyCountsByGroup: " + Ex.ToString() );
				}

				return Results;
			}
		}

		System.Data.Linq.Table<FunctionCall> FunctionCalls
		{
			get
			{
				return this.CrashRepository.Context.FunctionCalls;
			}
		}


		void BuildPattern( Crash CrashInstance )
		{

			List<string> Pattern = new List<string>();

			// Get an array of callstack items
			CallStackContainer CallStack = new CallStackContainer( CrashInstance );
			CallStack.bDisplayFunctionNames = true;

			if( CrashInstance.Pattern == null )
			{
				// Set the module based on the modules in the callstack
				CrashInstance.Module = CallStack.GetModuleName();
				try
				{
					using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(Id=" + CrashInstance.Id + ")" ) )
					{
						foreach( CallStackEntry Entry in CallStack.CallStackEntries.Take( 64 ) )
						{
							FunctionCall CurrentFunctionCall = new FunctionCall();

							if( FunctionCalls.Where( f => f.Call == Entry.FunctionName ).Count() > 0 )
							{
								CurrentFunctionCall = FunctionCalls.Where( f => f.Call == Entry.FunctionName ).First();
							}
							else
							{
								CurrentFunctionCall = new FunctionCall();
								CurrentFunctionCall.Call = Entry.FunctionName;
								FunctionCalls.InsertOnSubmit( CurrentFunctionCall );
							}

							//CrashRepository.Context.SubmitChanges();

							Pattern.Add( CurrentFunctionCall.Id.ToString() );
						}

						//CrashInstance.Pattern = "+";
						CrashInstance.Pattern = string.Join( "+", Pattern );
						// We need something like this +1+2+3+5+ for searching for exact pattern like +5+
						//CrashInstance.Pattern += "+";

						CrashRepository.Context.SubmitChanges();
					}

				}
				catch( Exception Ex )
				{
					FLogger.WriteEvent( "Exception in BuildPattern: " + Ex.ToString() );
				}
			}
		}

		/// <summary>
		/// Retrieve all Buggs matching the search criteria.
		/// </summary>
		/// <param name="FormData">The incoming form of search criteria from the client.</param>
		/// <returns>A view to display the filtered Buggs.</returns>
		public ReportsViewModel GetResults( FormHelper FormData )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() ) )
			{
				string AnonumousGroup = "Anonymous";
				List<String> Users = CrashRepository.GetUsersForGroup( AnonumousGroup );
				int AnonymousGroupID = BuggRepository.GetIdFromUserGroup( AnonumousGroup );
				HashSet<int> AnonumousIDs = BuggRepository.GetUserIdsFromUserGroup( AnonumousGroup );
				int AnonymousID = AnonumousIDs.First();
				HashSet<string> UserNamesForUserGroup = BuggRepository.GetUserNamesFromUserGroups( AnonumousGroup );

				//FormData.DateFrom = DateTime.Now.AddDays( -1 );

				var Crashes = CrashRepository
					.FilterByDate( CrashRepository.ListAll(), FormData.DateFrom, FormData.DateTo.AddDays( 1 ) )
					// Only crashes and asserts
					.Where( Crash => Crash.CrashType == 1 || Crash.CrashType == 2 )
					// Only anonymous user
					.Where( Crash => Crash.UserNameId.Value == AnonymousID )
					.Select( Crash => new
					{
						ID = Crash.Id,
						TimeOfCrash = Crash.TimeOfCrash.Value,
						//UserID = Crash.UserNameId.Value, 
						BuildVersion = Crash.BuildVersion,
						JIRA = Crash.TTPID,
						Platform = Crash.PlatformName,
						FixCL = Crash.FixedChangeList,
						BuiltFromCL = Crash.ChangeListVersion,
						Pattern = Crash.Pattern,
						MachineID = Crash.ComputerName,
					} )
					.ToList();
				int NumCrashes = Crashes.Count;

				/*
				// Build patterns for crashes where patters is null.
				var CrashesWithoutPattern = CrashRepository
					.FilterByDate( CrashRepository.ListAll(), FormData.DateFrom, FormData.DateTo.AddDays( 1 ) )
					// Only crashes and asserts
					.Where( Crash => Crash.Pattern == null || Crash.Pattern == "" )
					.Select( Crash => Crash )
					.ToList();

				foreach( var Crash in CrashesWithoutPattern )
				{
					////BuildPattern( Crash );
				}
				*/

				// Total # of ALL (Anonymous) crashes in timeframe
				int TotalAnonymousCrashes = NumCrashes;

				// Total # of UNIQUE (Anonymous) crashes in timeframe
				HashSet<string> UniquePatterns = new HashSet<string>();
				HashSet<string> UniqueMachines = new HashSet<string>();
				Dictionary<string, int> PatternToCount = new Dictionary<string, int>();

				//List<int> CrashesWithoutPattern = new List<int>();
				//List<DateTime> CrashesWithoutPatternDT = new List<DateTime>();

				foreach( var Crash in Crashes )
				{
					if( string.IsNullOrEmpty( Crash.Pattern ) )
					{
						//CrashesWithoutPattern.Add( Crash.ID );
						//CrashesWithoutPatternDT.Add( Crash.TimeOfCrash );
						continue;
					}

					UniquePatterns.Add( Crash.Pattern );
					UniqueMachines.Add( Crash.MachineID );

					bool bAdd = !PatternToCount.ContainsKey( Crash.Pattern );
					if( bAdd )
					{
						PatternToCount.Add( Crash.Pattern, 1 );
					}
					else
					{
						PatternToCount[Crash.Pattern]++;
					}
				}
				var PatternToCountOrdered = PatternToCount.OrderByDescending( X => X.Value ).ToList();
				// The 100 top.
				var PatternAndCount = PatternToCountOrdered.Take( 100 ).ToDictionary( X => X.Key, Y => Y.Value );

				int TotalUniqueAnonymousCrashes = UniquePatterns.Count;

				// Total # of AFFECTED USERS (Anonymous) in timeframe
				int TotalAffectedUsers = UniqueMachines.Count;

				var RealBuggs = BuggRepository.GetDataContext().Buggs.Where( Bugg => PatternAndCount.Keys.Contains( Bugg.Pattern ) ).ToList();

				List<Bugg> Buggs = new List<Bugg>( 100 );
				foreach( var Top in PatternAndCount )
				{
					Bugg NewBugg = new Bugg();

					Bugg RealBugg = RealBuggs.Where( X => X.Pattern == Top.Key ).FirstOrDefault();
					if( RealBugg != null )
					{
						using( FAutoScopedLogTimer TopTimer = new FAutoScopedLogTimer( string.Format("{0}:{1}", Buggs.Count+1, RealBugg.Id ) ) )
						{
							// Index												// number
							NewBugg.Id = RealBugg.Id;								// CrashGroup URL (a link to the Bugg)
							NewBugg.TTPID = RealBugg.TTPID;							// JIRA
							NewBugg.FixedChangeList = RealBugg.FixedChangeList;		// FixCL
							NewBugg.NumberOfCrashes = Top.Value;					// # Occurrences

							//NewBugg.BuildVersion = 

							var CrashesForBugg = Crashes.Where( Crash => Crash.Pattern == Top.Key ).ToList();

							NewBugg.AffectedBuilds = new SortedSet<string>();

							HashSet<string> MachineIds = new HashSet<string>();
							foreach( var Crash in CrashesForBugg )
							{
								// Only add machine if the number has 32 characters
								if( Crash.MachineID != null && Crash.MachineID.Length == 32 )
								{
									MachineIds.Add( Crash.MachineID );
								}

								// Ignore bad build versions.
								if( Crash.BuildVersion.StartsWith( "4." ) )
								{
									NewBugg.AffectedBuilds.Add( Crash.BuildVersion );
								}
							}

							if( NewBugg.AffectedBuilds.Count > 0 )
							{
								NewBugg.BuildVersion = NewBugg.AffectedBuilds.Last();	// Latest Version Affected
							}

							NewBugg.NumberOfUniqueMachines = MachineIds.Count;		// # Affected Users
							string LatestCLAffected = CrashesForBugg.				// CL of the latest build
								Where( Crash => Crash.BuildVersion == NewBugg.BuildVersion ).
								Max( Crash => Crash.BuiltFromCL );

							int ILatestCLAffected = -1;
							int.TryParse( LatestCLAffected, out ILatestCLAffected );
							NewBugg.LatestCLAffected = ILatestCLAffected;			// Latest CL Affected

							string LatestOSAffected = CrashesForBugg.OrderByDescending( Crash => Crash.TimeOfCrash ).First().Platform;

							NewBugg.LatestOSAffected = LatestOSAffected;			// Latest Environment Affected

							NewBugg.TimeOfFirstCrash = RealBugg.TimeOfFirstCrash;	// First Crash Timestamp

							Buggs.Add( NewBugg );
						}
					}
					else
					{
						FLogger.WriteEvent( "Bugg for pattern " + Top.Key + " not found" );
					}
				}

				return new ReportsViewModel
				{
					Buggs = Buggs,
					/*Crashes = Crashes,*/
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
		/// <param name="ReportsForm"></param>
		/// <returns></returns>
		public ActionResult Index( FormCollection ReportsForm )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() ) )
			{
				FormHelper FormData = new FormHelper( Request, ReportsForm, "JustReport" );
				ReportsViewModel Results = GetResults( FormData );
				Results.GenerationTime = LogTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Index", Results );
			}
		}
	}
}