// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Linq.Expressions;
using System.Net;
using System.Web;
using System.Web.Mvc;
using Microsoft.Practices.ObjectBuilder2;
using Newtonsoft.Json.Linq;
using Tools.CrashReporter.CrashReportWebSite.DataModels;
using Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories;
using Tools.CrashReporter.CrashReportWebSite.ViewModels;

namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{
	/// <summary>
	/// A controller to handle the Bugg data.
	/// </summary>
	public class BuggsController : Controller
	{
	    private int pageSize = 50;

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
			using (var logTimer = new FAutoScopedLogTimer( this.GetType().ToString()))
			{
				var formData = new FormHelper( Request, BuggsForm, "CrashInTimeFrameGroup" );
			    var results = GetResults( formData );
				results.GenerationTime = logTimer.GetElapsedSeconds().ToString( "F2" );
				return View( "Index", results );
			}
		}

		/// <summary>
		/// The Show action.
		/// </summary>
		/// <param name="buggsForm">The form of user data passed up from the client.</param>
		/// <param name="id">The unique id of the Bugg.</param>
		/// <returns>The view to display a Bugg on the client.</returns>
		public ActionResult Show( FormCollection buggsForm, int id, int page = 0 )
		{
			using( var logTimer = new FAutoScopedLogTimer( this.GetType().ToString() + "(BuggId=" + id + ")") )
			{
			    using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
			    {

			        // Set the display properties based on the radio buttons
			        var displayModuleNames = buggsForm["DisplayModuleNames"] == "true";
			        var displayFunctionNames = buggsForm["DisplayFunctionNames"] == "true";
			        var displayFileNames = buggsForm["DisplayFileNames"] == "true";
			        var displayFilePathNames = false;


			        // Set up the view model with the Crash data
                    if(buggsForm["Page"] != null)
                        int.TryParse(buggsForm["Page"], out page);

			        if (buggsForm["DisplayFilePathNames"] == "true")
			        {
			            displayFilePathNames = true;
			            displayFileNames = false;
			        }

			        var displayUnformattedCallStack = buggsForm["DisplayUnformattedCallStack"] == "true";

			        var model = GetResult(id, page, pageSize, unitOfWork);
			        model.SourceContext = model.CrashData.First().SourceContext;
			        model.Bugg.PrepareBuggForJira(model.CrashData);
			        // Handle 'CopyToJira' button
			        var buggIdToBeAddedToJira = 0;
			        foreach (var entry in buggsForm.Cast<object>().Where(entry => entry.ToString().Contains("CopyToJira-")))
			        {
			            int.TryParse(entry.ToString().Substring("CopyToJira-".Length), out buggIdToBeAddedToJira);
			            break;
			        }
			        if (buggIdToBeAddedToJira != 0)
			        {
			            model.Bugg.JiraProject = buggsForm["JiraProject"];
			            model.Bugg.CopyToJira();
			        }

			        var jc = JiraConnection.Get();
			        var bValidJira = false;

			        // Verify valid JiraID, this may be still a TTP
			        if (!string.IsNullOrEmpty(model.Bugg.TTPID))
			        {
			            var jira = 0;
			            int.TryParse(model.Bugg.TTPID, out jira);

			            if (jira == 0)
			            {
			                bValidJira = true;
			            }
			        }

			        if (jc.CanBeUsed() && bValidJira)
			        {
			            // Grab the data form JIRA.
			            var jiraSearchQuery = "key = " + model.Bugg.TTPID;
			            var jiraResults = new Dictionary<string, Dictionary<string, object>>();

			            try
			            {
			                jiraResults = jc.SearchJiraTickets(
			                    jiraSearchQuery,
			                    new string[]
			                    {
			                        "key", // string
			                        "summary", // string
			                        "components", // System.Collections.ArrayList, Dictionary<string,object>, name
			                        "resolution", // System.Collections.Generic.Dictionary`2[System.String,System.Object], name
			                        "fixVersions", // System.Collections.ArrayList, Dictionary<string,object>, name
			                        "customfield_11200" // string
			                    });
			            }
			            catch (System.Exception)
			            {
			                model.Bugg.JiraSummary = "JIRA MISMATCH";
			                model.Bugg.JiraComponentsText = "JIRA MISMATCH";
			                model.Bugg.JiraResolution = "JIRA MISMATCH";
			                model.Bugg.JiraFixVersionsText = "JIRA MISMATCH";
			                model.Bugg.JiraFixCL = "JIRA MISMATCH";
			            }

			            // Jira Key, Summary, Components, Resolution, Fix version, Fix changelist
			            if (jiraResults.Any())
			            {
			                var jira = jiraResults.First();
			                var summary = (string) jira.Value["summary"];
			                var componentsText = "";
			                var components = (System.Collections.ArrayList) jira.Value["components"];
			                foreach (Dictionary<string, object> component in components)
			                {
			                    componentsText += (string) component["name"];
			                    componentsText += " ";
			                }

			                var resolutionFields = (Dictionary<string, object>) jira.Value["resolution"];
			                var resolution = resolutionFields != null ? (string) resolutionFields["name"] : "";

			                var fixVersionsText = "";
			                var fixVersions = (System.Collections.ArrayList) jira.Value["fixVersions"];
			                foreach (Dictionary<string, object> fixVersion in fixVersions)
			                {
			                    fixVersionsText += (string) fixVersion["name"];
			                    fixVersionsText += " ";
			                }

			                var fixCl = jira.Value["customfield_11200"] != null
			                    ? (int) (decimal) jira.Value["customfield_11200"]
			                    : 0;

			                //Conversion to ado.net entity framework

			                model.Bugg.JiraSummary = summary;
			                model.Bugg.JiraComponentsText = componentsText;
			                model.Bugg.JiraResolution = resolution;
			                model.Bugg.JiraFixVersionsText = fixVersionsText;
			                if (fixCl != 0)
			                {
			                    model.Bugg.FixedChangeList = fixCl.ToString();
			                }
			            }
			        }

			        // Apply any user settings
			        if (buggsForm.Count > 0)
			        {
			            if (!string.IsNullOrEmpty(buggsForm["SetStatus"]))
			            {
			                model.Bugg.Status = buggsForm["SetStatus"];
                            unitOfWork.CrashRepository.SetStatusByBuggId(model.Bugg.Id, buggsForm["SetFixedIn"]);
			            }

			            if (!string.IsNullOrEmpty(buggsForm["SetFixedIn"]))
			            {
			                model.Bugg.FixedChangeList = buggsForm["SetFixedIn"];
                            unitOfWork.CrashRepository.SetFixedCLByBuggId(model.Bugg.Id, buggsForm["SetFixedIn"]);
			            }

			            if (!string.IsNullOrEmpty(buggsForm["SetTTP"]))
			            {
			                model.Bugg.TTPID = buggsForm["SetTTP"];
                            unitOfWork.CrashRepository.SetJiraByBuggId(model.Bugg.Id, buggsForm["SetTTP"]);
			            }
                        unitOfWork.BuggRepository.Update(model.Bugg);
			            
			            unitOfWork.Save();
			        }

			        var newCrash = model.CrashData.FirstOrDefault();
			        if (newCrash != null)
			        {
			            var callStack = new CallStackContainer(newCrash.CrashType, newCrash.RawCallStack, newCrash.PlatformName);

			            // Set callstack properties
			            callStack.bDisplayModuleNames = displayModuleNames;
			            callStack.bDisplayFunctionNames = displayFunctionNames;
			            callStack.bDisplayFileNames = displayFileNames;
			            callStack.bDisplayFilePathNames = displayFilePathNames;
			            callStack.bDisplayUnformattedCallStack = displayUnformattedCallStack;

			            model.CallStack = callStack;

			            // Shorten very long function names.
			            foreach (var entry in model.CallStack.CallStackEntries)
			            {
			                entry.FunctionName = entry.GetTrimmedFunctionName(128);
			            }

			            model.SourceContext = newCrash.SourceContext;

			            model.LatestCrashSummary = newCrash.Summary;
			        }

			        model.LatestCrashSummary = newCrash.Summary;
			        model.Bugg.LatestCrashSummary = newCrash.Summary;
			        model.GenerationTime = logTimer.GetElapsedSeconds().ToString("F2");

			        //Populate Jira Projects
			        var jiraConnection = JiraConnection.Get();
			        var response = jiraConnection.JiraRequest("/issue/createmeta", JiraConnection.JiraMethod.GET, null,
			            HttpStatusCode.OK);

			        using (var responseReader = new StreamReader(response.GetResponseStream()))
			        {
			            var responseText = responseReader.ReadToEnd();

			            JObject jsonObject = JObject.Parse(responseText);

			            JToken fields = jsonObject["projects"];

			            foreach (var project in fields)
			            {
			                model.JiraProjects.Add(new SelectListItem()
			                {
			                    Text = project["name"].ToObject<string>(),
			                    Value = project["key"].ToObject<string>()
			                });
			            }
			        }

			        model.PagingInfo = new PagingInfo
			        {
			            CurrentPage = page,
			            PageSize = pageSize,
			            TotalResults = model.Bugg.NumberOfCrashes.Value
			        };

			        return View("Show", model);
			    }
			}
		}
        
        #region Private Methods

        /// <summary>
        /// Retrieve all Buggs matching the search criteria.
        /// </summary>
        /// <param name="FormData">The incoming form of search criteria from the client.</param>
        /// <returns>A view to display the filtered Buggs.</returns>
        private BuggsViewModel GetResults(FormHelper FormData)
        {
            // Right now we take a Result IQueryable starting with ListAll() Buggs then we widdle away the result set by tacking on
            // Linq queries. Essentially it's Results.ListAll().Where().Where().Where().Where().Where().Where()
            // Could possibly create more optimized queries when we know exactly what we're querying
            // The downside is that if we add more parameters each query may need to be updated.... Or we just add new case statements
            // The other downside is that there's less code reuse, but that may be worth it.
            var fromDate = FormData.DateFrom;
            var toDate = FormData.DateTo.AddDays(1);

            IQueryable<Crash> results = null;
            IQueryable<Bugg> buggResults = null;
            var skip = (FormData.Page - 1) * FormData.PageSize;
            var take = FormData.PageSize;

            var sortedResultsList = new System.Collections.Generic.List<Bugg>();
            var groupCounts = new System.Collections.Generic.SortedDictionary<string, int>();
            int totalCountedRecords = 0;

            using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
            {
                var userGroupId = unitOfWork.UserGroupRepository.First(data => data.Name == FormData.UserGroup).Id;

                // Get every Bugg.
                var resultsAll = unitOfWork.CrashRepository.ListAll();

                // Look at all Buggs that are still 'open' i.e. the last Crash occurred in our date range.
                results = FilterByDate(resultsAll, FormData.DateFrom, FormData.DateTo);

                // Filter results by build version.
                if (!string.IsNullOrEmpty(FormData.VersionName))
                {
                    results = results.Where(data => data.BuildVersion == FormData.VersionName);
                }

                // Filter by BranchName
                if (!string.IsNullOrEmpty(FormData.BranchName))
                {
                    results =
                        results.Where(da => da.Branch.Equals(FormData.BranchName));
                }

                // Filter by PlatformName
                if (!string.IsNullOrEmpty(FormData.PlatformName))
                {
                    results =
                        results.Where(da => da.PlatformName.Equals(FormData.PlatformName));
                }

                // Run at the end
                if (!string.IsNullOrEmpty(FormData.SearchQuery))
                {
                    try
                    {
                        var queryString = HttpUtility.HtmlDecode(FormData.SearchQuery.ToString()) ?? "";

                        // Take out terms starting with a -
                        var terms = queryString.Split("-, ;+".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);
                        var allFuncionCallIds = new HashSet<int>();
                        foreach (var term in terms)
                        {
                            var functionCallIds = unitOfWork.FunctionRepository.Get(functionCallInstance => functionCallInstance.Call.Contains(term)).Select(x => x.Id).ToList();
                            foreach (var id in functionCallIds)
                            {
                                allFuncionCallIds.Add(id);
                            }
                        }

                        // Search for all function ids. OR operation, not very efficient, but for searching for one function should be ok.
                        results = allFuncionCallIds.Aggregate(results, (current, id) => current.Where(x => x.Pattern.Contains(id + "+") || x.Pattern.Contains("+" + id)));
                    }
                    catch (Exception ex)
                    {
                        Debug.WriteLine("Exception in Search: " + ex.ToString());
                    }
                }

                // Filter by Crash Type
                if (FormData.CrashType != "All")
                {
                    switch (FormData.CrashType)
                    {
                        case "Crash":
                            results = results.Where(buggInstance => buggInstance.CrashType == 1);
                            break;
                        case "Assert":
                            results = results.Where(buggInstance => buggInstance.CrashType == 2);
                            break;
                        case "Ensure":
                            results = results.Where(buggInstance => buggInstance.CrashType == 3);
                            break;
                        case "GPUCrash":
                            results = results.Where(buggInstance => buggInstance.CrashType == 4);
                            break;
                        case "CrashAsserts":
                            results =
                                results.Where(buggInstance => buggInstance.CrashType == 1 || buggInstance.CrashType == 2 || buggInstance.CrashType == 4);
                            break;
                    }
                }

                // Filter by user group if present
                if (!string.IsNullOrEmpty(FormData.UserGroup))
                    results = FilterByUserGroup(results, userGroupId);

                if (!string.IsNullOrEmpty(FormData.JiraId))
                {
                    results = results.Where(data => data.Jira == FormData.JiraId);
                }

                var CrashByUserGroupCounts = unitOfWork.CrashRepository.ListAll().Where(data =>
                    data.User.UserGroupId == userGroupId &&
                    data.TimeOfCrash >= fromDate &&
                    data.TimeOfCrash <= toDate &&
                    data.BuggId != null)
                    .GroupBy(data => data.BuggId)
                    .Select(data => new {data.Key, Count = data.Count()})
                    .OrderByDescending(data => data.Count)
                    .ToDictionary(data => data.Key, data => data.Count);

                var CrashInTimeFrameCounts = unitOfWork.CrashRepository.ListAll().Where(data =>
                    data.TimeOfCrash >= fromDate &&
                    data.TimeOfCrash <= toDate
                    && data.BuggId != null)
                    .GroupBy(data => data.BuggId)
                    .Select(data => new {data.Key, Count = data.Count()})
                    .OrderByDescending(data => data.Count)
                    .ToDictionary(data => data.Key, data => data.Count);

                var affectedUsers = unitOfWork.CrashRepository.ListAll().Where(data =>
                    data.User.UserGroupId == userGroupId &&
                    data.TimeOfCrash >= fromDate &&
                    data.TimeOfCrash <= toDate && data.BuggId != null)
                    .Select(data => new { BuggId = data.BuggId, ComputerName = data.ComputerName })
                    .Distinct()
                    .GroupBy(data => data.BuggId)
                    .Select(data => new {data.Key, Count = data.Count()})
                    .OrderByDescending(data => data.Count)
                    .ToDictionary(data => data.Key, data => data.Count);

                var ids = results.Where(data => data.BuggId != null).Select(data => data.BuggId).Distinct().ToList();

                buggResults = unitOfWork.BuggRepository.ListAll().Where(data => ids.Contains(data.Id));

                //buggResults = results.Where(data => data.BuggId != null).Select(data => data.Bugg).Distinct();
                
                // Grab just the results we want to display on this page
                totalCountedRecords = buggResults.Count();

                buggResults = GetSortedResults(buggResults, FormData.SortTerm, FormData.SortTerm == "descending",
                    FormData.DateFrom, FormData.DateTo, FormData.UserGroup);

               

                sortedResultsList =
                    buggResults.ToList();

                // Get UserGroup ResultCounts
                var groups =
                    buggResults.SelectMany(data => data.UserGroups)
                        .GroupBy(data => data.Name)
                        .Select(data => new {Key = data.Key, Count = data.Count()})
                        .ToDictionary(x => x.Key, y => y.Count);
                
                groupCounts = new SortedDictionary<string, int>(groups);

                foreach (var bugg in sortedResultsList)
                {
                    if (CrashByUserGroupCounts.ContainsKey(bugg.Id))
                    {
                        bugg.CrashesInTimeFrameGroup = CrashByUserGroupCounts[bugg.Id];
                    }
                    else
                        bugg.CrashesInTimeFrameGroup = 0;

                    if (CrashInTimeFrameCounts.ContainsKey(bugg.Id))
                    {
                        bugg.CrashesInTimeFrameAll = CrashInTimeFrameCounts[bugg.Id];
                    }
                    else
                        bugg.CrashesInTimeFrameAll = 0;

                    if (affectedUsers.ContainsKey(bugg.Id))
                    {
                        bugg.NumberOfUniqueMachines = affectedUsers[bugg.Id];
                    }
                    else
                        bugg.NumberOfUniqueMachines = 0;
                }

                sortedResultsList = sortedResultsList.OrderByDescending(data => data.CrashesInTimeFrameGroup).Skip(skip)
                    .Take(totalCountedRecords >= skip + take ? take : totalCountedRecords).ToList();

                foreach (var bugg in sortedResultsList)
                {
                    var Crash =
                        unitOfWork.CrashRepository.First(data => data.BuggId == bugg.Id);
                    bugg.FunctionCalls = Crash.GetCallStack().GetFunctionCalls();
                }
            }

            System.Collections.Generic.List<SelectListItem> branchNames;
            System.Collections.Generic.List<SelectListItem> versionNames;
            System.Collections.Generic.List<SelectListItem> platformNames;
            using (var unitOfWork = new UnitOfWork(new CrashReportEntities()))
            {
                branchNames = unitOfWork.CrashRepository.GetBranchesAsListItems();
                versionNames = unitOfWork.CrashRepository.GetVersionsAsListItems();
                platformNames = unitOfWork.CrashRepository.GetPlatformsAsListItems();
            }

            return new BuggsViewModel()
            {
                Results = sortedResultsList,
                PagingInfo = new PagingInfo { CurrentPage = FormData.Page, PageSize = FormData.PageSize, TotalResults = totalCountedRecords },
                SortTerm = FormData.SortTerm,
                SortOrder = FormData.SortOrder,
                UserGroup = FormData.UserGroup,
                CrashType = FormData.CrashType,
                SearchQuery = FormData.SearchQuery,
                BranchNames = branchNames,
                VersionNames = versionNames,
                PlatformNames = platformNames,
                DateFrom = (long)(FormData.DateFrom - CrashesViewModel.Epoch).TotalMilliseconds,
                DateTo = (long)(FormData.DateTo - CrashesViewModel.Epoch).TotalMilliseconds,
                VersionName = FormData.VersionName,
                PlatformName = FormData.PlatformName,
                BranchName = FormData.BranchName,
                GroupCounts = groupCounts,
                Jira = FormData.JiraId,
            };
        }

        /// <summary>
        /// Retrieve all Data for a single bug given by the id
        /// </summary>
        /// <param name="buggsForm"></param>
        /// <param name="id"></param>
        /// <returns></returns>
        private BuggViewModel GetResult(int id, int page, int pageSize, UnitOfWork unitOfWork)
	    {
            var model = new BuggViewModel();
            model.Bugg = unitOfWork.BuggRepository.GetById(id);

            model.Bugg.CrashesInTimeFrameAll = 0;
            model.Bugg.CrashesInTimeFrameGroup = 0;
            model.Bugg.NumberOfCrashes = unitOfWork.CrashRepository.Count(data => data.BuggId == id);
            model.Bugg.NumberOfUniqueMachines =
                unitOfWork.CrashRepository.ListAll()
                    .Where(data => data.BuggId == id)
                    .GroupBy(data => data.ComputerName)
                    .Count();
            model.CrashData = unitOfWork.CrashRepository.ListAll().Where(data => data.BuggId == id)
                .OrderByDescending(data => data.TimeOfCrash).Skip(page * pageSize).Take(pageSize).Select(data => new CrashDataModel()
            {
                Id = data.Id,
                ChangelistVersion = data.ChangelistVersion,
                GameName = data.GameName,
                EngineMode = data.EngineMode,
                PlatformName = data.PlatformName,
                TimeOfCrash = data.TimeOfCrash,
                Description = data.Description,
                RawCallStack = data.RawCallStack,
                CrashType = data.CrashType,
                Summary = data.Summary,
                SourceContext = data.SourceContext,
                BuildVersion = data.BuildVersion,
                ComputerName = data.ComputerName,
                Branch = data.Branch
            }).ToList();

            return model;
	    }

        /// <summary>
        /// Filter a set of Buggs by a jira ticket.
        /// </summary>
        /// <param name="results">The unfiltered set of Buggs</param>
        /// <param name="jira">The ticket by which to filter the list of buggs</param>
        /// <returns></returns>
        private IQueryable<Bugg> FilterByJira(IQueryable<Bugg> results, string jira)
        {
            return results.Where(bugg => bugg.TTPID == jira);
        }

        /// <summary>
        /// Filter a set of Buggs to a date range.
        /// </summary>
        /// <param name="results">The unfiltered set of Buggs.</param>
        /// <param name="dateFrom">The earliest date to filter by.</param>
        /// <param name="dateTo">The latest date to filter by.</param>
        /// <returns>The set of Buggs between the earliest and latest date.</returns>
        private IQueryable<Crash> FilterByDate(IQueryable<Crash> results, DateTime dateFrom, DateTime dateTo)
        {
            dateTo = dateTo.AddDays(1);
            //var buggsInTimeFrame =
            //    results.Where(bugg => (bugg.TimeOfFirstCrash >= dateFrom && bugg.TimeOfFirstCrash <= dateTo) ||
            //        (bugg.TimeOfLastCrash >= dateFrom && bugg.TimeOfLastCrash <= dateTo) || (bugg.TimeOfFirstCrash <= dateFrom && bugg.TimeOfLastCrash >= dateTo));
            var buggsInTimeFrame = results.Where(MyCrash => MyCrash.TimeOfCrash >= dateFrom && MyCrash.TimeOfCrash <= dateTo);

            return buggsInTimeFrame;
        }

        /// <summary>
        /// Filter a set of Buggs by build version.
        /// </summary>
        /// <param name="results">The unfiltered set of Buggs.</param>
        /// <param name="buildVersion">The build version to filter by.</param>
        /// <returns>The set of Buggs that matches specified build version</returns>
        private IQueryable<Bugg> FilterByBuildVersion(IQueryable<Bugg> results, string buildVersion)
        {
            if (!string.IsNullOrEmpty(buildVersion))
            {
                results = results.Where(data => data.BuildVersion.Contains(buildVersion));
            }
            return results;
        }

        /// <summary>
        /// Filter a set of Buggs by user group name.
        /// </summary>
        /// <param name="setOfBuggs">The unfiltered set of Buggs.</param>
        /// <param name="groupName">The user group name to filter by.</param>
        /// <returns>The set of Buggs by users in the requested user group.</returns>
        private IQueryable<Crash> FilterByUserGroup(IQueryable<Crash> setOfBuggs, int groupName)
        {
            return setOfBuggs.Where(data => data.User.UserGroup.Id == groupName);
        }

        /// <summary>
        /// Sort the container of Buggs by the requested criteria.
        /// </summary>
        /// <param name="results">A container of unsorted Buggs.</param>
        /// <param name="sortTerm">The term to sort by.</param>
        /// <param name="bSortDescending">Whether to sort by descending or ascending.</param>
        /// <param name="dateFrom">The date of the earliest Bugg to examine.</param>
        /// <param name="dateTo">The date of the most recent Bugg to examine.</param>
        /// <param name="groupName">The user group name to filter by.</param>
        /// <returns>A sorted container of Buggs.</returns>
        private IOrderedQueryable<Bugg> GetSortedResults(IQueryable<Bugg> results, string sortTerm, bool bSortDescending, DateTime dateFrom, DateTime dateTo, string groupName)
        {
            IOrderedQueryable<Bugg> orderedResults = null;

            try
            {
                switch (sortTerm)
                {
                    case "Id":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.Id, bSortDescending);
                        break;
                    case "BuildVersion":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.BuildVersion, bSortDescending);
                        break;
                    case "LatestCrash":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.TimeOfLastCrash, bSortDescending);
                        break;
                    case "FirstCrash":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.TimeOfFirstCrash, bSortDescending);
                        break;
                    case "NumberOfCrash":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.NumberOfCrashes, bSortDescending);
                        break;
                    case "NumberOfUsers":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.NumberOfUniqueMachines, bSortDescending);
                        break;
                    case "Pattern":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.Pattern, bSortDescending);
                        break;
                    case "CrashType":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.CrashType, bSortDescending);
                        break;
                    case "Status":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.Status, bSortDescending);
                        break;
                    case "FixedChangeList":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.FixedChangeList, bSortDescending);
                        break;
                    case "TTPID":
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.TTPID, bSortDescending);
                        break;
                    default:
                        orderedResults = EnumerableOrderBy(results, buggCrashInstance => buggCrashInstance.Id, bSortDescending);
                        break;

                }
                return orderedResults;
            }
            catch (Exception ex)
            {
                Debug.WriteLine("Exception in GetSortedResults: " + ex.ToString());
            }
            return orderedResults;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <typeparam name="TKey"></typeparam>
        /// <param name="query"></param>
        /// <param name="predicate"></param>
        /// <param name="bDescending"></param>
        /// <returns></returns>
        private IOrderedQueryable<Bugg> EnumerableOrderBy<TKey>(IQueryable<Bugg> query, Expression<Func<Bugg, TKey>> predicate, bool bDescending)
        {
            return bDescending ? query.OrderByDescending(predicate) : query.OrderBy(predicate);
        }
        
        /// <summary>Dispose the controller - safely getting rid of database connections</summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
        }

        #endregion
    }
}
