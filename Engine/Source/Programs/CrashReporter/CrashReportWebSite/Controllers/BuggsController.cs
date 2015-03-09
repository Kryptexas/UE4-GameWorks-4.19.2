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
	/// A controller to handle the Bugg data.
	/// </summary>
	public class BuggsController : Controller
	{
		/// <summary>
		/// An empty constructor.
		/// </summary>
		public BuggsController()
		{
		}

		/// <summary>
		/// The Index action.
		/// </summary>
		/// <param name="BuggsForm">The form of user data passed up from the client.</param>
		/// <returns>The view to display a list of Buggs on the client.</returns>
		public ActionResult Index( FormCollection BuggsForm )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() ) )
			{
				BuggRepository Buggs = new BuggRepository();

				FormHelper FormData = new FormHelper( Request, BuggsForm, "CrashesInTimeFrameGroup" );
				BuggsViewModel Results = Buggs.GetResults( FormData );
				Results.GenerationTime = LogTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Index", Results );
			}
		}

		/// <summary>
		/// The Show action.
		/// </summary>
		/// <param name="BuggsForm">The form of user data passed up from the client.</param>
		/// <param name="id">The unique id of the Bugg.</param>
		/// <returns>The view to display a Bugg on the client.</returns>
		public ActionResult Show( FormCollection BuggsForm, int id )
		{
			using( FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(BuggId=" + id + ")" ) )
			{
				BuggRepository Buggs = new BuggRepository();

				// Set the display properties based on the radio buttons
				bool DisplayModuleNames = false;
				if( BuggsForm["DisplayModuleNames"] == "true" )
				{
					DisplayModuleNames = true;
				}

				bool DisplayFunctionNames = false;
				if( BuggsForm["DisplayFunctionNames"] == "true" )
				{
					DisplayFunctionNames = true;
				}

				bool DisplayFileNames = false;
				if( BuggsForm["DisplayFileNames"] == "true" )
				{
					DisplayFileNames = true;
				}

				bool DisplayFilePathNames = false;
				if( BuggsForm["DisplayFilePathNames"] == "true" )
				{
					DisplayFilePathNames = true;
					DisplayFileNames = false;
				}

				bool DisplayUnformattedCallStack = false;
				if( BuggsForm["DisplayUnformattedCallStack"] == "true" )
				{
					DisplayUnformattedCallStack = true;
				}

				// Create a new view and populate with crashes
				List<Crash> Crashes = null;
				Bugg Bugg = new Bugg();

				BuggViewModel Model = new BuggViewModel();
				Bugg = Buggs.GetBugg( id );
				if( Bugg == null )
				{
					return RedirectToAction( "" );
				}

				// @TODO yrx 2015-02-17 JIRA
				using( FAutoScopedLogTimer GetCrashesTimer = new FAutoScopedLogTimer( "Bugg.GetCrashes().ToList" ) )
				{
					Crashes = Bugg.GetCrashes();
					Bugg.AffectedVersions = new SortedSet<string>();

					HashSet<string> MachineIds = new HashSet<string>();
					foreach( Crash Crash in Crashes )
					{
						MachineIds.Add( Crash.ComputerName );
						// Ignore bad build versions.
						if( Crash.BuildVersion.StartsWith( "4." ) )
						{
							Bugg.AffectedVersions.Add( Crash.BuildVersion );
						}

						if( Crash.User == null )
						{
							//??
						}
					}
					Bugg.NumberOfUniqueMachines = MachineIds.Count;
				}

				// Apply any user settings
				if( BuggsForm.Count > 0 )
				{
					if( !string.IsNullOrEmpty( BuggsForm["SetStatus"] ) )
					{
						Bugg.Status = BuggsForm["SetStatus"];
						Buggs.SetBuggStatus( Bugg.Status, id );
					}

					if( !string.IsNullOrEmpty( BuggsForm["SetFixedIn"] ) )
					{
						Bugg.FixedChangeList = BuggsForm["SetFixedIn"];
						Buggs.SetBuggFixedChangeList( Bugg.FixedChangeList, id );
					}

					if( !string.IsNullOrEmpty( BuggsForm["SetTTP"] ) )
					{
						Bugg.TTPID = BuggsForm["SetTTP"];
						Buggs.SetJIRAForBuggAndCrashes( Bugg.TTPID, id );
					}

					if( !string.IsNullOrEmpty( BuggsForm["Description"] ) )
					{
						Bugg.Description = BuggsForm["Description"];
					}

					// <STATUS>
				}

				// Set up the view model with the crash data
				Model.Bugg = Bugg;
				Model.Crashes = Crashes;

				Crash NewCrash = Model.Crashes.FirstOrDefault();
				if( NewCrash != null )
				{
					using( FScopedLogTimer LogTimer2 = new FScopedLogTimer( "CallstackTrimming" ) )
					{
						CallStackContainer CallStack = new CallStackContainer( NewCrash );

						// Set callstack properties
						CallStack.bDisplayModuleNames = DisplayModuleNames;
						CallStack.bDisplayFunctionNames = DisplayFunctionNames;
						CallStack.bDisplayFileNames = DisplayFileNames;
						CallStack.bDisplayFilePathNames = DisplayFilePathNames;
						CallStack.bDisplayUnformattedCallStack = DisplayUnformattedCallStack;

						Model.CallStack = CallStack;

						// Shorten very long function names.
						foreach( CallStackEntry Entry in Model.CallStack.CallStackEntries )
						{
							Entry.FunctionName = Entry.GetTrimmedFunctionName( 128 );
						}

						Model.SourceContext = NewCrash.SourceContext;
					}
				}

				/*using( FScopedLogTimer LogTimer2 = new FScopedLogTimer( "BuggsController.Show.PopulateUserInfo" + "(id=" + id + ")" ) )
				{
					// Add in the users for each crash in the Bugg
					foreach( Crash CrashInstance in Model.Crashes )
					{
						LocalCrashRepository.PopulateUserInfo( CrashInstance );
					}
				}*/
				Model.GenerationTime = LogTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Show", Model );
			}
		}
	}
}
