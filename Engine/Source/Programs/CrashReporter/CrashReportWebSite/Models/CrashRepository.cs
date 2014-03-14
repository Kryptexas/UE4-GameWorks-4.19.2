// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Data.Linq.SqlClient;
using System.Data.Linq;
using System.Diagnostics;
using System.Linq;
using System.Linq.Expressions;
using System.Web;

using Naspinski.IQueryableSearch;

using Tools.CrashReporter.CrashReportCommon;

namespace Tools.CrashReporter.CrashReportWebSite.Models
{
	/// <summary>
	/// A model to talk to the database.
	/// </summary>
	public class CrashRepository : IDisposable
	{
		private CrashReportDataContext CrashRepositoryDataContext;

		/// <summary>
		/// The default constructor.
		/// </summary>
		public CrashRepository()
		{
			CrashRepositoryDataContext = new CrashReportDataContext();
		}

		/// <summary>
		/// Implementing Dispose.
		/// </summary>
		public void Dispose()
		{
			Dispose( true );
			GC.SuppressFinalize( this );
		}

		/// <summary>
		/// Disposes the resources.
		/// </summary>
		/// <param name="Disposing">true if the Dispose call is from user code, and not system code.</param>
		protected virtual void Dispose( bool Disposing )
		{
			CrashRepositoryDataContext.Dispose();
		}

		/// <summary>
		/// Return the list of users belonging to the user group.
		/// </summary>
		/// <param name="UserGroupName">The user group name to interrogate.</param>
		/// <returns>The list of users in the requested user group.</returns>
		public List<string> GetUsersForGroup( string UserGroupName )
		{
			List<string> Users = new List<string>();
			try
			{
				int UserGroupId = FindOrAddUserGroup( UserGroupName );
				Users =
				(
					from UserDetail in CrashRepositoryDataContext.Users
					where UserDetail.UserGroupId == UserGroupId
					orderby UserDetail.UserName
					select UserDetail.UserName
				).ToList();
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in GetUsersForGroup: " + Ex.ToString() );
			}

			return Users;
		}

		/// <summary>
		/// Gets the crash from an id.
		/// </summary>
		/// <param name="Id">The id of the crash we wish to examine.</param>
		/// <returns>The crash with the requested id.</returns>
		public Crash GetCrash( int Id )
		{
			try
			{
				IQueryable<Crash> Crashes =
				(
					from CrashDetail in CrashRepositoryDataContext.Crashes
					where CrashDetail.Id == Id
					select CrashDetail
				);

				return Crashes.FirstOrDefault();
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in GetCrash: " + Ex.ToString() );
			}

			return null;
		}

		/// <summary>
		/// Sets the status of a crash.
		/// </summary>
		/// <param name="Status">The new status of the crash (from a predetermined set).</param>
		/// <param name="Id">The id of the crash to update the status.</param>
		public void SetCrashStatus( string Status, int Id )
		{
			try
			{
				string Query = "UPDATE Crashes SET Status = {0} WHERE Id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, Status, Id );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetCrashStatus: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Sets the changelist the crash was fixed in.
		/// </summary>
		/// <param name="FixedChangeList">The string determining the changelist the crash was fixed.</param>
		/// <param name="Id">The id of the fixed crash.</param>
		public void SetCrashFixedChangeList( string FixedChangeList, int Id )
		{
			try
			{
				string Query = "UPDATE Crashes SET FixedChangeList = {0} WHERE Id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, FixedChangeList, Id );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetCrashFixedChangeList: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Sets the TTP ID associated with the crash.
		/// </summary>
		/// <param name="TTPID">A string representing a TTP.</param>
		/// <param name="Id">The id of the crash to update.</param>
		public void SetCrashTTPID( string TTPID, int Id )
		{
			try
			{
				string Query = "UPDATE Crashes SET TTPID = {0} WHERE Id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, TTPID, Id );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetCrashTTPID: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Sets the description of the crash.
		/// </summary>
		/// <param name="Description">A description of the crash.</param>
		/// <param name="Id">The id of the crash to update.</param>
		public void SetCrashDescription( string Description, int Id )
		{
			try
			{
				string Query = "UPDATE Crashes SET Description = {0} WHERE Id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, Description, Id );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetDescription: " + Ex.ToString() );
			}
		}


		/// <summary>
		/// Sets the status for all crashes in a Bugg.
		/// </summary>
		/// <param name="Status">The new status of all crashes.</param>
		/// <param name="BuggId">The id of the Bugg to update the crashes for.</param>
		public void SetBuggStatus( string Status, int BuggId )
		{
			try
			{
				string Query = "UPDATE Crashes SET Status = {0} WHERE Id IN ( SELECT CrashId FROM Buggs_Crashes WHERE BuggId = {1} )";
				CrashRepositoryDataContext.ExecuteCommand( Query, Status, BuggId );

				Query = "UPDATE Buggs SET Status = {0} WHERE id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, Status, BuggId );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetBuggStatus: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Sets the fixed in changelist for all crashes in a Bugg.
		/// </summary>
		/// <param name="FixedChangeList">A string representing a revision.</param>
		/// <param name="BuggId">The id of the Bugg to update the crashes for.</param>
		public void SetBuggFixedChangeList( string FixedChangeList, int BuggId )
		{
			try
			{
				string Query = "UPDATE Crashes SET FixedChangeList = {0} WHERE Id IN ( SELECT CrashId FROM Buggs_Crashes WHERE BuggId = {1} )";
				CrashRepositoryDataContext.ExecuteCommand( Query, FixedChangeList, BuggId );

				Query = "UPDATE Buggs SET FixedChangeList = {0} WHERE id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, FixedChangeList, BuggId );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetBuggFixedChangeList: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Sets the TTPID for all crashes in a Bugg.
		/// </summary>
		/// <param name="TTPID">A string representing a TTP.</param>
		/// <param name="BuggId">The id of the Bugg to update the crashes for.</param>
		public void SetBuggTTPID( string TTPID, int BuggId )
		{
			try
			{
				string Query = "UPDATE Crashes SET TTPID = {0} WHERE Id IN ( SELECT CrashId FROM Buggs_Crashes WHERE BuggId = {1} )";
				CrashRepositoryDataContext.ExecuteCommand( Query, TTPID, BuggId );

				Query = "UPDATE Buggs SET TTPID = {0} WHERE id = {1}";
				CrashRepositoryDataContext.ExecuteCommand( Query, TTPID, BuggId );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in SetBuggTTPID: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Sets the UserName and UserGroupName as derived data for a crash.
		/// </summary>
		/// <param name="CrashInstance">An instance of a crash we wish to augment with additional data.</param>
		public void PopulateUserInfo( Crash CrashInstance )
		{
			try
			{
				var query = (
					from UserDetail in CrashRepositoryDataContext.Users
					where UserDetail.Id == CrashInstance.User.Id
					join UserGroupDetail in CrashRepositoryDataContext.UserGroups on UserDetail.UserGroupId equals UserGroupDetail.Id
					select UserGroupDetail.Name
				);

				CrashInstance.UserGroupName = query.First();

			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in PopulateUserInfo: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// List all crashes from most recent to oldest.
		/// </summary>
		/// <returns>A container of all known crashes.</returns>
		public IQueryable<Crash> ListAll()
		{
			IQueryable<Crash> Crashes = null;
			try
			{
				Crashes =
				(
					from CrashDetail in CrashRepositoryDataContext.Crashes
					select CrashDetail
				).OrderByDescending( CrashDetail => CrashDetail.TimeOfCrash );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in ListAll: " + Ex.ToString() );
			}

			return Crashes;
		}

		/// <summary>
		/// Select a slice of crashes from the filtered set.
		/// </summary>
		/// <param name="Skip">The number of crashes to skip over.</param>
		/// <param name="Take">The number of crashes to return.</param>
		/// <returns>A container with a subset of crashes.</returns>
		public IQueryable<Crash> List( int Skip, int Take )
		{
			IQueryable<Crash> Crashes = null;
			try
			{
				Crashes =
				(
					from CrashDetail in CrashRepositoryDataContext.Crashes
					select CrashDetail
				).OrderByDescending( CrashDetail => CrashDetail.TimeOfCrash )
				.Skip( Skip )
				.Take( Take );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in List: " + Ex.ToString() );
			}

			return Crashes;
		}

		/// <summary>
		/// Apply a dynamic query to a set of results.
		/// </summary>
		/// <param name="Results">The initial set of results.</param>
		/// <param name="Query">The query to apply.</param>
		/// <returns>A set of results filtered by the query.</returns>
		public IQueryable<Crash> Search( IQueryable<Crash> Results, string Query )
		{
			IQueryable<Crash> Crashes = null;
			try
			{
				string DecodedQuery = HttpUtility.HtmlDecode( Query );
				if( DecodedQuery == null )
				{
					DecodedQuery = "";
				}

				string[] Terms = DecodedQuery.Split( "-, ;+".ToCharArray(), StringSplitOptions.RemoveEmptyEntries );
				string TermsToUse = "";
				foreach( string Term in Terms )
				{
					if( !TermsToUse.Contains( Term ) )
					{
						TermsToUse = TermsToUse + "+" + Term;
					}
				}

				// Search the results by the search terms using IQueryable search
				Crashes = ( IQueryable<Crash> )Results.Search( TermsToUse.Split( "+".ToCharArray() ) );
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in Search: " + Ex.ToString() );
				Crashes = Results;
			}

			return Crashes;
		}

		/// <summary>
		/// Return a view model containing a sorted list of crashes based on the user input.
		/// </summary>
		/// <param name="FormData">The user input from the client.</param>
		/// <returns>A view model to use to display a list of crashes.</returns>
		/// <remarks>The filtering mechanism works by generating a fluent query to describe the set of crashes we wish to view. Basically, 'Select.Where().Where().Where().Where()' etc.
		/// The filtering is applied in the following order:
		/// 1. Get all crashes.
		/// 2. Filter between the start and end dates.
		/// 3. Apply the search query.
		/// 4. Filter by the branch.
		/// 5. Filter by the game name.
		/// 6. Filter by the type of reports to view.
		/// 7. Filter by the user group.
		/// 8. Sort the results by the sort term.
		/// 9. Take one page of results.</remarks>
		public CrashesViewModel GetResults( FormHelper FormData )
		{
			IQueryable<Crash> Results = null;
			int Skip = ( FormData.Page - 1 ) * FormData.PageSize;
			int Take = FormData.PageSize;

			Results = ListAll();
			Results = FilterByDate( Results, FormData.DateFrom, FormData.DateTo );

			// Grab Results 
			if( !string.IsNullOrEmpty( FormData.SearchQuery ) )
			{
				Results = Search( Results, FormData.SearchQuery );
			}

			// Start Filtering the results

			// Filter by BranchName
			if( !string.IsNullOrEmpty( FormData.BranchName ) )
			{
				if( FormData.BranchName.StartsWith( "-" ) )
				{
					Results =
					(
						from CrashDetail in Results
						where !CrashDetail.Branch.Contains( FormData.BranchName.Substring( 1 ) )
						select CrashDetail
					);
				}
				else
				{
					Results =
					(
						from CrashDetail in Results
						where CrashDetail.Branch.Contains( FormData.BranchName )
						select CrashDetail
					);
				}
			}

			// Filter by GameName
			if( !string.IsNullOrEmpty( FormData.GameName ) )
			{
				if( FormData.GameName.StartsWith( "-" ) )
				{
					Results =
					(
						from CrashDetail in Results
						where !CrashDetail.GameName.Contains( FormData.GameName.Substring( 1 ) )
						select CrashDetail
					);
				}
				else
				{
					Results =
					(
						from CrashDetail in Results
						where CrashDetail.GameName.Contains( FormData.GameName )
						select CrashDetail
					);
				}
			}

			// Filter by Crash Type
			if( FormData.CrashType != "All" )
			{
				switch (FormData.CrashType)
				{
					case "Crashes":
						Results = Results.Where(CrashInstance => CrashInstance.CrashType == 1);
						break;
					case "Assert":
						Results = Results.Where(CrashInstance => CrashInstance.CrashType == 2);
						break;
					case "Ensure":
						Results = Results.Where(CrashInstance => CrashInstance.CrashType == 3);
						break;
					case "CrashesAsserts":
						Results = Results.Where(CrashInstance => CrashInstance.CrashType == 1 || CrashInstance.CrashType == 2);
						break;
				}
			}

			// Get UserGroup ResultCounts
			Dictionary<string, int> GroupCounts = GetCountsByGroup( Results );

			// Filter by user group if present
            int UserGroupId;
            if (!string.IsNullOrEmpty(FormData.UserGroup))
            {
                UserGroupId = FindOrAddUserGroup(FormData.UserGroup);
            }
            else
            {
                UserGroupId = 1;
            }
			Results =
			(
				from CrashDetail in Results
				from UserDetail in CrashRepositoryDataContext.Users
				where UserDetail.UserGroupId == UserGroupId &&
					(CrashDetail.UserNameId == UserDetail.Id || CrashDetail.UserName == UserDetail.UserName)
				select CrashDetail
			);	

			// Pass in the results and return them sorted properly
			Results = GetSortedResults( Results, FormData.SortTerm, ( FormData.SortOrder == "Descending" ) );

			// Get the Count for pagination
			int ResultCount = Results.Count();

			// Grab just the results we want to display on this page
			Results = Results.Skip( Skip ).Take( Take );

			// Process call stack for display
			foreach( Crash CrashInstance in Results )
			{
				// Put callstacks into an list so we can access them line by line in the view
				CrashInstance.CallStackContainer = GetCallStack( CrashInstance );
			}

			return new CrashesViewModel
			{
				Results = Results,
				PagingInfo = new PagingInfo { CurrentPage = FormData.Page, PageSize = FormData.PageSize, TotalResults = ResultCount },
				Order = FormData.SortOrder,
				Term = FormData.SortTerm,
				UserGroup = FormData.UserGroup,
				CrashType = FormData.CrashType,
				Query = FormData.SearchQuery,
				DateFrom = (long)(FormData.DateFrom - CrashesViewModel.Epoch).TotalMilliseconds,
				DateTo = (long)(FormData.DateTo - CrashesViewModel.Epoch).TotalMilliseconds,
				BranchName = FormData.BranchName,
				GameName = FormData.GameName,
				GroupCounts = GroupCounts,
			};
		}

		/// <summary>
		/// Gets a container of crash counts per user group for all crashes.
		/// </summary>
		/// <returns>A dictionary of user group names, and the count of crashes for each group.</returns>
		public Dictionary<string, int> GetCountsByGroup()
		{
			Dictionary<string, int> Results = new Dictionary<string, int>();

			try
			{
				var GroupCounts =
				(
					from UserDetail in CrashRepositoryDataContext.Users
					join UserGroupDetail in CrashRepositoryDataContext.UserGroups on UserDetail.UserGroupId equals UserGroupDetail.Id
					group UserDetail by UserGroupDetail.Name into GroupCount
					select new { Key = GroupCount.Key, Count = GroupCount.Count() }
				);

				foreach( var GroupCount in GroupCounts )
				{
					Results.Add( GroupCount.Key, GroupCount.Count );
				}

				// Add in all groups, even though there are no crashes associated
				IEnumerable<string> UserGroups = ( from UserGroupDetail in CrashRepositoryDataContext.UserGroups select UserGroupDetail.Name );
				foreach( string UserGroupName in UserGroups )
				{
					if( !Results.Keys.Contains( UserGroupName ) )
					{
						Results[UserGroupName] = 0;
					}
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in GetCountsByGroup: " + Ex.ToString() );
			}

			return Results;
		}

		/// <summary>
		/// Gets a container of crash counts per user group for the set of crashes passed in.
		/// </summary>
		/// <param name="Crashes">The set of crashes to tabulate by user group.</param>
		/// <returns>A dictionary of user group names, and the count of Buggs for each group.</returns>
		public Dictionary<string, int> GetCountsByGroup( IQueryable<Crash> Crashes )
		{
			Dictionary<string, int> Results = new Dictionary<string, int>();

			try
			{
				Results =
				(
#if WOOBLE
					from CrashDetail in Crashes
					join UserDetail in CrashRepositoryDataContext.Users on CrashDetail.UserNameId equals UserDetail.Id
					join UserGroupDetail in CrashRepositoryDataContext.UserGroups on UserDetail.UserGroupId equals UserGroupDetail.Id
					group CrashDetail by UserGroupDetail.Name into GroupCount
#else
					 from UserDetail in CrashRepositoryDataContext.Users
					 join UserGroupDetail in CrashRepositoryDataContext.UserGroups on UserDetail.UserGroupId equals UserGroupDetail.Id
					 group UserDetail by UserGroupDetail.Name into GroupCount
#endif
					select new { Key = GroupCount.Key, Count = GroupCount.Count() }
				).ToDictionary(x => x.Key, y => y.Count);

				// Add in all groups, even though there are no crashes associated
				IEnumerable<string> UserGroups = ( from UserGroupDetail in CrashRepositoryDataContext.UserGroups select UserGroupDetail.Name );
				foreach( string UserGroupName in UserGroups )
				{
					if( !Results.Keys.Contains( UserGroupName ) )
					{
						Results[UserGroupName] = 0;
					}
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in GetCountsByGroup: " + Ex.ToString() );
			}

			return Results;
		}

		/// <summary>
		/// Return a dictionary of crashes per group grouped by week.
		/// </summary>
		/// <param name="Crashes">A set of crashes to interrogate.</param>
		/// <param name="UserGroupId">The id of the user group to interrogate.</param>
		/// <param name="UndefinedUserGroupId">Id of the undefined user group.</param>
		/// <returns>A dictionary of week vs. crash count.</returns>
		public Dictionary<DateTime, int> GetWeeklyCountsByGroup( IQueryable<Crash> Crashes, int UserGroupId, int UndefinedUserGroupId )
		{
			Dictionary<DateTime, int> Results = new Dictionary<DateTime, int>();

			try
			{
				Results =
				(
					from CrashDetail in Crashes
					join UserDetail in CrashRepositoryDataContext.Users on CrashDetail.UserNameId equals UserDetail.Id
					where ( UserGroupId < 0 && UserDetail.UserGroupId != UndefinedUserGroupId ) || UserDetail.UserGroupId == UserGroupId
					group CrashDetail by CrashDetail.TimeOfCrash.Value.AddDays( -( int )CrashDetail.TimeOfCrash.Value.DayOfWeek ).Date into GroupCount
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

		/// <summary>
		/// Return a dictionary of crashes per group grouped by day.
		/// </summary>
		/// <param name="Crashes">A set of crashes to interrogate.</param>
		/// <param name="UserGroupId">The id of the user group to interrogate.</param>
		/// <param name="UndefinedUserGroupId">Id of the undefined user group.</param>
		/// <returns>A dictionary of day vs. crash count.</returns>
		public Dictionary<DateTime, int> GetDailyCountsByGroup( IQueryable<Crash> Crashes, int UserGroupId, int UndefinedUserGroupId )
		{
			Dictionary<DateTime, int> Results = new Dictionary<DateTime, int>();

			try
			{
				Results =
				(
					from CrashDetail in Crashes
					join UserDetail in CrashRepositoryDataContext.Users on CrashDetail.UserNameId equals UserDetail.Id
					where ( UserGroupId < 0 && UserDetail.UserGroupId != UndefinedUserGroupId ) || UserDetail.UserGroupId == UserGroupId
					group CrashDetail by CrashDetail.TimeOfCrash.Value.Date into GroupCount
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

		/// <summary>
		/// Filter a set of crashes to a date range.
		/// </summary>
		/// <param name="Results">The unfiltered set of crashes.</param>
		/// <param name="DateFrom">The earliest date to filter by.</param>
		/// <param name="DateTo">The latest date to filter by.</param>
		/// <returns>The set of crashes between the earliest and latest date.</returns>
		public IQueryable<Crash> FilterByDate( IQueryable<Crash> Results, DateTime DateFrom, DateTime DateTo )
		{
			Results = Results.Where( CrashInstance => CrashInstance.TimeOfCrash >= DateFrom && CrashInstance.TimeOfCrash <= DateTo.AddDays( 1 ) );
			return Results;
		}

		public static IQueryable<TSource> OrderBy<TSource, TKey>(IQueryable<TSource> Query, Expression<Func<TSource, TKey>> Predicate, bool bDescending)
		{
			return bDescending ? Query.OrderByDescending(Predicate) : Query.OrderBy(Predicate);
		}

		/// <summary>
		/// Apply the sort term to a set of crashes.
		/// </summary>
		/// <param name="Results">The unsorted set of crashes.</param>
		/// <param name="SortTerm">The term to sort by. e.g. UserName</param>
		/// <param name="bSortByDescending">Whether to sort ascending or descending.</param>
		/// <returns>A sorted set of crashes.</returns>
		public IQueryable<Crash> GetSortedResults( IQueryable<Crash> Results, string SortTerm, bool bSortByDescending )
		{
			switch( SortTerm )
			{
				case "Id":
					Results = OrderBy(Results, CrashInstance => CrashInstance.Id, bSortByDescending);
					break;

				case "TimeOfCrash":
					Results = OrderBy(Results, CrashInstance => CrashInstance.TimeOfCrash, bSortByDescending);
					break;

				case "UserName":
					// Note: only works where user is stored by Id
					Results = OrderBy(Results.Join( CrashRepositoryDataContext.Users, CrashInstance => CrashInstance.UserNameId, UserInstance => UserInstance.Id,
													( CrashInstance, UserInstance ) => new { CrashInstance, UserInstance.UserName } ),
													Joined => Joined.UserName, bSortByDescending).Select( Joined => Joined.CrashInstance);
					break;

				case "RawCallStack":
					Results = OrderBy(Results, CrashInstance => CrashInstance.RawCallStack, bSortByDescending);
					break;

				case "GameName":
					Results = OrderBy(Results, CrashInstance => CrashInstance.GameName, bSortByDescending);
					break;

				case "EngineMode":
					Results = OrderBy(Results, CrashInstance => CrashInstance.EngineMode, bSortByDescending);
					break;

				case "FixedChangeList":
					Results = OrderBy(Results, CrashInstance => CrashInstance.FixedChangeList, bSortByDescending);
					break;

				case "TTPID":
					Results = OrderBy(Results, CrashInstance => CrashInstance.TTPID, bSortByDescending);
					break;

				case "Branch":
					Results = OrderBy(Results, CrashInstance => CrashInstance.Branch, bSortByDescending);
					break;

				case "ChangeListVersion":
					Results = OrderBy(Results, CrashInstance => CrashInstance.ChangeListVersion, bSortByDescending);
					break;

				case "ComputerName":
					Results = OrderBy(Results, CrashInstance => CrashInstance.ComputerName, bSortByDescending);
					break;

				case "PlatformName":
					Results = OrderBy(Results, CrashInstance => CrashInstance.PlatformName, bSortByDescending);
					break;

				case "Status":
					Results = OrderBy(Results, CrashInstance => CrashInstance.Status, bSortByDescending);
					break;

				case "Module":
					Results = OrderBy(Results, CrashInstance => CrashInstance.Module, bSortByDescending);
					break;
			}

			return Results;
		}

		/// <summary>
		/// Retrieve the parsed callstack for the crash.
		/// </summary>
		/// <param name="CrashInstance">The crash we require the callstack for.</param>
		/// <returns>A class containing the parsed callstack.</returns>
		public CallStackContainer GetCallStack( Crash CrashInstance )
		{
			CachedDataService CachedResults = new CachedDataService( HttpContext.Current.Cache, this );
			return CachedResults.GetCallStack( CrashInstance );
		}

		/// <summary>
		/// Build a callstack pattern for a crash to ease bucketing of crashes into Buggs.
		/// </summary>
		/// <param name="CrashInstance">A crash that was recently added to the database.</param>
		public void BuildPattern( Crash CrashInstance )
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
					foreach( CallStackEntry Entry in CallStack.CallStackEntries )
					{
						FunctionCall CurrentFunctionCall = new FunctionCall();

						if( CrashRepositoryDataContext.FunctionCalls.Where( f => f.Call == Entry.FunctionName ).Count() > 0 )
						{
							CurrentFunctionCall = CrashRepositoryDataContext.FunctionCalls.Where( f => f.Call == Entry.FunctionName ).First();
						}
						else
						{
							CurrentFunctionCall = new FunctionCall();
							CurrentFunctionCall.Call = Entry.FunctionName;
							CrashRepositoryDataContext.FunctionCalls.InsertOnSubmit( CurrentFunctionCall );
						}

						int Count = CrashRepositoryDataContext.Crash_FunctionCalls.Where( c => c.CrashId == CrashInstance.Id && c.FunctionCallId == CurrentFunctionCall.Id ).Count();
						if( Count < 1 )
						{
							Crash_FunctionCall JoinTable = new Crash_FunctionCall();
							JoinTable.Crash = CrashInstance;
							JoinTable.FunctionCall = CurrentFunctionCall;
							CrashRepositoryDataContext.Crash_FunctionCalls.InsertOnSubmit( JoinTable );
						}

						CrashRepositoryDataContext.SubmitChanges();

						Pattern.Add( CurrentFunctionCall.Id.ToString() );
					}

					CrashInstance.Pattern = string.Join( "+", Pattern );
					CrashRepositoryDataContext.SubmitChanges();
				}
				catch( Exception Ex )
				{
					Debug.WriteLine( "Exception in BuildPattern: " + Ex.ToString() );
				}
			}
		}

		/// <summary>
		/// Sets a user group for a given user.
		/// </summary>
		/// <param name="UserName">The user name to update.</param>
		/// <param name="UserGroupName">The user group to add the user to.</param>
		public void SetUserGroup( string UserName, string UserGroupName )
		{
			try
			{
				IQueryable<int> UserGroups = ( from UserGroupDetail in CrashRepositoryDataContext.UserGroups
											   where UserGroupDetail.Name.ToLower() == UserGroupName.ToLower()
											   select UserGroupDetail.Id );

				IQueryable<int> UserNames = ( from UserDetail in CrashRepositoryDataContext.Users
										  where UserDetail.UserName.ToLower() == UserName.ToLower()
										  select UserDetail.Id );

				if( UserGroups.Count() == 1 && UserNames.Count() == 1 )
				{
					string Query = "UPDATE Users SET UserGroupId = {0} WHERE Id = {1}";
					CrashRepositoryDataContext.ExecuteCommand( Query, UserGroups.First(), UserNames.First() );
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in AdSetUserGroupdUser: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Find a user group, or add it if it doesn't exist. Return the id of the user group.
		/// </summary>
		/// <param name="UserGroupName">The user group name to find or add.</param>
		/// <returns>The id of the user group that was found or added.</returns>
		/// <remarks>All user group interaction is done this way to remove any dependencies on pre-populated tables.</remarks>
		public int FindOrAddUserGroup( string UserGroupName )
		{
			int UserGroupNameId = 0;
			try
			{
				IQueryable<int> UserGroups = ( from UserGroupDetail in CrashRepositoryDataContext.UserGroups
										  where UserGroupDetail.Name.ToLower() == UserGroupName.ToLower()
										  select UserGroupDetail.Id );

				// If there is no existing user, add a new one
				if( UserGroups.Count() == 0 )
				{
					UserGroup NewUserGroup = new UserGroup();
					NewUserGroup.Name = UserGroupName;
					CrashRepositoryDataContext.UserGroups.InsertOnSubmit( NewUserGroup );

					CrashRepositoryDataContext.SubmitChanges();
					UserGroupNameId = NewUserGroup.Id;
				}
				else
				{
					UserGroupNameId = UserGroups.First();
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in AddUserGroup: " + Ex.ToString() );
			}

			return UserGroupNameId;
		}

		/// <summary>
		/// Map a Guid from a Windows Error Report to a user name and machine name.
		/// </summary>
		/// <param name="UserName">The user name to map.</param>
		/// <param name="MachineName">The machine name to map.</param>
		/// <param name="MachineGUID">The Guid of the machine to map to.</param>
		public void AddUserMapping( string UserName, string MachineName, string MachineGUID )
		{
			try
			{
				// Find if we've already mapped this guid
				int MachineGUIDCount = ( from PIIMappingDetail in CrashRepositoryDataContext.PIIMappings
										 where PIIMappingDetail.MachineGUID == MachineGUID
										 select PIIMappingDetail.ID ).Count();

				// If there is no mapping, add a new one
				if( MachineGUIDCount == 0 )
				{
					int UserNameId = CrashRepositoryDataContext.FindOrAddUser( UserName );

					PIIMapping NewMapping = new PIIMapping();
					NewMapping.UserNameId = UserNameId;
					NewMapping.MachineName = MachineName;
					NewMapping.MachineGUID = MachineGUID;

					CrashRepositoryDataContext.PIIMappings.InsertOnSubmit( NewMapping );
					CrashRepositoryDataContext.SubmitChanges();

					// Fix up the crashes with this GUID we just mapped
					string UpdateQuery = "UPDATE Crashes SET ComputerName = '" + NewMapping.MachineName + "', UserNameId = '" + NewMapping.UserNameId + "' WHERE ComputerName = '" + MachineGUID + "'";
					CrashRepositoryDataContext.ExecuteCommand( UpdateQuery );
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in AddUserMapping: " + Ex.ToString() );
			}
		}

		/// <summary>
		/// Extract the changelist from the 4 digit version number.
		/// </summary>
		/// <param name="NewCrash">The crash to extract the changelist for.</param>
		/// <returns>The changelist number the build was made from.</returns>
		private int DeriveChangelist( Crash NewCrash )
		{
			int Changelist = 0;
			Version ApplicationVersion = null;

			if( Version.TryParse( NewCrash.BuildVersion, out ApplicationVersion ) )
			{
				Changelist = ApplicationVersion.Build * 65536 + ApplicationVersion.Revision;
			}

			return Changelist;
		}

		/// <summary>
		/// Add a new crash to the db based on a CrashDescription sent from the client.
		/// </summary>
		/// <param name="NewCrashInfo">The crash description sent from the client.</param>
		/// <returns>The id of the newly added crash.</returns>
		public int AddNewCrash( CrashDescription NewCrashInfo )
		{
			int NewID = -1;

			PIIMapping Mapping = ( from PIIMappingDetail in CrashRepositoryDataContext.PIIMappings
									where PIIMappingDetail.MachineGUID == NewCrashInfo.MachineGuid
									select PIIMappingDetail ).FirstOrDefault();

			Crash NewCrash = new Crash();
			NewCrash.Branch = NewCrashInfo.BranchName;
			NewCrash.BaseDir = NewCrashInfo.BaseDir;
			NewCrash.BuildVersion = NewCrashInfo.BuildVersion;
			NewCrash.ChangeListVersion = DeriveChangelist( NewCrash ).ToString();
			NewCrash.CommandLine = NewCrashInfo.CommandLine.Trim( "!".ToCharArray() );
			NewCrash.EngineMode = NewCrashInfo.EngineMode;

			NewCrash.ComputerName = NewCrashInfo.MachineGuid;
			NewCrash.UserNameId = CrashRepositoryDataContext.FindOrAddUser( "Anonymous" );
			if( Mapping != null )
			{
				NewCrash.ComputerName = Mapping.MachineName;
				NewCrash.UserNameId = Mapping.UserNameId;
			}

			NewCrash.Description = "";
			if( NewCrashInfo.UserDescription != null )
			{
				NewCrash.Description = string.Join( Environment.NewLine, NewCrashInfo.UserDescription );
			}

			NewCrash.EngineMode = NewCrashInfo.EngineMode;
			NewCrash.GameName = NewCrashInfo.GameName;
			NewCrash.LanguageExt = NewCrashInfo.Language;		// Convert
			NewCrash.PlatformName = NewCrashInfo.Platform;
			if( NewCrashInfo.CallStack != null )
			{
				NewCrash.RawCallStack = string.Join( "\n", NewCrashInfo.CallStack );
			}

			if( NewCrashInfo.SourceContext != null )
			{
				NewCrash.SourceContext = string.Join( "\n", NewCrashInfo.SourceContext );
			}

			NewCrash.TimeOfCrash = NewCrashInfo.TimeofCrash;
					
			NewCrash.HasLogFile = NewCrashInfo.bHasLog;
			NewCrash.HasMiniDumpFile = NewCrashInfo.bHasMiniDump;
			NewCrash.HasDiagnosticsFile = NewCrashInfo.bHasDiags;
			NewCrash.HasVideoFile = NewCrashInfo.bHasVideo;
			NewCrash.HasMetaData = NewCrashInfo.bHasWERData;

			NewCrash.TTPID = "";
			NewCrash.FixedChangeList = "";

			// Set the crash type
			NewCrash.CrashType = 1;
			if (NewCrash.RawCallStack != null)
			{
				if (NewCrash.RawCallStack.Contains("FDebug::AssertFailed()"))
				{
					NewCrash.CrashType = 2;
				}
				else if (NewCrash.RawCallStack.Contains("FDebug::EnsureFailed()"))
				{
					NewCrash.CrashType = 3;
				}
			}

			// As we're adding it, the status is always new
			NewCrash.Status = "New";

			// Add the crash to the database
			CrashRepositoryDataContext.Crashes.InsertOnSubmit( NewCrash );
			CrashRepositoryDataContext.SubmitChanges();

			NewID = NewCrash.Id;

			// Build a callstack pattern for crash bucketing
			BuildPattern( NewCrash );

			return NewID;
		}
	}

	public partial class CrashReportDataContext
	{
		/// <summary>
		/// Find a user to the database, and add them if they do not exist.
		/// </summary>
		/// <param name="UserName">The user name to find or add.</param>
		/// <param name="UserGroupId">The id of the user group to add the user to.</param>
		/// <returns>The id of the found or added user.</returns>
		/// <remarks>All user interaction is done this way to remove any dependencies on pre-populated tables.</remarks>
		public int FindOrAddUser( string UserName, int UserGroupId = 1 )
		{
			int UserNameId = 0;
			try
			{
				IQueryable<int> UserNames = ( from UserDetail in Users
										  where UserDetail.UserName.ToLower() == UserName.ToLower()
										  select UserDetail.Id );

				// If there is no existing user, add a new one
				if( UserNames.Count() == 0 )
				{
					User NewUser = new User();
					NewUser.UserName = UserName;
					NewUser.UserGroupId = UserGroupId;
					Users.InsertOnSubmit( NewUser );

					SubmitChanges();
					UserNameId = NewUser.Id;
				}
				else
				{
					UserNameId = UserNames.First();
				}
			}
			catch( Exception Ex )
			{
				Debug.WriteLine( "Exception in AddUser: " + Ex.ToString() );
			}

			return UserNameId;
		}
	}
}
