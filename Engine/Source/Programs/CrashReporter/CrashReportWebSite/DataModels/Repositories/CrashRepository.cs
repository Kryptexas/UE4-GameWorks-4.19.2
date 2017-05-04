using System;
using System.Collections.Generic;
using System.Data.Entity;
using System.Linq;
using System.Linq.Expressions;
using System.Runtime.Caching;
using System.Web.Mvc;
using Tools.CrashReporter.CrashReportWebSite.Properties;
using System.Runtime.Caching;

namespace Tools.CrashReporter.CrashReportWebSite.DataModels.Repositories
{
    /// <summary>
    /// 
    /// </summary>
    public class CrashRepository : ICrashRepository
    {
        private CrashReportEntities _entityContext;
        
        private static MemoryCache cache = MemoryCache.Default;

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="entityContext">A reference to the *single* instance of the data context.</param>
        public CrashRepository(CrashReportEntities entityContext)
        {
            _entityContext = entityContext;
        }

        /// <summary>
        /// Return a queryable string for query construction.
        /// 
        /// NOTE - This is bad. Replace this method with proper expression tree construction and strictly return
        /// enumerated lists. All data handling should happen within the repository. 
        /// </summary>
        /// <returns></returns>
        public IQueryable<Crash> ListAll()
        {
            return _entityContext.Crashes;
        }

        /// <summary>
        /// Count the number of objects that satisfy the filter
        /// </summary>
        /// <param name="filter"></param>
        /// <returns></returns>
        public int Count(Expression<Func<Crash, bool>> filter)
        {
            return _entityContext.Crashes.Count(filter);
        }

        /// <summary>
        /// Get a filtered list of Crashes from data storage
        /// Calling this method returns the data directly. It will execute the data retrieval - in this case an sql transaction.
        /// </summary>
        /// <param name="filter">A linq expression used to filter the Crash table</param>
        /// <returns>Returns a fully filtered enumerated list object of Crashes.</returns>
        public IEnumerable<Crash> Get(Expression<Func<Crash, bool>> filter)
        {
            return _entityContext.Crashes.Where(filter).ToList();
        }

        /// <summary>
        /// Get a filtered, ordered list of Crash
        /// </summary>
        /// <param name="filter">An expression used to filter the Crashes</param>
        /// <param name="orderBy">An function delegate userd to order the results from the Crash table</param>
        /// <returns></returns>
        public IEnumerable<Crash> Get(Expression<Func<Crash, bool>> filter,
            Func<IQueryable<Crash>, IOrderedQueryable<Crash>> orderBy)
        {
            return orderBy(_entityContext.Crashes.Where(filter)).ToList();
        }

        /// <summary>
        /// Return an ordered list of Crashes with data preloading
        /// </summary>
        /// <param name="filter">A linq expression used to filter the Crash table</param>
        /// <param name="orderBy">A linq expression used to order the results from the Crash table</param>
        /// <param name="includeProperties">A linq expression indicating properties to dynamically load.</param>
        /// <returns></returns>
        public IEnumerable<Crash> Get(Expression<Func<Crash, bool>> filter,
            Func<IQueryable<Crash>, IOrderedQueryable<Crash>> orderBy,
            params Expression<Func<Crash, object>>[] includeProperties)
        {
            var query = _entityContext.Crashes.Where(filter);
            foreach (var includeProperty in includeProperties)
            {
                query.Include(includeProperty);
            }

            return orderBy == null ? query.ToList() : orderBy(query).ToList();
        }

        /// <summary>
        /// Get a Crash from it's id
        /// </summary>
        /// <param name="id">The id of the Crash to retrieve</param>
        /// <returns>Crash data model</returns>
        public Crash GetById(int id)
        {
            var result = _entityContext.Crashes.First(data => data.Id == id);
            result.ReadCrashContextIfAvailable();
            return result;
        }

        /// <summary>
        /// Check if there are any Crashes matching a specific filter.
        /// </summary>
        /// <param name="filter"></param>
        /// <returns></returns>
        public bool Any(Expression<Func<Crash, bool>> filter)
        {
            return _entityContext.Crashes.Any(filter);
        }

        /// <summary>
        /// Get the first Crash matching a specific filter.
        /// </summary>
        /// <param name="filter"></param>
        /// <returns></returns>
        public Crash First(Expression<Func<Crash, bool>> filter)
        {
            return _entityContext.Crashes.FirstOrDefault(filter);
        }

        /// <summary>
        /// Add a new Crash to the data store
        /// </summary>
        /// <param name="entity"></param>
        public void Save(Crash entity)
        {
            _entityContext.Crashes.Add(entity);
        }

        /// <summary>
        /// Remove a Crash from the data store
        /// </summary>
        /// <param name="entity"></param>
        public void Delete(Crash entity)
        {
            _entityContext.Crashes.Remove(entity);
        }

        /// <summary>
        /// Update an existing Crash
        /// </summary>
        /// <param name="entity"></param>
        public void Update(Crash entity)
        {
            var set = _entityContext.Set<Crash>();
            var entry = set.Local.SingleOrDefault(f => f.Id == entity.Id);

            if (entry != null)
            {
                var attachedFeature = _entityContext.Entry(entry);
                attachedFeature.CurrentValues.SetValues(entity);
                attachedFeature.State = EntityState.Modified;
            }
            else
            {
                _entityContext.Crashes.Attach(entity);
                _entityContext.Entry(entity).State = EntityState.Modified;
            }
        }

        private static DateTime LastBranchesDate = DateTime.UtcNow.AddDays(-1);
        private static List<SelectListItem> BranchesAsSelectList = null;
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public List<SelectListItem> GetBranchesAsListItems()
        {
            var branchesAsSelectList = cache["branches"] as List<SelectListItem>;

            if (branchesAsSelectList == null)
            {
                using (FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer("CrashRepository.GetBranches"))
                {
                    var date = DateTime.Now.AddDays(-14);

                    var BranchList = _entityContext.Crashes
                        .Where(n => n.TimeOfCrash > date)
                        .Where(c => c.CrashType != 3) // Ignore ensures
                        // Depot - //depot/UE4* || Stream //UE4, //Something etc.
                        .Where(n => n.Branch.StartsWith("UE4") || n.Branch.StartsWith("//"))
                        .Select(n => n.Branch)
                        .Distinct()
                        .ToList();

                    branchesAsSelectList =
                        BranchList
                        .Select(listItem => new SelectListItem { Selected = false, Text = listItem, Value = listItem })
                        .OrderBy(listItem => listItem.Text)
                        .ToList();

                    branchesAsSelectList.Insert(0, new SelectListItem { Selected = true, Text = "", Value = "" });
                }
                
                var searchFilterCachePolicy = new CacheItemPolicy() { AbsoluteExpiration = DateTimeOffset.Now.AddMinutes(Settings.Default.SearchFilterCacheInMinutes) };
                cache.Set("branches", branchesAsSelectList, searchFilterCachePolicy);
            }

            return branchesAsSelectList;
        }

        /// <summary>
        /// Static list of platforms for filtering
        /// </summary>
        public List<SelectListItem> GetPlatformsAsListItems()
        {
            var PlatformsAsListItems = new List<SelectListItem>();
            
            string[] PlatformNames = { "Win64", "Win32", "Mac", "Linux", "PS4", "XboxOne" };

            PlatformsAsListItems = PlatformNames
                .Select(listitem => new SelectListItem { Selected = false, Text = listitem, Value = listitem })
                .ToList();
                PlatformsAsListItems.Insert(0, new SelectListItem { Selected = true, Text = "", Value = "" });

            return PlatformsAsListItems;
        }


        /// <summary>
        /// Static list of Engine Modes for filtering
        /// </summary>
        public List<SelectListItem> GetEngineModesAsListItems()
        {
            string[] engineModes = { "Commandlet", "Editor", "Game", "Server" };

            List<SelectListItem> engineModesAsListItems = engineModes
                .Select(listitem => new SelectListItem { Selected = false, Text = listitem, Value = listitem })
                .ToList();
            engineModesAsListItems.Insert(0, new SelectListItem { Selected = true, Text = "", Value = "" });

            return engineModesAsListItems;
        }

        private static HashSet<string> DistinctBuildVersions = null;
        /// <summary>
        /// Retrieves a list of distinct UE4 Versions from the CrashRepository
        /// </summary>
        public List<SelectListItem> GetVersionsAsListItems()
        {
            List<SelectListItem> versionsAsSelectList = cache["versions"] as List<SelectListItem>;

            if (versionsAsSelectList == null)
            {
                var date = DateTime.Now.AddDays(-14);
                var BuildVersions = _entityContext.Crashes
                .Where(c => c.TimeOfCrash > date)
                .Where(c => c.CrashType != 3) // Ignore ensures
                .Select(c => c.BuildVersion)
                .Distinct()
                .ToList();

                DistinctBuildVersions = new HashSet<string>();

                foreach (var BuildVersion in BuildVersions)
                {
                    var BVParts = BuildVersion.Split(new char[] { '.' }, StringSplitOptions.RemoveEmptyEntries);
                    if (BVParts.Length > 2 && BVParts[0] != "0")
                    {
                        string CleanBV = string.Format("{0}.{1}.{2}", BVParts[0], BVParts[1], BVParts[2]);
                        DistinctBuildVersions.Add(CleanBV);
                    }
                }

                versionsAsSelectList = DistinctBuildVersions
                    .Select(listitem => new SelectListItem { Selected = false, Text = listitem, Value = listitem })
                    .ToList();
                versionsAsSelectList.Insert(0, new SelectListItem { Selected = true, Text = "", Value = "" });

                var searchFilterCachePolicy = new CacheItemPolicy() { AbsoluteExpiration = DateTimeOffset.Now.AddMinutes(Settings.Default.SearchFilterCacheInMinutes) };
                cache.Set("versions", versionsAsSelectList, searchFilterCachePolicy);
            }

            return versionsAsSelectList;
        }

        private static DateTime LastEngineVersionDate = DateTime.UtcNow.AddDays(-1);
        private static HashSet<string> DistinctEngineVersions = null;
        public List<SelectListItem> GetEngineVersionsAsListItems()
        {
            List<SelectListItem> engineVersionsAsSelectList = cache["engineVersions"] as List<SelectListItem>;

            if (engineVersionsAsSelectList == null)
            {
                var date = DateTime.Now.AddDays(-14);
                var engineVersions = _entityContext.Crashes
                .Where(c => c.TimeOfCrash > date)
                .Where(c => c.CrashType != 3) // Ignore ensures
                .Select(c => c.EngineVersion)
                .Distinct()
                .ToList();

                var DistinctEngineVersions = new HashSet<string>();

                foreach (var engineVersion in engineVersions)
                {
                    if (string.IsNullOrWhiteSpace(engineVersion))
                        continue;

                    var BVParts = engineVersion.Split(new char[] { '.' }, StringSplitOptions.RemoveEmptyEntries);
                    if (BVParts.Length > 2 && BVParts[0] != "0")
                    {
                        string CleanBV = string.Format("{0}.{1}.{2}", BVParts[0], BVParts[1], BVParts[2]);
                        DistinctEngineVersions.Add(CleanBV);
                    }
                }

                engineVersionsAsSelectList = DistinctEngineVersions
                    .Select(listitem => new SelectListItem { Selected = false, Text = listitem, Value = listitem })
                    .ToList();
                engineVersionsAsSelectList.Insert(0, new SelectListItem { Selected = true, Text = "", Value = "" });

                var searchFilterCachePolicy = new CacheItemPolicy() { AbsoluteExpiration = DateTimeOffset.Now.AddMinutes(Settings.Default.SearchFilterCacheInMinutes) };
                cache.Set("engineVersions", engineVersionsAsSelectList, searchFilterCachePolicy);
            }

            return engineVersionsAsSelectList;
        } 


        /// <summary>
        /// Retrieves a list of distinct UE4 Versions from the CrashRepository
        /// </summary>
        public HashSet<string> GetVersions()
        {
            GetVersionsAsListItems();
            return DistinctBuildVersions;
        }

        public void SetStatusByBuggId(int buggId, string status)
        {
            _entityContext.Crashes.SqlQuery("UPDATE CRASHES SET Status = '" + status + "' * WHERE Crashes.BuggId = '" + buggId.ToString("N") + "'");
        }

        public void SetFixedCLByBuggId(int buggId, string fixedCl)
        {
            _entityContext.Crashes.SqlQuery("UPDATE CRASHES SET FixedChangeList = '" + fixedCl + "' * WHERE Crashes.BuggId = '" + buggId.ToString("N") + "'");
        }

        public void SetJiraByBuggId(int buggId, string jira)
        {
            _entityContext.Crashes.SqlQuery("UPDATE CRASHES SET Jira = '" + jira + "' * WHERE Crashes.BuggId = '" + buggId.ToString("N") + "'");
        }
    }
}
