using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Tools.CrashReporter.CrashReportCommon;
using Tools.CrashReporter.CrashReportWebSite.Properties;

namespace Tools.CrashReporter.CrashReportWebSite.Classes
{
    public class PerformanceTracker
    {
        private int _count = 0;
        private int _reportsThreshold = 100;

        private Dictionary<string, double> _actionTimeDictionary;

        private SlackWriter _slackWriter;

        public PerformanceTracker()
        {
            _slackWriter = new SlackWriter()
            {
                WebhookUrl = Settings.Default.SlackWebhookUrl,
                Channel = Settings.Default.SlackChannel,
                Username = Settings.Default.SlackUsername,
                IconEmoji = Settings.Default.SlackEmoji
            };
            _actionTimeDictionary = new Dictionary<string, double>();
        }

        public PerformanceTracker(int reportsThreshold = 100)
        {
            _reportsThreshold = reportsThreshold;
            _slackWriter = new SlackWriter()
            {
                WebhookUrl = Settings.Default.SlackWebhookUrl,
                Channel = Settings.Default.SlackChannel,
                Username = Settings.Default.SlackUsername,
                IconEmoji = Settings.Default.SlackEmoji
            };
            _actionTimeDictionary = new Dictionary<string, double>();
        }

        public void AddStat(string actionName, double elapsedTime)
        {
            if (_actionTimeDictionary.ContainsKey(actionName))
            {
                _actionTimeDictionary[actionName] += elapsedTime;
            }
            else
            {
                _actionTimeDictionary.Add(actionName, elapsedTime);
            }
        }

        public void ClearStats()
        {
            _actionTimeDictionary.Clear();
            _count = 0;
        }

        public void IncrementCount()
        {
            _count++;
            if (_count >= _reportsThreshold)
            {
                WritePerfReport();
                ClearStats();
            }
        }

        public void WritePerfReport()
        {
            foreach (var stat in _actionTimeDictionary)
            {
                _slackWriter.Write("Average time for " + stat.Key + " over last " + _reportsThreshold.ToString("N") + " crashes. : " + (stat.Value / _reportsThreshold).ToString("F2"));
            }
            _count -= _reportsThreshold;
        }

        public int GetCount()
        {
            return _count;
        }
    }
}