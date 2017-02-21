// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.Linq;
using System.Web.Mvc;
using Tools.CrashReporter.CrashReportWebSite.DataModels;
using Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories;
using Tools.CrashReporter.CrashReportWebSite.ViewModels;

namespace Tools.CrashReporter.CrashReportWebSite.Controllers
{
	/// <summary>Controls the start page.</summary>
	public class HomeController : Controller
	{
        /// <summary>
        /// 
        /// </summary>
	    public HomeController(IUnitOfWork unitOfWork)
	    {
	        
	    }

		/// <summary>
		/// The main view of the home page.
		/// </summary>
		public ActionResult Index()
		{
            using (var logTimer = new FAutoScopedLogTimer( this.GetType().ToString(), bCreateNewLog: true ))
            {
                var result = new CrashesViewModel();
                using (var entitile = new CrashReportEntities())
                {
                    var unitOfWork = new UnitOfWork(entitile);
                
                    result.BranchNames = unitOfWork.CrashRepository.GetBranchesAsListItems();
                    result.BranchNames.ForEach(data => data.Selected = false);
                    result.VersionNames = unitOfWork.CrashRepository.GetVersionsAsListItems();
                    result.VersionNames.ForEach(data => data.Selected = false);
                    result.PlatformNames = unitOfWork.CrashRepository.GetPlatformsAsListItems();
                    result.PlatformNames.ForEach(data => data.Selected = false);
                    result.EngineModes = unitOfWork.CrashRepository.GetEngineModesAsListItems();
                    result.EngineModes.ForEach(data => data.Selected = false);
                    result.EngineVersions = unitOfWork.CrashRepository.GetEngineVersionsAsListItems();
                    result.EngineVersions.ForEach(data => data.Selected = false);
                    result.IsVanilla = null;
                    result.GenerationTime = logTimer.GetElapsedSeconds().ToString("F2");
                }

			    return View( "Index", result );
			}
		}

        protected override void Dispose(bool disposing)
        {
            base.Dispose(disposing);
        }
	}
}