// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Diagnostics;
using System.IO;
using System.Web.Mvc;

using Tools.DotNETCommon.XmlHandler;
using Tools.CrashReporter.CrashReportCommon;
using Tools.CrashReporter.CrashReportWebSite.Models;

namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{
	/// <summary>
	/// The controller to handle the crash data.
	/// </summary>
	[HandleError]
	public class CrashesController : Controller
	{
		private CrashRepository LocalCrashRepository = new CrashRepository();

		/// <summary>
		/// An empty constructor.
		/// </summary>
		public CrashesController()
		{
		}

		/// <summary>
		/// Display a summary list of crashes based on the search criteria.
		/// </summary>
		/// <param name="CrashesForm">A form of user data passed up from the client.</param>
		/// <returns>A view to display a list of crash reports.</returns>
		public ActionResult Index( FormCollection CrashesForm )
		{
			// Handle any edits made in the Set form fields
			foreach( var Entry in CrashesForm )
			{
				int Id = 0;
				if( int.TryParse( Entry.ToString(), out Id ) )
				{
					Crash CurrentCrash = LocalCrashRepository.GetCrash( Id );
					if( CurrentCrash != null )
					{
						if( !string.IsNullOrEmpty( CrashesForm["SetStatus"] ) )
						{
							CurrentCrash.Status = CrashesForm["SetStatus"];
							LocalCrashRepository.SetCrashStatus( CurrentCrash.Status, Id );
						}

						if( !string.IsNullOrEmpty( CrashesForm["SetFixedIn"] ) )
						{
							CurrentCrash.FixedChangeList = CrashesForm["SetFixedIn"];
							LocalCrashRepository.SetCrashFixedChangeList( CurrentCrash.FixedChangeList, Id );
						}

						if( !string.IsNullOrEmpty( CrashesForm["SetTTP"] ) )
						{
							CurrentCrash.TTPID = CrashesForm["SetTTP"];
							LocalCrashRepository.SetCrashTTPID( CurrentCrash.TTPID, Id );
						}
					}
				}
			}

			// Parse the contents of the query string, and populate the form
			FormHelper FormData = new FormHelper( Request, CrashesForm, "TimeOfCrash" );
			CrashesViewModel Result = LocalCrashRepository.GetResults( FormData );

			// Add the FromCollection to the CrashesViewModel since we don't need it for the get results function but we do want to post it back to the page.
			Result.FormCollection = CrashesForm;

			return View( "Index", Result );
		}

		/// <summary>
		/// Show detailed information about a crash.
		/// </summary>
		/// <param name="CrashesForm">A form of user data passed up from the client.</param>
		/// <param name="Id">The unique id of the crash we wish to show the details of.</param>
		/// <returns>A view to show crash details.</returns>
		public ActionResult Show( FormCollection CrashesForm, int? Id )
		{
			if (!Id.HasValue)
			{
				return RedirectToAction("");
			}

			CallStackContainer CurrentCallStack = null;

			// Update the selected crash based on the form contents
			Crash CurrentCrash = LocalCrashRepository.GetCrash( Id.Value );

			if (CurrentCrash == null)
			{
				return RedirectToAction( "" );
			}

			if( !string.IsNullOrEmpty( CrashesForm["SetStatus"] ) )
			{
				CurrentCrash.Status = CrashesForm["SetStatus"];
				LocalCrashRepository.SetCrashStatus(CurrentCrash.Status, Id.Value);
			}

			if( !string.IsNullOrEmpty( CrashesForm["SetFixedIn"] ) )
			{
				CurrentCrash.FixedChangeList = CrashesForm["SetFixedIn"];
				LocalCrashRepository.SetCrashFixedChangeList(CurrentCrash.FixedChangeList, Id.Value);
			}

			if( !string.IsNullOrEmpty( CrashesForm["SetTTP"] ) )
			{
				CurrentCrash.TTPID = CrashesForm["SetTTP"];
				LocalCrashRepository.SetCrashTTPID(CurrentCrash.TTPID, Id.Value);
			}

			if( !string.IsNullOrEmpty( CrashesForm["Description"] ) )
			{
				CurrentCrash.Description = CrashesForm["Description"];
				LocalCrashRepository.SetCrashDescription(CurrentCrash.Description, Id.Value);
			}
			else
			{
				if( !string.IsNullOrEmpty( CurrentCrash.Description ) && string.IsNullOrEmpty( CrashesForm["Description"] ) )
				{
					CurrentCrash.Description = "";
					LocalCrashRepository.SetCrashDescription(CurrentCrash.Description, Id.Value);
				}
			}

			CurrentCallStack = new CallStackContainer( CurrentCrash );

			// Set callstack properties
			CurrentCallStack.bDisplayModuleNames = true;
			CurrentCallStack.bDisplayFunctionNames = true;
			CurrentCallStack.bDisplayFileNames = true;
			CurrentCallStack.bDisplayFilePathNames = true;
			CurrentCallStack.bDisplayUnformattedCallStack = false;

			CurrentCrash.CallStackContainer = CurrentCrash.GetCallStack();

			// Populate the crash with the correct user data
			LocalCrashRepository.PopulateUserInfo( CurrentCrash );

			return View( "Show", new CrashViewModel { Crash = CurrentCrash, CallStack = CurrentCallStack } );
		}

		/// <summary>
		/// Map a machine Guid to a user and machine name.
		/// </summary>
		/// <param name="UserName">A user name - typically 'first.last'.</param>
		/// <param name="MachineName">A machine name - e.g. Z2425</param>
		/// <param name="MachineGUID">A Guid for the client machine retrieved from the registry.</param>
		/// <returns>An empty result.</returns>
		/// <remarks>After mapping the user to the Guid, all machine Guids in crashes that match are updated.</remarks>
		public ActionResult RegisterPII( string UserName, string MachineName, string MachineGUID )
		{
			LocalCrashRepository.AddUserMapping( UserName, MachineName, MachineGUID );
			return Content( "", "text/xml" );
		}

		/// <summary>
		/// Add a crash passed in the payload as Xml to the database.
		/// </summary>
		/// <param name="id">Unused.</param>
		/// <returns>The row id of the newly added crash.</returns>
		public ActionResult AddCrash( int id )
		{
			CrashReporterResult NewCrashResult = new CrashReporterResult();
			NewCrashResult.ID = -1;

			try
			{
				using( StreamReader Reader = new StreamReader( Request.InputStream, Request.ContentEncoding ) )
				{
					string Result = Reader.ReadToEnd();
					CrashDescription NewCrash = XmlHandler.FromXmlString<CrashDescription>( Result );
					NewCrashResult.ID = LocalCrashRepository.AddNewCrash( NewCrash );
					NewCrashResult.bSuccess = true;
				}
			}
			catch( Exception Ex )
			{
				NewCrashResult.Message = Ex.ToString();
				NewCrashResult.bSuccess = false;
			}

			string ReturnResult = XmlHandler.ToXmlString<CrashReporterResult>( NewCrashResult );
			return Content( ReturnResult, "text/xml" );
		}
	}
}
