// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net;
using System.Web;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Tools.CrashReporter.CrashReportCommon;
using Tools.CrashReporter.CrashReportWebSite.Properties;
using Tools.CrashReporter.CrashReportWebSite.ViewModels;

namespace Tools.CrashReporter.CrashReportWebSite.DataModels
{
    public partial class Bugg
    {
        public int CrashesInTimeFrameGroup { get; set; }
        public int CrashesInTimeFrameAll { get; set; }
        public int NumberOfUniqueMachines { get; set; }

        /// <summary>  </summary>
        public SortedSet<string> AffectedPlatforms { get; set; }

        /// <summary></summary>
        public SortedSet<string> AffectedVersions { get; set; }

        /// <summary> 4.4, 4.5 and so. </summary>
        public SortedSet<string> AffectedMajorVersions { get; set; }

        /// <summary>  </summary>
        public SortedSet<string> BranchesFoundIn { get; set; }
        /// <summary></summary>
        public string JiraSummary { get; set; }

        /// <summary>The first line of the callstack</summary>
        string ToJiraSummary = "";

        /// <summary></summary>
        public string JiraComponentsText { get; set; }

        /// <summary></summary>
        public string JiraResolution { get; set; }

        /// <summary></summary>
        public string JiraFixVersionsText { get; set; }

        /// <summary></summary>
        public string JiraFixCL { get; set; }
        public List<string> GenericFunctionCalls = null;
        public List<string> FunctionCalls = new List<string>();

        List<string> ToJiraDescriptions = new List<string>();

        List<string> ToJiraFunctionCalls = null;

        /// <summary></summary>
        public int LatestCLAffected { get; set; }

        /// <summary></summary>
        public string LatestOSAffected { get; set; }

        /// <summary></summary>
        public string LatestCrashSummary { get; set; }

        /// <summary>Branches in JIRA</summary>
        HashSet<string> ToJiraBranches = null;

        /// <summary>Versions in JIRA</summary>
        List<object> ToJiraVersions = null;

        /// <summary>Platforms in JIRA</summary>
        List<object> ToJiraPlatforms = null;

        /// <summary>Branches in jira summary fortnite</summary>
        List<object> ToBranchName = null;

        /// <summary>The CL of the oldest build when this bugg is occurring</summary>
        int ToJiraFirstCLAffected { get; set; }

        public string JiraProject { get; set; }

        /// <summary></summary>
        public string GetCrashTypeAsString()
        {
            if (CrashType == 1)
            {
                return "Crash";
            }
            else if (CrashType == 2)
            {
                return "Assert";
            }
            else if (CrashType == 3)
            {
                return "Ensure";
            }
            else if (CrashType == 4)
            {
                return "GPUCrash";
            }
            return "Unknown";
        }

        /// <summary>
        /// Prepares Bugg for JIRA
        /// </summary>
        /// <param name="CrashesForBugg"></param>
        public void PrepareBuggForJira(List<CrashDataModel> CrashesForBugg)
        {
            var jiraConnection = JiraConnection.Get();

            this.AffectedVersions = new SortedSet<string>();
            this.AffectedMajorVersions = new SortedSet<string>(); // 4.4, 4.5 and so on
            this.BranchesFoundIn = new SortedSet<string>();
            this.AffectedPlatforms = new SortedSet<string>();
            var hashSetDescriptions = new HashSet<string>();

            var machineIds = new HashSet<string>();
            var firstClAffected = int.MaxValue;

            foreach (var crashDataModel in CrashesForBugg)
            {
                // Only add machine if the number has 32 characters
                if (crashDataModel.ComputerName != null && crashDataModel.ComputerName.Length == 32)
                {
                    machineIds.Add(crashDataModel.ComputerName);
                    if (crashDataModel.Description.Length > 4)
                    {
                        hashSetDescriptions.Add(crashDataModel.Description);
                    }
                }

                if (!string.IsNullOrEmpty(crashDataModel.BuildVersion))
                {
                    this.AffectedVersions.Add(crashDataModel.BuildVersion);
                }
                // Depot || Stream
                if (!string.IsNullOrEmpty(crashDataModel.Branch))
                {
                    this.BranchesFoundIn.Add(crashDataModel.Branch);
                }

                var CrashBuiltFromCl = 0;
                int.TryParse(crashDataModel.ChangelistVersion, out CrashBuiltFromCl);
                if (CrashBuiltFromCl > 0)
                {
                    firstClAffected = Math.Min(firstClAffected, CrashBuiltFromCl);
                }

                if (!string.IsNullOrEmpty(crashDataModel.PlatformName))
                {
                    // Platform = "Platform [Desc]";
                    var platSubs = crashDataModel.PlatformName.Split(" ".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);
                    if (platSubs.Length >= 1)
                    {
                        this.AffectedPlatforms.Add(platSubs[0]);
                    }
                }
            }

            //ToJiraDescriptons
            foreach (var line in hashSetDescriptions)
            {
                var listItem = "- " + HttpUtility.HtmlEncode(line);
                ToJiraDescriptions.Add(listItem);
            }

            this.ToJiraFirstCLAffected = firstClAffected;
            if (this.AffectedVersions.Count > 0)
            {
                this.BuildVersion = this.AffectedVersions.Last();	// Latest Version Affected
            }

            foreach (var affectedBuild in this.AffectedVersions)
            {
                var subs = affectedBuild.Split(".".ToCharArray(), StringSplitOptions.RemoveEmptyEntries);
                if (subs.Length >= 2)
                {
                    var majorVersion = subs[0] + "." + subs[1];
                    this.AffectedMajorVersions.Add(majorVersion);
                }
            }

            var bv = this.BuildVersion;
            var latestClAffected = CrashesForBugg.				// CL of the latest build
                Where(Crash => Crash.BuildVersion == bv).
                Max(Crash => Crash.ChangelistVersion);

            var ILatestCLAffected = -1;
            int.TryParse(latestClAffected, out ILatestCLAffected);
            this.LatestCLAffected = ILatestCLAffected;			// Latest CL Affected

            var latestOsAffected = CrashesForBugg.OrderByDescending(Crash => Crash.TimeOfCrash).First().PlatformName;
            this.LatestOSAffected = latestOsAffected;			// Latest Environment Affected

            // ToJiraSummary
            var functionCalls = new CallStackContainer(CrashesForBugg.First().CrashType, CrashesForBugg.First().RawCallStack, CrashesForBugg.First().PlatformName).GetFunctionCallsForJira();
            if (functionCalls.Count > 0)
            {
                this.ToJiraSummary = functionCalls[0];
                this.ToJiraFunctionCalls = functionCalls;
            }
            else
            {
                this.ToJiraSummary = "No valid callstack found";
                this.ToJiraFunctionCalls = new List<string>();
            }

            // ToJiraVersions
            this.ToJiraVersions = new List<object>();
            foreach (var version in this.AffectedMajorVersions)
            {
                var bValid = jiraConnection.GetNameToVersions().ContainsKey(version);
                if (bValid)
                {
                    this.ToJiraVersions.Add(jiraConnection.GetNameToVersions()[version]);
                }
            }

            // ToJiraBranches
            this.ToJiraBranches = new HashSet<string>();
            foreach (var branchName in this.BranchesFoundIn)
            {
                if (!string.IsNullOrEmpty(branchName))
                {
                    // Stream
                    if (branchName.StartsWith("//"))
                    {
                        this.ToJiraBranches.Add(branchName);
                    }
                    // Depot
                    else
                    {
                        this.ToJiraBranches.Add(CrashReporterConstants.P4_DEPOT_PREFIX + branchName);
                    }
                }
            }

            // ToJiraPlatforms
            this.ToJiraPlatforms = new List<object>();
            foreach (var platform in this.AffectedPlatforms)
            {
                bool bValid = jiraConnection.GetNameToPlatform().ContainsKey(platform);
                if (bValid)
                {
                    this.ToJiraPlatforms.Add(jiraConnection.GetNameToPlatform()[platform]);
                }
            }

            // ToJiraPlatforms
            this.ToBranchName = new List<object>();
            foreach (var BranchName in this.BranchesFoundIn)
            {
                bool bValid = jiraConnection.GetNameToBranch().ContainsKey(BranchName);
                if (bValid)
                {
                    this.ToBranchName.Add(jiraConnection.GetNameToBranch()[BranchName]);
                }
            }
        }

        /// <summary>
        /// Create a new Jira for this bug
        /// </summary>
        public void CopyToJira()
        {
            var jc = JiraConnection.Get();
            Dictionary<string, object> issueFields;

            switch (this.JiraProject)
            {
                case "UE":
                    issueFields = CreateGeneralIssue(jc);
                    break;
                case "FORT":
                    issueFields = CreateFortniteIssue(jc);
                    break;
                case "ORION":
                    issueFields = CreateOrionIssue(jc);
                    break;
                default:
                    issueFields = CreateProjectIssue(jc, this.JiraProject);
                    break;
            }

            if (jc.CanBeUsed() && string.IsNullOrEmpty(this.TTPID))
            {
                // Additional Info URL / Link to Crash/Bugg
                var key = jc.AddJiraTicket(issueFields);
                if (!string.IsNullOrEmpty(key))
                {
                    TTPID = key;
                }
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public Dictionary<string, object> CreateGeneralIssue(JiraConnection jc)
        {
            var fields = new Dictionary<string, object>();

            fields.Add("project", new Dictionary<string, object> { { "id", 11205 } });	    // UE

            fields.Add("summary", "[CrashReport] " + ToJiraSummary);						// Call Stack, Line 1
            fields.Add("description", string.Join("\r\n", ToJiraDescriptions));			    // Description
            fields.Add("issuetype", new Dictionary<string, object> { { "id", "1" } });	    // Bug
            fields.Add("labels", new string[] { "Crash", "liveissue" });					// <label>Crash, live issue</label>
            fields.Add("customfield_11500", ToJiraFirstCLAffected);						    // Changelist # / Found Changelist
            fields.Add("environment", LatestOSAffected);		    						// Platform

            // Components
            var SupportComponent = jc.GetNameToComponents()["Support"];
            fields.Add("components", new object[] { SupportComponent });

            // ToJiraVersions
            fields.Add("versions", ToJiraVersions);

            // ToJiraBranches
            fields.Add("customfield_12402", ToJiraBranches.ToList());						// NewBranchFoundIn

            // ToJiraPlatforms
            fields.Add("customfield_11203", ToJiraPlatforms);
            
            // Callstack customfield_11807
            string JiraCallstack = "{noformat}" + string.Join("\r\n", ToJiraFunctionCalls) + "{noformat}";
            fields.Add("customfield_11807", JiraCallstack);								    // Callstack

            string BuggLink = "http://Crashreporter/Buggs/Show/" + Id;
            fields.Add("customfield_11205", BuggLink);
            return fields;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="jc"></param>
        /// <returns></returns>
        public Dictionary<string, object> CreateFortniteIssue(JiraConnection jc)
        {
            var fields = new Dictionary<string, object>();

            fields.Add("summary", "[CrashReport] " + ToJiraSummary); // Call Stack, Line 1
            fields.Add("description", string.Join("\r\n", ToJiraDescriptions)); // Description
            fields.Add("project", new Dictionary<string, object> { { "id", 10600 } });
            fields.Add("issuetype", new Dictionary<string, object> { { "id", "1" } }); // Bug

            //branch found in - required = false
            fields.Add("customfield_11201", ToBranchName.ToList());

            //Platforms - required = false
            fields.Add("customfield_11203", new List<object>() { new Dictionary<string, object>() { { "self", Settings.Default.JiraDeploymentAddress + "/customFieldOption/11425" }, { "value", "PC" } } });
            
            // Callstack customfield_11807
            string JiraCallstack = "{noformat}" + string.Join("\r\n", ToJiraFunctionCalls) + "{noformat}";
            fields.Add("customfield_11807", JiraCallstack);         // Callstack

            return fields;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="jc"></param>
        /// <returns></returns>
        public Dictionary<string, object> CreateOrionIssue(JiraConnection jc)
        {
            var fields = new Dictionary<string, object>();

            fields.Add("summary", "[CrashReport] " + ToJiraSummary); // Call Stack, Line 1
            fields.Add("description", string.Join("\r\n", ToJiraDescriptions)); // Description
            fields.Add("project", new Dictionary<string, object> { { "id", 10700 } }); 
            fields.Add("issuetype", new Dictionary<string, object> { { "id", "1" } }); // Bug
            fields.Add("components", new object[]{new Dictionary<string, object>{{"id", "14604"}}});

            return fields;
        }

        /// <summary>
        /// Create an issue description from a project ID
        /// </summary>
        /// <param name="jc"></param>
        /// <param name="projectId"></param>
        /// <returns></returns>
        public Dictionary<string, object> CreateProjectIssue(JiraConnection jc, string projectId)
        {
            var response = jc.JiraRequest("/issue/createmeta?projectKeys="+ projectId +"&expand=projects.issuetypes.fields",
                JiraConnection.JiraMethod.GET, null, HttpStatusCode.OK);

            var issueFields = new Dictionary<string, object>();
            issueFields.Add("summary", "[CrashReport] " + ToJiraSummary); // Call Stack, Line 1
            issueFields.Add("description", string.Join("\r\n", ToJiraDescriptions)); // Description

            using (var responseReader = new StreamReader(response.GetResponseStream()))
            {
                var responseText = responseReader.ReadToEnd();

                JObject jsonObject = JObject.Parse(responseText);

                bool fields = jsonObject["projects"][0]["issuetypes"].Any();

                issueFields.Add("project", new Dictionary<string, object> { { "id", jsonObject["projects"][0]["id"].ToObject<int>() } });

                if (!fields) 
                    return issueFields;

                foreach (
                    JToken issuetype in
                        jsonObject["projects"][0]["issuetypes"])//.[0]["fields"])
                {
                    //Only fill required fields
                    if (issuetype["subtask"].ToObject<bool>() == true)
                        continue;

                    JToken field = issuetype["fields"];

                    if (field["required"].ToObject<bool>() == true)
                    {
                        //don't fill fields with a default value
                        if (field["hasDefaultValue"].ToObject<bool>() == true)
                            continue;

                        JToken outValue;

                        //if (field.Value["schema"]["type"].ToObject<string>() == "array")
                        //{
                        //    if (field.Value["allowedValues"] != null)
                        //    {
                        //        //don't add the same key twice
                        //        if (issueFields.ContainsKey(field.Key))
                        //            continue;

                        //        issueFields.Add(field.Key, new object[]
                        //        {
                        //            new Dictionary<string, object>
                        //            {
                        //                {"id", field.Value["allowedValues"][0]["id"].ToObject<string>()}
                        //            }
                        //        });
                        //    }
                        //}
                        //else
                        //{
                        //    if (obj.TryGetValue("allowedValues", out outValue))
                        //    {
                        //        //don't add the same key twice
                        //        if (issueFields.ContainsKey(field.Key))
                        //            continue;

                        //        issueFields.Add(field.Key,
                        //            new Dictionary<string, object>
                        //            {
                        //                {"id", field.Value["allowedValues"][0]["id"].ToObject<int>()}
                        //            });
                        //    }
                        //}
                    }
                }
            }

            return issueFields;
        }

        /// <summary> Returns concatenated string of fields from the specified JIRA list. </summary>
        public string GetFieldsFrom(List<object> jiraList, string fieldName)
        {
            var results = "";

            foreach (var obj in jiraList)
            {
                var dict = (Dictionary<string, object>)obj;
                try
                {
                    results += (string)dict[fieldName];
                    results += " ";
                }
                catch (System.Exception /*E*/ )
                {

                }
            }

            return results;
        }

        /// <summary> Creates a previews of the bugg, for verifying JIRA's fields. </summary>
        public string ToTooltip()
        {
            string NL = "&#013;";

            string Tooltip = NL;
            Tooltip += "Project: UE" + NL;
            Tooltip += "Summary: " + ToJiraSummary + NL;
            Tooltip += "Description: " + NL + string.Join(NL, ToJiraDescriptions) + NL;
            Tooltip += "Issuetype: " + "1 (bug)" + NL;
            Tooltip += "Labels: " + "Crash" + NL;
            Tooltip += "Changelist # / Found Changelist: " + ToJiraFirstCLAffected + NL;
            Tooltip += "LatestOSAffected: " + LatestOSAffected + NL;

            // "name"
            var JiraVersions = GetFieldsFrom(ToJiraVersions, "name");
            Tooltip += "JiraVersions: " + JiraVersions + NL;

            // "value"
            var jiraBranches = "";
            foreach (var Branch in ToJiraBranches)
            {
                jiraBranches += Branch + ", ";
            }
            Tooltip += "JiraBranches: " + jiraBranches + NL;

            // "value"
            var JiraPlatforms = GetFieldsFrom(ToJiraPlatforms, "value");
            Tooltip += "JiraPlatforms: " + JiraPlatforms + NL;

            var JiraCallstack = "Callstack:" + NL + string.Join(NL, ToJiraFunctionCalls) + NL;
            Tooltip += JiraCallstack;

            return Tooltip;
        }

        /// <summary>
        /// 
        /// </summary>
        public string GetAffectedVersions()
        {
            if (AffectedVersions.Count == 1)
            {
                return AffectedVersions.Max;
            }
            else
            {
                return AffectedVersions.Min + " - " + AffectedVersions.Max;
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public string GetLifeSpan()
        {
            TimeSpan LifeSpan = TimeOfLastCrash.Value - TimeOfFirstCrash.Value;
            // Only to visualize the life span, accuracy not so important here.
            int NumMonths = LifeSpan.Days / 30;
            int NumDays = LifeSpan.Days % 30;
            if (NumMonths > 0)
            {
                return string.Format("{0} month(s) {1} day(s)", NumMonths, NumDays);
            }
            else if (NumDays > 0)
            {
                return string.Format("{0} day(s)", NumDays);
            }
            else
            {
                return "Less than one day";
            }
        }
    }
}