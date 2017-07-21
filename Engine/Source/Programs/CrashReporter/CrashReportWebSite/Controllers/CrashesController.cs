// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Data.Entity;
using System.Data.Entity.Validation;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Web;
using System.Web.Mvc;
using Tools.CrashReporter.CrashReportWebSite.Classes;
using Tools.CrashReporter.CrashReportWebSite.DataModels;
using Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories;
using Tools.CrashReporter.CrashReportWebSite.Properties;
using Tools.CrashReporter.CrashReportWebSite.ViewModels;
using Tools.CrashReporter.CrashReportCommon;
using System.Data.SqlClient;
using System.Runtime.Caching;

namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{
	/// <summary>
	/// The controller to handle the Crash data.
	/// </summary>
	[HandleError]
	public class CrashesController : Controller
    {
        private static MemoryCache crashCache = MemoryCache.Default;

        //Ugly instantiation of Crash repository will replace with dependency injection BEFORE this gets anywhere near live.
	    private readonly SlackWriter _slackWriter;

	    private static readonly PerformanceTracker PerfTracker = new PerformanceTracker(100);
        
        /// <summary> Special user name, currently used to mark Crashes from UE4 releases. </summary>
        const string UserNameAnonymous = "Anonymous";

		/// <summary>
		/// An empty constructor.
		/// </summary>
		public CrashesController()
		{
		    _slackWriter = new SlackWriter()
		    {
		        WebhookUrl = Settings.Default.SlackWebhookUrl,
		        Channel = Settings.Default.SlackChannel,
		        Username = Settings.Default.SlackUsername,
		        IconEmoji = Settings.Default.SlackEmoji
		    };
		}

        class CrashCacheItem
        {
            public List<Crash> Crashes { get; set; }
            public Dictionary<string, int> GroupCount { get; set; }
            public int ResultCount { get; set; }
        }

        private string getCacheKeyFromFormData(FormHelper formData)
        {
            return string.Format("{0}-{1}-{2}-{3}-{4}-{5}-{6}-{7}-{8}-{9}-{10}-{11}-{12}-{13}-{14}-{15}-{16}-{17}-{18}-{19}-{20}-{21}-{22}",
                formData.BranchName,
                formData.BuggId,
                formData.BuiltFromCL,
                formData.CrashType,
                formData.DateFrom,
                formData.DateTo,
                formData.EngineMode,
                formData.EngineVersion,
                formData.EpicIdOrMachineQuery,
                formData.GameName,
                formData.JiraId,
                formData.JiraQuery,
                formData.MessageQuery,
                formData.Page,
                formData.PageSize,
                formData.PlatformName,
                formData.PreviousOrder,
                formData.SearchQuery,
                formData.SortOrder,
                formData.SortTerm,
                formData.UserGroup,
                formData.UsernameQuery,
                formData.VersionName);
        }

		/// <summary>
		/// Display a summary list of Crashes based on the search criteria.
		/// </summary>
		/// <param name="CrashesForm">A form of user data passed up from the client.</param>
		/// <returns>A view to display a list of Crash reports.</returns>
		public ActionResult Index(FormCollection CrashesForm )
		{
            using( var logTimer = new FAutoScopedLogTimer( this.GetType().ToString()) )
			{
				// Handle any edits made in the Set form fields
                //foreach( var Entry in CrashesForm )
                //{
                //    int Id = 0;
                //    if( int.TryParse( Entry.ToString(), out Id ) )
                //    {
                //        Crash currentCrash = _unitOfWork.CrashRepository.GetById(Id);
                //        if( currentCrash != null )
                //        {
                //            if( !string.IsNullOrEmpty( CrashesForm["SetStatus"] ) )
                //            {
                //                currentCrash.Status = CrashesForm["SetStatus"];
                //            }

                //            if( !string.IsNullOrEmpty( CrashesForm["SetFixedIn"] ) )
                //            {
                //                currentCrash.FixedChangeList = CrashesForm["SetFixedIn"];
                //            }

                //            if( !string.IsNullOrEmpty( CrashesForm["SetTTP"] ) )
                //            {
                //                currentCrash.Jira = CrashesForm["SetTTP"];
                //            }
                //        }
                //    }

                //    _unitOfWork.Save();
                //}

				// <STATUS>

				// Parse the contents of the query string, and populate the form
				var formData = new FormHelper( Request, CrashesForm, "TimeOfCrash" );
				var result = GetResults( formData );

			    using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
			    {
			        result.BranchNames = unitOfWork.CrashRepository.GetBranchesAsListItems();
                    result.VersionNames = unitOfWork.CrashRepository.GetVersionsAsListItems();
                    result.PlatformNames = unitOfWork.CrashRepository.GetPlatformsAsListItems();
                    result.EngineModes = unitOfWork.CrashRepository.GetEngineModesAsListItems();
                    result.EngineVersions = unitOfWork.CrashRepository.GetEngineVersionsAsListItems();
			    }

                // Add the FromCollection to the CrashesViewModel since we don't need it for the get results function but we do want to post it back to the page.
				result.FormCollection = CrashesForm;
				result.GenerationTime = logTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Index", result );
			}
		}

		/// <summary>
		/// Show detailed information about a Crash.
		/// </summary>
		/// <param name="CrashesForm">A form of user data passed up from the client.</param>
		/// <param name="id">The unique id of the Crash we wish to show the details of.</param>
		/// <returns>A view to show Crash details.</returns>
		public ActionResult Show(FormCollection CrashesForm, int id)
		{
			using( var logTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(CrashId=" + id + ")", bCreateNewLog: true ) )
			{
				CallStackContainer currentCallStack = null;
                Crash currentCrash = null;
			    User CrashUser = null;
			    Bugg currentBugg = null;
			    UserGroup currentUserGroup = null;

				// Update the selected Crash based on the form contents
			    using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
			    {
                    currentCrash = unitOfWork.CrashRepository.GetById(id);
			        CrashUser = currentCrash.User;
			        currentBugg = currentCrash.Bugg;
			        currentUserGroup = CrashUser.UserGroup;
			    }

			    if( currentCrash == null )
				{
					return RedirectToAction( "Index" );
				}

				string FormValue;

				FormValue = CrashesForm["SetStatus"];
				if( !string.IsNullOrEmpty( FormValue ) )
				{
					currentCrash.Status = FormValue;
				}

				FormValue = CrashesForm["SetFixedIn"];
				if( !string.IsNullOrEmpty( FormValue ) )
				{
					currentCrash.FixedChangeList = FormValue;
				}

				FormValue = CrashesForm["SetTTP"];
				if( !string.IsNullOrEmpty( FormValue ) )
				{
					currentCrash.Jira = FormValue;
                    if(currentCrash.Bugg != null)
                        currentCrash.Bugg.TTPID = FormValue;
				}

				// Valid to set description to an empty string
				FormValue = CrashesForm["Description"];
				if( FormValue != null )
				{
					currentCrash.Description = FormValue;
				}
			    using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
			    {
			        unitOfWork.CrashRepository.Update(currentCrash);
			        unitOfWork.Save();
			    }

			    currentCallStack = new CallStackContainer( currentCrash.CrashType, currentCrash.RawCallStack, currentCrash.PlatformName );
			    currentCrash.Module = currentCallStack.GetModuleName();

				//Set call stack properties
				currentCallStack.bDisplayModuleNames = true;
				currentCallStack.bDisplayFunctionNames = true;
				currentCallStack.bDisplayFileNames = true;
				currentCallStack.bDisplayFilePathNames = true;
				currentCallStack.bDisplayUnformattedCallStack = false;

			    currentCrash.CallStackContainer = currentCallStack;

				var Model = new CrashViewModel { Crash = currentCrash, User = CrashUser, Bugg = currentBugg, UserGroup = currentUserGroup, CallStack = currentCallStack };
				Model.GenerationTime = logTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Show", Model );
			}
		}

		/// <summary>
		/// Add a Crash passed in the payload as Xml to the database.
		/// </summary>
		/// <param name="id">Unused.</param>
		/// <returns>The row id of the newly added Crash.</returns>
		public ActionResult AddCrash(int id)
		{
		    var newCrashResult = new CrashReporterResult();
		    CrashDescription newCrash;
		    newCrashResult.ID = -1;
		    string payloadString;
            
            using (var logTimer = new FAutoScopedLogTimer(this.GetType().ToString() 
                + "(CrashId=" + id + ")"))
            {
		        //Read the request payload
		        try
		        {
		            using (var reader = new StreamReader(Request.InputStream, Request.ContentEncoding))
		            {
		                payloadString = reader.ReadToEnd();
		                if (string.IsNullOrEmpty(payloadString))
		                {
		                    FLogger.Global.WriteEvent(string.Format("Add Crash Failed : Payload string empty"));
		                }
		            }
		        }
		        catch (Exception ex)
		        {
		            var messageBuilder = new StringBuilder();
		            messageBuilder.AppendLine("Error Reading Crash Payload");
		            messageBuilder.AppendLine("Exception was:");
		            messageBuilder.AppendLine(ex.ToString());

		            FLogger.Global.WriteException(messageBuilder.ToString());

		            newCrashResult.Message = messageBuilder.ToString();
		            newCrashResult.bSuccess = false;

		            return Content(XmlHandler.ToXmlString<CrashReporterResult>(newCrashResult), "text/xml");
		        }

		        // De-serialize the payload string
		        try
		        {
		            newCrash = XmlHandler.FromXmlString<CrashDescription>(payloadString);
		        }
		        catch (Exception ex)
		        {
		            var messageBuilder = new StringBuilder();
		            messageBuilder.AppendLine("Error Reading CrashDescription XML");
		            messageBuilder.AppendLine("Exception was: ");
		            messageBuilder.AppendLine(ex.ToString());

		            FLogger.Global.WriteException(messageBuilder.ToString());

		            newCrashResult.Message = messageBuilder.ToString();
		            newCrashResult.bSuccess = false;

		            return Content(XmlHandler.ToXmlString<CrashReporterResult>(newCrashResult), "text/xml");
		        }

		        //Add Crash to database
		        try
		        {
                    var crash = CreateCrash(newCrash);
		                newCrashResult.ID = crash.Id;
		                newCrashResult.bSuccess = true;
		        }
		        catch (DbEntityValidationException dbentEx)
		        {
		            var messageBuilder = new StringBuilder();
		            messageBuilder.AppendLine("Exception was:");
		            messageBuilder.AppendLine(dbentEx.ToString());

		            var innerEx = dbentEx.InnerException;
		            while (innerEx != null)
		            {
		                messageBuilder.AppendLine("Inner Exception : " + innerEx.Message);
		                innerEx = innerEx.InnerException;
		            }

		            if (dbentEx.EntityValidationErrors != null)
		            {
		                messageBuilder.AppendLine("Validation Errors : ");
		                foreach (var valErr in dbentEx.EntityValidationErrors)
		                {
		                    messageBuilder.AppendLine(
		                        valErr.ValidationErrors.Select(data => data.ErrorMessage)
		                            .Aggregate((current, next) => current + "; /n" + next));
		                }
		            }

		            messageBuilder.AppendLine("Received payload was:");
		            messageBuilder.AppendLine(payloadString);

		            FLogger.Global.WriteException(messageBuilder.ToString());
		            _slackWriter.Write(messageBuilder.ToString());

		            newCrashResult.Message = messageBuilder.ToString();
		            newCrashResult.bSuccess = false;
		        }
		        catch (SqlException sqlExc)
		        {
		            if (sqlExc.Number == -2) //If this is an sql timeout log the timeout and try again.
		            {
		                FLogger.Global.WriteEvent(string.Format("AddCrash: Timeout"));
		                newCrashResult.bTimeout = true;
		            }
		            else
		            {
		                var messageBuilder = new StringBuilder();
		                messageBuilder.AppendLine("Exception was:");
		                messageBuilder.AppendLine(sqlExc.ToString());
		                messageBuilder.AppendLine("Received payload was:");
		                messageBuilder.AppendLine(payloadString);

		                FLogger.Global.WriteException(messageBuilder.ToString());

		                newCrashResult.Message = messageBuilder.ToString();
		                newCrashResult.bSuccess = false;
		            }
		        }
		        catch (Exception ex)
		        {
		            var messageBuilder = new StringBuilder();
		            messageBuilder.AppendLine("Exception was:");
		            messageBuilder.AppendLine(ex.ToString());
		            messageBuilder.AppendLine("Received payload was:");
		            messageBuilder.AppendLine(payloadString);

		            FLogger.Global.WriteException(messageBuilder.ToString());

		            newCrashResult.Message = messageBuilder.ToString();
		            newCrashResult.bSuccess = false;
		        }

		        var returnResult = XmlHandler.ToXmlString<CrashReporterResult>(newCrashResult);

                PerfTracker.AddStat("Add Crash Complete", logTimer.GetElapsedSeconds());
                PerfTracker.IncrementCount();
                
		        return Content(returnResult, "text/xml");
		    }
		}

        /// <summary>
        /// Gets a list of Crashes filtered based on our form data.
        /// </summary>
        /// <param name="formData"></param>
        /// <returns></returns>
        public CrashesViewModel GetResults(FormHelper formData)
        {
            List<Crash> results = null;
            IQueryable<Crash> resultsQuery = null;

            var cacheKey = getCacheKeyFromFormData(formData);
            var cachedResults = crashCache.Get(cacheKey) as CrashCacheItem;

            if (cachedResults == null)
            {
                var skip = (formData.Page - 1) * formData.PageSize;
                var take = formData.PageSize;

                Dictionary<string, int> groupCounts;
                var resultCount = 0;
                using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
                {
                    resultsQuery = ConstructQueryForFiltering(unitOfWork, formData);

                    // Filter by data and get as enumerable.
                    resultsQuery = FilterByDate(resultsQuery, formData.DateFrom, formData.DateTo);

                    // Filter by BuggId
                    if (!string.IsNullOrEmpty(formData.BuggId))
                    {
                        var buggId = 0;
                        var bValid = int.TryParse(formData.BuggId, out buggId);

                        if (bValid)
                        {
                            var newBugg = unitOfWork.BuggRepository.GetById(buggId);

                            if (newBugg != null)
                            {
                                resultsQuery = resultsQuery.Where(data => data.PatternId == newBugg.PatternId);
                            }
                        }
                    }

                    var countsQuery =
                        resultsQuery.GroupBy(data => data.User.UserGroup)
                            .Select(data => new { Key = data.Key.Name, Count = data.Count() });
                    groupCounts = countsQuery.OrderBy(data => data.Key)
                        .ToDictionary(data => data.Key, data => data.Count);

                    // Filter by user group if present
                    var userGroupId = !string.IsNullOrEmpty(formData.UserGroup)
                        ? unitOfWork.UserGroupRepository.First(data => data.Name == formData.UserGroup).Id
                        : 1;
                    resultsQuery = resultsQuery.Where(data => data.User.UserGroupId == userGroupId);

                    var orderedQuery = GetSortedQuery(resultsQuery, formData.SortTerm ?? "TimeOfCrash",
                        formData.SortOrder == "Descending");

                    // Grab just the results we want to display on this page
                    results = orderedQuery.Skip(skip).Take(take).ToList();

                    // Get the Count for pagination
                    resultCount = orderedQuery.Count();

                    // Process call stack for display
                    foreach (var CrashInstance in results)
                    {
                        // Put call stacks into an list so we can access them line by line in the view
                        CrashInstance.CallStackContainer = new CallStackContainer(CrashInstance.CrashType, CrashInstance.RawCallStack, CrashInstance.PlatformName);
                    }

                    cachedResults = new CrashCacheItem()
                    {
                        Crashes = results,
                        GroupCount = groupCounts,
                        ResultCount = resultCount
                    };

                    var itemCachePolicy = new CacheItemPolicy() { AbsoluteExpiration = DateTimeOffset.Now.AddMinutes(Settings.Default.CrashesCacheInMinutes) };

                    crashCache.Add(cacheKey, cachedResults, itemCachePolicy);
                }
            }

            return new CrashesViewModel
            {
                Results = cachedResults.Crashes,
                PagingInfo = new PagingInfo { CurrentPage = formData.Page, PageSize = formData.PageSize, TotalResults = cachedResults.ResultCount },
                SortOrder = formData.SortOrder,
                SortTerm = formData.SortTerm,
                UserGroup = formData.UserGroup,
                CrashType = formData.CrashType,
                SearchQuery = formData.SearchQuery,
                UsernameQuery = formData.UsernameQuery,
                EpicIdOrMachineQuery = formData.EpicIdOrMachineQuery,
                MessageQuery = formData.MessageQuery,
                BuiltFromCL = formData.BuiltFromCL,
                BuggId = formData.BuggId,
                JiraQuery = formData.JiraQuery,
                DateFrom = (long)(formData.DateFrom.ToUniversalTime() - CrashesViewModel.Epoch).TotalMilliseconds,
                DateTo = (long)(formData.DateTo.ToUniversalTime() - CrashesViewModel.Epoch).TotalMilliseconds,
                BranchName = formData.BranchName,
                VersionName = formData.VersionName,
                PlatformName = formData.PlatformName,
                GameName = formData.GameName,
                GroupCounts = cachedResults.GroupCount
            };
        }

	    /// <summary>
	    /// Returns a lambda expression used to sort our Crash data.
	    /// </summary>
	    /// <param name="resultsQuery">A query filter on the Crashes entity.</param>
	    /// <param name="sortTerm">Sort term identifying the field on which to sort.</param>
	    /// <param name="sortDescending">bool indicating sort order</param>
	    /// <returns></returns>
        public IOrderedQueryable<Crash> GetSortedQuery(IQueryable<Crash> resultsQuery, string sortTerm, bool sortDescending)
	    {
            switch (sortTerm)
            {
                case "Id":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.Id) : resultsQuery.OrderBy(data => data.Id);
                    break;
                case "TimeOfCrash":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.TimeOfCrash) : resultsQuery.OrderBy(CrashInstance => CrashInstance.TimeOfCrash);
                    break;
                case "UserName":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.User.UserName) : resultsQuery.OrderBy(CrashInstance => CrashInstance.User.UserName);
                    break;
                case "RawCallStack":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.RawCallStack) : resultsQuery.OrderBy(CrashInstance => CrashInstance.RawCallStack);
                    break;
                case "GameName":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.GameName) : resultsQuery.OrderBy(CrashInstance => CrashInstance.GameName);
                    break;
                case "EngineMode":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.EngineMode) : resultsQuery.OrderBy(CrashInstance => CrashInstance.EngineMode);
                    break;
                case "FixedChangeList":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.FixedChangeList) : resultsQuery.OrderBy(CrashInstance => CrashInstance.FixedChangeList);
                    break;
                case "TTPID":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.Jira) : resultsQuery.OrderBy(CrashInstance => CrashInstance.Jira);
                    break;
                case "Branch":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.Branch) : resultsQuery.OrderBy(CrashInstance => CrashInstance.Branch);
                    break;
                case "ChangeListVersion":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.ChangelistVersion) : resultsQuery.OrderBy(CrashInstance => CrashInstance.ChangelistVersion);
                    break;
                case "ComputerName":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.ComputerName) : resultsQuery.OrderBy(CrashInstance => CrashInstance.ComputerName);
                    break;
                case "PlatformName":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.PlatformName) : resultsQuery.OrderBy(CrashInstance => CrashInstance.PlatformName);
                    break;
                case "Status":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.Status) : resultsQuery.OrderBy(CrashInstance => CrashInstance.Status);
                    break;
                case "Module":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.Module) : resultsQuery.OrderBy(CrashInstance => CrashInstance.Module);
                    break;
                case "Summary":
                    return sortDescending ? resultsQuery.OrderByDescending(data => data.Summary) : resultsQuery.OrderBy(data => data.Summary);
            }
	        return null;
	    }

        /// <summary>Constructs query for filtering.</summary>
        private IQueryable<Crash> ConstructQueryForFiltering(IUnitOfWork unitOfWork, FormHelper formData)
        {
            //Instead of returning a queryable and filtering it here we should construct a filtering expression and pass that to the repository.
            //I don't like that data handling is taking place in the controller.
            var results = unitOfWork.CrashRepository.ListAll();

            // Grab Results 
            var queryString = HttpUtility.HtmlDecode(formData.SearchQuery);
            if (!string.IsNullOrEmpty(queryString))
            {
                if (!string.IsNullOrEmpty(queryString))
                {
                    //We only use SearchQuery now for CallStack searching - if there's a SearchQuery value and a Username value, we need to get rid of the 
                    //Username so that we can create a broader search range
                    formData.UsernameQuery = "";
                }
                results = results.Where(data => data.RawCallStack.Contains(formData.SearchQuery));
            }

            if (formData.IsVanilla.HasValue)
            {
                results = results.Where(data => data.IsVanilla == formData.IsVanilla.Value);
            }

            // Filter by Crash Type
            if (formData.CrashType != "All")
            {
                switch (formData.CrashType)
                {
                    case "Crashes":
                        results = results.Where(CrashInstance => CrashInstance.CrashType == 1);
                        break;
                    case "Assert":
                        results = results.Where(CrashInstance => CrashInstance.CrashType == 2);
                        break;
                    case "Ensure":
                        results = results.Where(CrashInstance => CrashInstance.CrashType == 3);
                        break;
                    case "GPUCrash":
                        results = results.Where(CrashInstance => CrashInstance.CrashType == 4);
                        break;
                    case "CrashesAsserts":
                        results = results.Where(CrashInstance => CrashInstance.CrashType == 1 || CrashInstance.CrashType == 2 || CrashInstance.CrashType == 4);
                        break;
                }
            }

            // JRX Restore EpicID/UserName searching
            if (!string.IsNullOrEmpty(formData.UsernameQuery))
            {
                var decodedUsername = HttpUtility.HtmlDecode(formData.UsernameQuery).ToLower();
                var user = unitOfWork.UserRepository.First(data => data.UserName.Contains(decodedUsername));
                if (user != null)
                {
                    var userId = user.Id;
                    results = (
                        from CrashDetail in results
                        where CrashDetail.UserId == userId
                        select CrashDetail);
                }
            }

            // Start Filtering the results
            if (!string.IsNullOrEmpty(formData.EpicIdOrMachineQuery))
            {
                var decodedEpicOrMachineId = HttpUtility.HtmlDecode(formData.EpicIdOrMachineQuery).ToLower();
                results = (
                    from CrashDetail in results
                    where CrashDetail.EpicAccountId.Equals(decodedEpicOrMachineId) || 
                    CrashDetail.ComputerName.Equals(decodedEpicOrMachineId)
                    select CrashDetail );
            }

            if (!string.IsNullOrEmpty(formData.JiraQuery))
            {
                var decodedJira = HttpUtility.HtmlDecode(formData.JiraQuery).ToLower();
                results =
                    (
                        from CrashDetail in results
                        where CrashDetail.Jira.Contains(decodedJira)
                        select CrashDetail
                    );
            }

            // Filter by BranchName
            if (!string.IsNullOrEmpty(formData.BranchName))
            {
                results =
                    (
                        from CrashDetail in results
                        where CrashDetail.Branch.Equals(formData.BranchName)
                        select CrashDetail
                    );
            }

            // Filter by VersionName
            if (!string.IsNullOrEmpty(formData.VersionName))
            {
                results =
                    (
                        from CrashDetail in results
                        where CrashDetail.BuildVersion.Equals(formData.VersionName)
                        select CrashDetail
                    );
            }

            //Filter by Engine Version
            if (!string.IsNullOrEmpty(formData.EngineVersion))
            {
                results =
                    (
                        from CrashDetail in results
                        where CrashDetail.EngineVersion.Equals(formData.EngineVersion)
                        select CrashDetail
                    );
            }

            // Filter by VersionName
            if (!string.IsNullOrEmpty(formData.EngineMode))
            {
                results =
                    (
                        from CrashDetail in results
                        where CrashDetail.EngineMode.Equals(formData.EngineMode)
                        select CrashDetail
                    );
            }

            // Filter by PlatformName
            if (!string.IsNullOrEmpty(formData.PlatformName))
            {
                results =
                (
                    from CrashDetail in results
                    where CrashDetail.PlatformName.Contains(formData.PlatformName)
                    select CrashDetail
                );
            }

            // Filter by GameName
            if (!string.IsNullOrEmpty(formData.GameName))
            {
                var DecodedGameName = HttpUtility.HtmlDecode(formData.GameName).ToLower();

                if (DecodedGameName.StartsWith("-"))
                {
                    results =
                    (
                        from CrashDetail in results
                        where !CrashDetail.GameName.Contains(DecodedGameName.Substring(1))
                        select CrashDetail
                    );
                }
                else
                {
                    results =
                    (
                        from CrashDetail in results
                        where CrashDetail.GameName.Contains(DecodedGameName)
                        select CrashDetail
                    );
                }
            }

            // Filter by MessageQuery
            if (!string.IsNullOrEmpty(formData.MessageQuery))
            {
                results =
                (
                    from CrashDetail in results
                    where CrashDetail.Summary.Contains(formData.MessageQuery) || CrashDetail.Description.Contains(formData.MessageQuery)
                    select CrashDetail
                );
            }

            // Filter by BuiltFromCL
            if (!string.IsNullOrEmpty(formData.BuiltFromCL))
            {
                var builtFromCl = 0;
                var bValid = int.TryParse(formData.BuiltFromCL, out builtFromCl);

                if (bValid)
                {
                    results =
                        (
                            from CrashDetail in results
                            where CrashDetail.ChangelistVersion.Equals(formData.BuiltFromCL)
                            select CrashDetail
                        );
                }
            }

            return results.Include(data => data.User);
        }

        /// <summary>
        /// Filter a set of Crashes to a date range.
        /// </summary>
        /// <param name="Results">The unfiltered set of Crashes.</param>
        /// <param name="DateFrom">The earliest date to filter by.</param>
        /// <param name="DateTo">The latest date to filter by.</param>
        /// <returns>The set of Crashes between the earliest and latest date.</returns>
        public IQueryable<Crash> FilterByDate(IQueryable<Crash> Results, DateTime DateFrom, DateTime DateTo)
        {
            using (FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer(this.GetType().ToString() + " SQL"))
            {
                DateTo = DateTo.AddDays(1);

                IQueryable<Crash> CrashesInTimeFrame = Results
                    .Where(MyCrash => MyCrash.TimeOfCrash >= DateFrom && MyCrash.TimeOfCrash <= DateTo);
                return CrashesInTimeFrame;
            }
        }

	    /// <summary>
	    /// Create a new Crash data model object and insert it into the database
	    /// </summary>
	    /// <param name="description"></param>
	    /// <returns></returns>
	    private Crash CreateCrash(CrashDescription description)
	    {
	        var newCrash = new Crash
	        {
	            Branch = description.BranchName,
	            BaseDir = description.BaseDir,
	            BuildVersion = description.EngineVersion,
	            EngineVersion = description.BuildVersion,
	            ChangelistVersion = description.BuiltFromCL.ToString(),
	            CommandLine = description.CommandLine,
	            EngineMode = description.EngineMode,
	            ComputerName = description.MachineGuid,
	            IsVanilla = description.EngineModeEx.ToLower() == "vanilla"
	        };

	        //If there's a valid EpicAccountId assign that.
	        if (!string.IsNullOrEmpty(description.EpicAccountId))
	        {
	            newCrash.EpicAccountId = description.EpicAccountId;
	        }

	        newCrash.Description = "";
	        if (description.UserDescription != null)
	        {
	            newCrash.Description = string.Join(Environment.NewLine, description.UserDescription);
	        }
	        if (newCrash.Description.Length > 4095)
	            newCrash.Description = newCrash.Description.Substring(0, 4095);

	        newCrash.EngineMode = description.EngineMode;
	        newCrash.GameName = description.GameName;
	        newCrash.LanguageExt = description.Language; //Converted by the Crash process.
	        newCrash.PlatformName = description.Platform;
	        newCrash.HasLogFile = description.bHasLog;
	        newCrash.HasVideoFile = description.bHasVideo;
	        newCrash.HasMiniDumpFile = description.bHasMiniDump;

	        if (description.ErrorMessage != null)
	        {
	            newCrash.Summary = string.Join("\n", description.ErrorMessage);
	        }

	        if (description.CallStack != null)
	        {
	            newCrash.RawCallStack = string.Join("\n", description.CallStack);
	        }

	        if (description.SourceContext != null)
	        {
	            newCrash.SourceContext = string.Join("\n", description.SourceContext);
	        }

	        newCrash.TimeOfCrash = description.TimeofCrash;

	        newCrash.Jira = "";
	        newCrash.FixedChangeList = "";
	        newCrash.ProcessFailed = description.bProcessorFailed;

	        // Set the Crash type
	        newCrash.CrashType = 1;

	        //if we have a Crash type set the Crash type
	        if (!string.IsNullOrEmpty(description.CrashType))
	        {
	            switch (description.CrashType.ToLower())
	            {
	                case "crash":
	                    newCrash.CrashType = 1;
	                    break;
	                case "assert":
	                    newCrash.CrashType = 2;
	                    break;
	                case "ensure":
	                    newCrash.CrashType = 3;
                        break;
                    case "gpucrash":
                        newCrash.CrashType = 4;
                        break;
	                case "":
	                case null:
	                default:
	                    newCrash.CrashType = 1;
	                    break;
	            }
	        }
	        else //else fall back to the old behavior and try to determine type from RawCallStack
	        {
	            if (newCrash.RawCallStack != null)
	            {
	                if (newCrash.RawCallStack.Contains("FDebug::AssertFailed"))
	                {
	                    newCrash.CrashType = 2;
	                }
	                else if (newCrash.RawCallStack.Contains("FDebug::Ensure"))
	                {
	                    newCrash.CrashType = 3;
	                }
	                else if (newCrash.RawCallStack.Contains("FDebug::OptionallyLogFormattedEnsureMessageReturningFalse"))
	                {
	                    newCrash.CrashType = 3;
	                }
	                else if (newCrash.RawCallStack.Contains("NewReportEnsure"))
	                {
	                    newCrash.CrashType = 3;
	                }
	            }
	        }

	        // As we're adding it, the status is always new
	        newCrash.Status = "New";
	        newCrash.UserActivityHint = description.UserActivityHint;
	        newCrash.UserName = (!string.IsNullOrEmpty(description.UserName)) ? description.UserName : UserNameAnonymous;
            
            //Match this crash to an existing user or create a new user if one does not exist.
	       using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
	        {
	            //if there's a valid username assign the associated UserNameId else use "anonymous".
	            var userName = (!string.IsNullOrEmpty(description.UserName)) ? description.UserName : UserNameAnonymous;
                    
                if (unitOfWork.UserRepository.Any(data => data.UserName.Equals(userName)))
                {
                    newCrash.UserId = unitOfWork.UserRepository.ListAll()
                        .Where(data => data.UserName.Equals(userName))
                        .Select(data => data.Id)
                        .First();
                }
                else
                {
                    newCrash.User = new User() { UserName = description.UserName, UserGroupId = 5 };
                }
	        }

	        try
	        {
	            //Build the callstack pattern for this crash 
	            BuildPattern(newCrash);
	        }
	        catch (Exception ex)
	        {
	            FLogger.Global.WriteException("Error in Create Crash Build Pattern Method");
	            _slackWriter.Write("Error in Create Crash Build Pattern Method");
	            _slackWriter.Write(ex.Message);
	            if (ex.InnerException != null)
	            {
	                _slackWriter.Write(ex.InnerException.Message);
	            }
	            throw;
	        }

	        if(newCrash.CommandLine == null)
                newCrash.CommandLine = "";

            using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
	        {
	            var callStackRepository = unitOfWork.CallstackRepository;

	            try
	            {
	                var crashRepo = unitOfWork.CrashRepository;

	                //if we don't have any callstack data then insert the Crash and return
	                if (string.IsNullOrEmpty(newCrash.Pattern))
	                {
	                    crashRepo.Save(newCrash);
	                    unitOfWork.Save();
	                    return newCrash;
	                }

	                //If this isn't a new pattern then link it to our Crash data model
	                if (callStackRepository.Any(data => data.Pattern == newCrash.Pattern))
	                {
	                    var callstackPattern = callStackRepository.First(data => data.Pattern == newCrash.Pattern);
	                    newCrash.PatternId = callstackPattern.id;
	                }
	                else
	                {
	                    //if this is a new callstack pattern then insert into data model and create a new bugg.
	                    var callstackPattern = new CallStackPattern {Pattern = newCrash.Pattern};
	                    callStackRepository.Save(callstackPattern);
	                    unitOfWork.Save();
	                    newCrash.PatternId = callstackPattern.id;
	                }

	                //Mask out the line number and File path from our error message.
	                var errorMessageString = description.ErrorMessage != null
	                    ? String.Join("", description.ErrorMessage)
	                    : "";

	                //Create our masking regular expressions
	                var fileRegex = new Regex(@"(\[File:).*?(])"); //Match the filename out the file name
	                var lineRegex = new Regex(@"(\[Line:).*?(])"); //Match the line no.

	                /**
                        * * Regex to match ints of two characters or longer
                        * * First term ((?<=\s)|(-)) : Positive look behind, match if preceeded by whitespace or if first character is '-'
                        * Second term (\d{3,}) match three or more decimal chracters in a row.
                        * Third term (?=(\s|$)) positive look ahead match if followed by whitespace or end of line/file.
                        */
	                var intRegex = new Regex(@"-?\d{3,}");

	                /**
                        * Regular expression for masking out floats
                        */
	                var floatRegex = new Regex(@"-?\d+\.\d+");

	                /**
                        * Regular expression for masking out hexadecimal numbers
                        */
	                var hexRegex = new Regex(@"0x[\da-fA-F]+");

	                //mask out terms matches by our regex's
	                var trimmedError = fileRegex.Replace(errorMessageString, "");
	                trimmedError = lineRegex.Replace(trimmedError, "");
	                trimmedError = floatRegex.Replace(trimmedError, "");
	                trimmedError = hexRegex.Replace(trimmedError, "");
	                trimmedError = intRegex.Replace(trimmedError, "");
	                if (trimmedError.Length > 450)
	                    trimmedError = trimmedError.Substring(0, 450);
	                //error message is used as an index - trim to max index length

	                //Check to see if the masked error message is unique
	                ErrorMessage errorMessage = null;
	                if (
	                    unitOfWork.ErrorMessageRepository.Any(
	                        data =>
	                            data.ErrorMessageText.Equals(trimmedError, StringComparison.InvariantCultureIgnoreCase)))
	                {
	                    errorMessage =
	                        unitOfWork.ErrorMessageRepository.First(
	                            data =>
	                                data.ErrorMessageText.Equals(trimmedError,
	                                    StringComparison.InvariantCultureIgnoreCase));
	                }
	                else
	                {
	                    //if it's a new message then add it to the database.
	                    errorMessage = new ErrorMessage()
	                    {
	                        ErrorMessageText = trimmedError.ToLower(CultureInfo.InvariantCulture)
	                    };
	                    unitOfWork.ErrorMessageRepository.Save(errorMessage);
	                    unitOfWork.Save();
	                }

	                //Check for an existing bugg with this pattern and error message / no error message
	                if (unitOfWork.BuggRepository.Any(data =>
	                    (data.PatternId == newCrash.PatternId) &&
	                    (data.ErrorMessageId == errorMessage.Id || data.ErrorMessageId == null)))
	                {
	                    //if a bugg exists for this pattern update the bugg data
	                    var bugg = unitOfWork.BuggRepository.First(data => data.PatternId == newCrash.PatternId);
	                    bugg.PatternId = newCrash.PatternId;

	                    bugg.CrashType = newCrash.CrashType;
	                    bugg.ErrorMessageId = errorMessage.Id;

	                    //also update the bugg data while we're here
	                    bugg.TimeOfLastCrash = newCrash.TimeOfCrash;
	                    bugg.NumberOfCrashes = bugg.NumberOfCrashes + 1;

	                    if (String.Compare(newCrash.BuildVersion, bugg.BuildVersion,
	                        StringComparison.Ordinal) != 1)
	                        bugg.BuildVersion = newCrash.BuildVersion;

	                    unitOfWork.Save();

	                    //if a bugg exists update this Crash from the bugg
	                    //buggs are authoritative in this case
	                    newCrash.BuggId = bugg.Id;
	                    newCrash.Jira = bugg.TTPID;
	                    newCrash.FixedChangeList = bugg.FixedChangeList;
	                    newCrash.Status = bugg.Status;
	                    unitOfWork.CrashRepository.Save(newCrash);
	                    unitOfWork.Save();
	                }
	                else
	                {
	                    //if there's no bugg for this pattern create a new bugg and insert into the data store.
	                    var bugg = new Bugg();
	                    bugg.TimeOfFirstCrash = newCrash.TimeOfCrash;
	                    bugg.TimeOfLastCrash = newCrash.TimeOfCrash;
	                    bugg.TTPID = newCrash.Jira;
	                    bugg.Pattern = newCrash.Pattern;
	                    bugg.PatternId = newCrash.PatternId;
	                    bugg.NumberOfCrashes = 1;
	                    bugg.NumberOfUsers = 1;
	                    bugg.NumberOfUniqueMachines = 1;
	                    bugg.BuildVersion = newCrash.BuildVersion;
	                    bugg.CrashType = newCrash.CrashType;
	                    bugg.Status = newCrash.Status;
	                    bugg.FixedChangeList = newCrash.FixedChangeList;
	                    bugg.ErrorMessageId = errorMessage.Id;
	                    bugg.NumberOfCrashes = 1;
	                    newCrash.Bugg = bugg;
	                    unitOfWork.BuggRepository.Save(bugg);
	                    unitOfWork.CrashRepository.Save(newCrash);
	                    unitOfWork.Save();
	                }
	            }
	            catch (DbEntityValidationException dbentEx)
	            {
	                var messageBuilder = new StringBuilder();
	                messageBuilder.AppendLine("Db Entity Validation Exception Exception was:");
	                messageBuilder.AppendLine(dbentEx.ToString());

	                var innerEx = dbentEx.InnerException;
	                while (innerEx != null)
	                {
	                    messageBuilder.AppendLine("Inner Exception : " + innerEx.Message);
	                    innerEx = innerEx.InnerException;
	                }

	                if (dbentEx.EntityValidationErrors != null)
	                {
	                    messageBuilder.AppendLine("Validation Errors : ");
	                    foreach (var valErr in dbentEx.EntityValidationErrors)
	                    {
	                        messageBuilder.AppendLine(
	                            valErr.ValidationErrors.Select(data => data.ErrorMessage)
	                                .Aggregate((current, next) => current + "; /n" + next));
	                    }
	                }
	                FLogger.Global.WriteException(messageBuilder.ToString());
	                _slackWriter.Write("Create Crash Exception : " + messageBuilder.ToString());
	                throw;
	            }
	            catch (Exception ex)
	            {
	                var messageBuilder = new StringBuilder();
	                messageBuilder.AppendLine("Create Crash Exception : ");
	                messageBuilder.AppendLine(ex.Message.ToString());
	                var innerEx = ex.InnerException;
	                while (innerEx != null)
	                {
	                    messageBuilder.AppendLine("Inner Exception : " + innerEx.Message);
	                    innerEx = innerEx.InnerException;
	                }

	                FLogger.Global.WriteException("Create Crash Exception : " +
	                                                messageBuilder.ToString());
	                _slackWriter.Write("Create Crash Exception : " + messageBuilder.ToString());
	                throw;
	            }
            }

	        return newCrash;
	    }

	    /// <summary>
	    /// Create call stack pattern and either insert into database or match to existing.
	    /// Update Associate Buggs.
	    /// </summary>
	    /// <param name="newCrash"></param>
        private void BuildPattern(Crash newCrash)
	    {
	        
            var callstack = new CallStackContainer(newCrash.CrashType, newCrash.RawCallStack, newCrash.PlatformName);
	            newCrash.Module = callstack.GetModuleName();
	            newCrash.Module = newCrash.Module.Length >= 127 ? newCrash.Module.Substring(0, 127) : newCrash.Module;

	            if (newCrash.PatternId == null)
	            {
	                var patternList = new List<string>();

	                try
                    {
                        using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
                        {
                            foreach (var entry in callstack.CallStackEntries.Take(CallStackContainer.MaxLinesToParse))
                            {
                                var functionCallId = 0;

	                            //if this function is already in the database get id
	                            if (unitOfWork.FunctionRepository.Any(f => f.Call == entry.FunctionName))
	                            {
	                                functionCallId =
	                                    unitOfWork.FunctionRepository.FirstId(f => f.Call == entry.FunctionName);
	                            }
	                            else
	                            {
	                                //else add this function to the db.
	                                var call = entry.FunctionName;
	                                if (call.Length > 899)
	                                    call = call.Substring(0, 899);
	                                var currentFunctionCall = new FunctionCall {Call = call};
	                                unitOfWork.FunctionRepository.Save(currentFunctionCall);
	                                unitOfWork.Save();
	                                functionCallId = currentFunctionCall.Id;
	                            }

	                            patternList.Add(functionCallId.ToString());
	                        }
	                    }

	                    newCrash.Pattern = string.Join("+", patternList);
	                }
	                catch (Exception ex)
	                {
	                    var messageBuilder = new StringBuilder();
	                    FLogger.Global.WriteException("Build Pattern exception: " +
	                                                  ex.Message.ToString(CultureInfo.InvariantCulture));
	                    messageBuilder.AppendLine("Exception was:");
	                    messageBuilder.AppendLine(ex.ToString());
	                    while (ex.InnerException != null)
	                    {
	                        ex = ex.InnerException;
	                        messageBuilder.AppendLine(ex.ToString());
	                    }

	                    _slackWriter.Write("Build Pattern Exception : " + ex.Message.ToString(CultureInfo.InvariantCulture));
	                    throw;
	                }
	            }
	    }

        /// <summary></summary>
        /// <param name="disposing"></param>
	    protected override void Dispose(bool disposing)
	    {
            base.Dispose(disposing);
	    }

        /// <summary>
        /// 
        /// </summary>
	    public void TestAddCrash()
	    {
            using (
                var logTimer = new FAutoScopedLogTimer(this.GetType().ToString() + "()"))
            {
                string CrashContext;
                using (
                    var stream =
                        new FileStream(
                            "D:/CR/Engine/Source/Programs/CrashReporter/CrashReportWebSite/bin/Content/CrashContext.txt",
                            FileMode.Open))
                {
                    CrashContext = new StreamReader(stream).ReadToEnd();
                }

                var Crash = XmlHandler.FromXmlString<CrashDescription>(CrashContext);

                var crash = CreateCrash(Crash);

                _slackWriter.Write("Test Add Crash completed. Total Time : " + logTimer.GetElapsedSeconds().ToString("F2"));

            }
	    }
	}
}