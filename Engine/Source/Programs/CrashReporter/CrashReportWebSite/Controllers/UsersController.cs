// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Web;
using System.Web.Mvc;

using Tools.CrashReporter.CrashReportWebSite.Models;

namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{
	/// <summary>
	/// The controller to handle associating users with groups.
	/// </summary>
	public class UsersController : Controller
	{
		/// <summary>
		/// Display the main user groups.
		/// </summary>
		/// <param name="UserForms">A form of user data passed up from the client.</param>
		/// <param name="UserGroup">The name of the current user group to display users for.</param>
		/// <returns>A view to show a list of users in the current user group.</returns>
		public ActionResult Index( FormCollection UserForms, string UserGroup )
		{
			// Examine an incoming form for a new user group
			foreach( var FormInstance in UserForms )
			{
				string UserName = FormInstance.ToString();
				string NewUserGroup = UserForms[UserName];
				FRepository.Get().SetUserGroup( UserName, NewUserGroup );
			}

			UsersViewModel Model = new UsersViewModel();

			Model.UserGroup = UserGroup;
			Model.Users = FRepository.Get().GetUserNamesFromGroupName( UserGroup );
			Model.GroupCounts = GetCountsByGroup();

			return View( "Index", Model );
		}

		/// <summary>
		/// Gets a container of crash counts per user group for all crashes.
		/// </summary>
		/// <returns>A dictionary of user group names, and the count of crashes for each group.</returns>
		public Dictionary<string, int> GetCountsByGroup()
		{
			// @TODO yrx 2014-11-06 Optimize?
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + " SQL OPT" ) )
			{
				Dictionary<string, int> Results = new Dictionary<string, int>();
				var Context = FRepository.Get().Context;

				try
				{
					var GroupCounts =
					(
						from UserDetail in Context.Users
						join UserGroupDetail in Context.UserGroups on UserDetail.UserGroupId equals UserGroupDetail.Id
						group UserDetail by UserGroupDetail.Name into GroupCount
						select new { Key = GroupCount.Key, Count = GroupCount.Count() }
					);

					foreach( var GroupCount in GroupCounts )
					{
						Results.Add( GroupCount.Key, GroupCount.Count );
					}

					// Add in all groups, even though there are no crashes associated
					IEnumerable<string> UserGroups = ( from UserGroupDetail in Context.UserGroups select UserGroupDetail.Name );
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
		}
	}
}
