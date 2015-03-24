using System;
using System.Collections.Generic;
using System.Data;
using System.Data.Common;
using System.Data.Linq.SqlClient;
using System.Data.Linq;
using System.Data.SqlClient;
using System.Diagnostics;
using System.Linq;
using System.Linq.Expressions;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Contexts;
using System.Web;
using System.Web.Mvc;
using System.Web.UI.MobileControls;
using Naspinski.IQueryableSearch;

using Tools.CrashReporter.CrashReportCommon;
using Tools.CrashReporter.CrashReportWebSite.Controllers;

namespace Tools.CrashReporter.CrashReportWebSite.Models
{
    public class DatabaseHelper
    {
        public static string CrashReporterConnectionString =
            "Data Source=db-09;Initial Catalog=CrashReport;Server=db-09;Database=CrashReport;Integrated Security=true";

        public static CrashReportDataContext Context = new CrashReportDataContext();

        /// <summary>
        /// Retrieves a list of distinct UE4 Branches from the CrashRepository
        /// </summary>
        /// <returns></returns>
        public static List<SelectListItem> GetBranches()
        {
            var lm = Context.Crashes.Where(n => n.Branch.Contains("UE4")).Select(n => n.Branch).Distinct();
            var list = lm.Select(listitem => new SelectListItem {Selected = false, Text = listitem, Value = listitem}).ToList();
            list.Insert(0,new SelectListItem{Selected = true, Text = "", Value = ""});
            return list;
        }

        /// <summary>
        /// Performs a direct SQL Query to the server, server runs query and returns the results. Should save A LOT of time!
        /// </summary>
        /// <param name="formData"></param>
        /// <returns></returns>
        public static IEnumerable<Crash> GetCrashesFromSQLQuery(FormHelper FormData)
        {
            DataTable table = new DataTable();
            DateTime Epoch = new DateTime(1970, 1, 1);
            IEnumerable<Crash> test = null;

            var ds = new DataTable();
            var DateFrom = (long)(FormData.DateFrom - Epoch).TotalMilliseconds;
            var DateTo = (long)(FormData.DateFrom - Epoch).TotalMilliseconds;

            string crashQuery = "SELECT * FROM Crashes WHERE ";

            using (var connection = new SqlConnection(CrashReporterConnectionString))
            {
                connection.Open();
                
                //First add the dates to the query, (if there are any)
                crashQuery = crashQuery + "DateFrom >= @dateFrom AND DateTo <= @dateTo";

                if (!string.IsNullOrEmpty(FormData.SearchQuery))
                {
                    crashQuery = crashQuery + " AND Username = @username";

                }
                if (!string.IsNullOrEmpty(FormData.MachineIdQuery))
                {
                    crashQuery = crashQuery + " AND ComputerName = @computername";
                    
                }
                if (!string.IsNullOrEmpty(FormData.JiraQuery))
                {
                    crashQuery = crashQuery + " AND TTPID = @ttpid";
                    
                }
                if (!string.IsNullOrEmpty(FormData.EpicIdQuery))
                {
                    crashQuery = crashQuery + " AND EpicAccountId = @epicaccount";
                    
                }
                if (!string.IsNullOrEmpty(FormData.GameName))
                {
                    crashQuery = crashQuery + " AND GameName = @gamename";
                    
                }
                if (!string.IsNullOrEmpty(FormData.BranchName))
                {
                    crashQuery = crashQuery + " AND BranchName = @branchname";
                    
                }
                crashQuery = crashQuery + ";";
                var command = new SqlCommand(crashQuery, connection);
                command.Parameters.AddWithValue("@dateFrom", DateFrom);
                command.Parameters.AddWithValue("@dateTo", DateTo);
                command.Parameters.AddWithValue("@username", FormData.SearchQuery);
                command.Parameters.AddWithValue("@computername", FormData.MachineIdQuery);
                command.Parameters.AddWithValue("@ttpid", FormData.JiraQuery);
                command.Parameters.AddWithValue("@epicaccount", FormData.EpicIdQuery);
                command.Parameters.AddWithValue("@gamename", FormData.GameName);
                command.Parameters.AddWithValue("@branchname", FormData.BranchName);
                SqlDataAdapter da = new SqlDataAdapter(command);
                da.Fill(table);
                connection.Close();
            }
            return test;
        }
    }
}