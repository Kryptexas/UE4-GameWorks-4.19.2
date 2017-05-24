// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.Collections.Generic;
using System.Web.Mvc;

namespace Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories
{
    /// <summary>
    /// 
    /// </summary>
    public interface ICrashRepository : IDataRepository<Crash>
    {
        List<SelectListItem> GetBranchesAsListItems();
        List<SelectListItem> GetPlatformsAsListItems();
        List<SelectListItem> GetEngineModesAsListItems();
        List<SelectListItem> GetVersionsAsListItems();
        List<SelectListItem> GetEngineVersionsAsListItems();
        void SetStatusByBuggId(int buggId, string status);
        void SetFixedCLByBuggId(int buggId, string fixedCl);
        void SetJiraByBuggId(int buggId, string jira);
    }
}
