// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using Amazon.Runtime;
using Amazon.S3;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Web;
using System.Web.Helpers;
using System.Xml;
using Tools.CrashReporter.CrashReportCommon;
using Tools.CrashReporter.CrashReportWebSite.Models;
using Tools.CrashReporter.CrashReportWebSite.Properties;

namespace Tools.CrashReporter.CrashReportWebSite.DataModels
{
    /// <summary>
    /// 
    /// </summary>
    public partial class Crash
    {
        public CallStackContainer CallStackContainer { get; set; }

        /// <summary>Crash context for this Crash.</summary>
        FGenericCrashContext _CrashContext = null;

        bool _bUseFullMinidumpPath = false;

        private string SitePath = Properties.Settings.Default.SitePath;

        public static AmazonClient AmazonClient { get; private set; }

        public List<String> Plugins { get; private set; }

        static Crash()
        {
            string AWSError;
            AWSCredentials AWSCredentialsForOutput = new StoredProfileAWSCredentials(Settings.Default.AWSS3ProfileName, Settings.Default.AWSCredentialsFilepath);
            AmazonS3Config S3ConfigForOutput = new AmazonS3Config
            {
                ServiceURL = Settings.Default.AWSS3URL
            };

            AmazonClient = new AmazonClient(AWSCredentialsForOutput, null, S3ConfigForOutput, out AWSError);

            if (!AmazonClient.IsS3Valid)
            {
                System.Diagnostics.Debug.WriteLine("Failed to initailize S3");
            }
        }

        /// <summary>If available, will read CrashContext.runtime-xml.</summary>
        public void ReadCrashContextIfAvailable()
        {
            try
            {
                //\\epicgames.net\Root\Projects\Paragon\QA_CrashReports
                bool bHasCrashContext = HasCrashContextFile();
                Plugins = new List<string>();
                if (bHasCrashContext)
                {
                    //_CrashContext = FGenericCrashContext.FromFile(SitePath + GetCrashContextUrl());
                    _CrashContext = FGenericCrashContext.FromUrl(GetCrashContextUrl());
                    bool bTest = _CrashContext != null && !string.IsNullOrEmpty(_CrashContext.PrimaryCrashProperties.FullCrashDumpLocation);
                    if (bTest)
                    {
                        _bUseFullMinidumpPath = true;

                        //Some temporary code to redirect to the new file location for fulldumps for paragon.
                        //Consider removing this once fulldumps stop appearing in the old location.
                        if (_CrashContext.PrimaryCrashProperties.FullCrashDumpLocation.ToLower()
                                .Contains("\\\\epicgames.net\\root\\dept\\gameqa\\paragon\\paragon_launcherCrashdumps"))
                        {
                            //Files from old versions of the client may end up in the old location. Check for files there first.
                            if (!System.IO.Directory.Exists(_CrashContext.PrimaryCrashProperties.FullCrashDumpLocation))
                            {
                                var suffix =
                                _CrashContext.PrimaryCrashProperties.FullCrashDumpLocation.Substring("\\\\epicgames.net\\root\\dept\\gameqa\\paragon\\paragon_launcherCrashdumps".Length);
                                _CrashContext.PrimaryCrashProperties.FullCrashDumpLocation = String.Format("\\\\epicgames.net\\Root\\Projects\\Paragon\\QA_CrashReports{0}", suffix);

                                //If the file doesn't exist in the new location either then don't use the full minidump path.
                                _bUseFullMinidumpPath =
                                    System.IO.Directory.Exists(_CrashContext.PrimaryCrashProperties.FullCrashDumpLocation);
                            }
                        }

                        //End of temporary code.

                        FLogger.Global.WriteEvent("ReadCrashContextIfAvailable " + _CrashContext.PrimaryCrashProperties.FullCrashDumpLocation + " is " + _bUseFullMinidumpPath);
                    }

                    if (_CrashContext.UntypedEnabledPlugins != null)
                    {
                        XmlNode[] nodes = _CrashContext.UntypedEnabledPlugins as XmlNode[];

                        if (nodes != null)
                        {
                            foreach (var node in nodes)
                            {
                                try
                                {
                                    dynamic data = Json.Decode(node.InnerText);

                                    String printName = data.FriendlyName + " " + data.VersionName;
                                    System.Diagnostics.Debug.WriteLine(printName);

                                    Plugins.Add(printName);
                                }
                                catch (Exception e)
                                {
                                    Debug.WriteLine("Error trying to add a plugin entry: " + e.ToString());
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception Ex)
            {
                Debug.WriteLine("Exception in ReadCrashContextIfAvailable: " + Ex.ToString());
            }
        }

        /// <summary>Return true, if there is a Crash context file associated with the Crash</summary>
        public bool HasCrashContextFile()
        {
            var Path = GetCrashContextUrl();
            return !String.IsNullOrEmpty(Path);
        }

        /// <summary>Whether to use the full minidump.</summary>
        public bool UseFullMinidumpPath()
        {
            return _bUseFullMinidumpPath;
        }

        /// <summary>Return true, if there is a metadata file associated with the Crash</summary>
        public bool HasMetaDataFile()
        {
            var Path = GetMetaDataUrl();
            return !String.IsNullOrEmpty(Path);
        }

        /// <summary>
        /// Retrieve the parsed callstack for the Crash.
        /// </summary>
        /// <param name="CrashInstance">The Crash we require the callstack for.</param>
        /// <returns>A class containing the parsed callstack.</returns>
        public CallStackContainer GetCallStack()
        {
            return new CallStackContainer(this.CrashType, this.RawCallStack, this.PlatformName);
        }

        /// <summary>
        /// Return the Url of the log.
        /// </summary>
        public string GetLogUrl()
        {
            // Check compressed first. Then uncompressed
            string fileName = GetFilePath("_Launch.log" + Settings.Default.AWSS3CompressedSuffix);
            if (fileName == null)
            {
                GetFilePath("_Launch.log");
            }

            return fileName;
        }

        /// <summary>
        /// Return the Url of the minidump.
        /// </summary>
        public string GetMiniDumpUrl()
        {
            // Check compressed first. Then uncompressed
            string fileName = GetFilePath("_MiniDump.dmp" + Settings.Default.AWSS3CompressedSuffix);
            if (fileName == null)
            {
                GetFilePath("_MiniDump.dmp");
            }

            return fileName;
        }

        /// <summary>
        /// Tooltip for the dump.
        /// </summary>
        public string GetDumpTitle()
        {
            var Title = UseFullMinidumpPath() ?
                "Copy the link and open in the explorer" : "";
            return Title;
        }

        /// <summary>
        /// Return the short name of the dump.
        /// </summary>
        public string GetDumpName()
        {
            return UseFullMinidumpPath() ? "Fulldump" : "Minidump";
        }

        /// <summary>
        /// Return the Url of the diagnostics file.
        /// </summary>
        public string GetDiagnosticsUrl()
        {
            return GetFilePath("_Diagnostics.txt");
        }

        /// <summary>
        /// Return the Url of the Crash context file.
        /// </summary>
        public string GetCrashContextUrl()
        {
            return GetFilePath("_CrashContext.runtime-xml");
        }

        /// <summary>
        /// Return the Url of the Windows Error Report meta data file.
        /// </summary>
        public string GetMetaDataUrl()
        {
            return GetFilePath("_WERMeta.xml");
        }

        /// <summary>
        /// Return the Url of the report video file.
        /// </summary>
        /// <returns>The Url of the Crash report video file.</returns>
        public string GetVideoUrl()
        {
            return GetFilePath("_CrashVideo.avi");
        }

        /// <summary>
        /// Return a display friendly version of the time of Crash.
        /// </summary>
        /// <returns>A pair of strings representing the date and time of the Crash.</returns>
        public string[] GetTimeOfCrash()
        {
            string[] Results = new string[2];
            
            DateTime LocalTimeOfCrash = TimeOfCrash.ToLocalTime();
            Results[0] = LocalTimeOfCrash.ToShortDateString();
            Results[1] = LocalTimeOfCrash.ToShortTimeString();

            return Results;
        }

        /// <summary>
        /// Return lines of processed callstack entries.
        /// </summary>
        /// <param name="StartIndex">The starting entry of the callstack.</param>
        /// <param name="Count">The number of lines to return.</param>
        /// <returns>Lines of processed callstack entries.</returns>
        public List<CallStackEntry> GetCallStackEntries(int StartIndex, int Count)
        {
            using (FAutoScopedLogTimer LogTimer = new FAutoScopedLogTimer(this.GetType().ToString() + "(Count=" + Count + ")"))
            {
                IEnumerable<CallStackEntry> Results = new List<CallStackEntry>() { new CallStackEntry() };

                if (CallStackContainer != null && CallStackContainer.CallStackEntries != null)
                {
                    int MaxCount = Math.Min(Count, CallStackContainer.CallStackEntries.Count);
                    Results = CallStackContainer.CallStackEntries.Take(MaxCount);
                }

                return Results.ToList();
            }
        }

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

        /// <summary>Return true, if there is a minidump file associated with the Crash</summary>
        public bool GetHasMiniDumpFile()
        {
            var Path = GetMiniDumpUrl();
            return !String.IsNullOrEmpty(Path);
        }

        /// <summary>Return true, if there is a video file associated with the Crash</summary>
        public bool GetHasVideoFile()
        {
            var Path = GetVideoUrl();
            return !String.IsNullOrEmpty(Path);
        }

        /// <summary>Return true, if there is a log file associated with the Crash</summary>
        public bool GetHasLogFile()
        {
            var Path = GetLogUrl();

            return !String.IsNullOrEmpty(Path);
        }

        /// <summary>Return true, if there is a diagnostics file associated with the Crash</summary>
        public bool GetHasDiagnosticsFile()
        {
            var Path = GetDiagnosticsUrl();
            return !String.IsNullOrEmpty(Path);
        }
        
        private string GetFilePath(String FileName)
        {
            string Path = null;

            if (Settings.Default.DownloadFromS3)
            {
                int ReportIDSegment = (Id / 10000) * 10000;

                string reportPath = string.Format("{0}/{1}/{2}/{2}{3}", Settings.Default.AWSS3KeyPrefix, ReportIDSegment, Id, FileName);

                Path = AmazonClient.GetS3SignedUrl("crashreporter", reportPath, 60);
            }
            else
            {
                Path = Properties.Settings.Default.CrashReporterFiles + Id + FileName;
            }

            return Path;
        }
    }
}