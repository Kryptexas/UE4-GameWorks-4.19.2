using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Tools.CrashReporter.CrashReportProcess
{
	class StatusReporting : IDisposable
	{
		public void InitQueue(string QueueName, string QueueLocation)
		{
			QueueLocations.Add(QueueName, QueueLocation);
			QueueSizes.Add(QueueName, 0);
		}

		public void SetQueueSize(string QueueName, int Size)
		{
			lock (DataLock)
			{
				QueueSizes[QueueName] = Size;
				bQueueSizesAvail = true;
			}
		}

		public void IncrementCount(string CountName)
		{
			lock (DataLock)
			{
				if (Counters.ContainsKey(CountName))
				{
					Counters[CountName]++;
				}
				else
				{
					Counters.Add(CountName, 1);
				}
			}
		}

		public void Alert(string AlertText)
		{
			// Report an alert condition to Slack immediately
			CrashReporterProcessServicer.WriteSlack("@channel ALERT: " + AlertText);
		}

		public void Start()
		{
			CrashReporterProcessServicer.WriteSlack(string.Format("CRP started (version {0})", Config.Default.VersionString));

			StringBuilder StartupMessage = new StringBuilder();

			StartupMessage.AppendLine("Queues...");
			foreach (var Queue in QueueLocations)
			{
				StartupMessage.AppendLine("> " + Queue.Key + " @ " + Queue.Value);
			}
			CrashReporterProcessServicer.WriteSlack(StartupMessage.ToString());

			StatusReportingTask = Task.Factory.StartNew(() =>
			{
				var Cancel = CancelSource.Token;

				lock (TaskMonitor)
				{
					while (!Cancel.IsCancellationRequested)
					{
						StringBuilder StatusReportMessage = new StringBuilder();
						lock (DataLock)
						{
							Dictionary<string, int> CountsInPeriod = GetCountsInPeriod(Counters, CountersAtLastReport);

							if (bQueueSizesAvail)
							{
								int ProcessingStartedInPeriod = 0;
								CountsInPeriod.TryGetValue(StatusReportingConstants.ProcessingStartedEvent, out ProcessingStartedInPeriod);
								if (ProcessingStartedInPeriod > 0)
								{
									int QueueSizeSum = QueueSizes.Values.Sum();
									TimeSpan MeanWaitTime =
										TimeSpan.FromMinutes(0.5*Config.Default.MinutesBetweenQueueSizeReports*
										                     (QueueSizeSum + QueueSizeSumAtLastReport)/ProcessingStartedInPeriod);
									QueueSizeSumAtLastReport = QueueSizeSum;

									string WaitTimeString;
									if (MeanWaitTime == TimeSpan.Zero)
									{
										WaitTimeString = "nil";
									}
									else if (MeanWaitTime < TimeSpan.FromMinutes(2))
									{
										WaitTimeString = MeanWaitTime.TotalSeconds + " seconds";
									}
									else
									{
										WaitTimeString = Convert.ToInt32(MeanWaitTime.TotalMinutes) + " minutes";
									}
									StartupMessage.AppendLine("Queue waiting time " + WaitTimeString);
								}

								StatusReportMessage.AppendLine("Queue sizes...");
								foreach (var Queue in QueueSizes)
								{
									StatusReportMessage.AppendLine("> " + Queue.Key + " " + Queue.Value);
								}
							}
							if (CountsInPeriod.Count > 0)
							{
								StatusReportMessage.AppendLine(string.Format("Events in the last {0} minutes...",
									Config.Default.MinutesBetweenQueueSizeReports));
								foreach (var CountInPeriod in CountsInPeriod)
								{
									StatusReportMessage.AppendLine("> " + CountInPeriod.Key + " " + CountInPeriod.Value);
								}
							}
						}
						CrashReporterProcessServicer.WriteSlack(StatusReportMessage.ToString());

						Monitor.Wait(TaskMonitor, TimeSpan.FromMinutes(Config.Default.MinutesBetweenQueueSizeReports));
					}
				}
			});
		}

		public void Dispose()
		{
			if (StatusReportingTask != null)
			{
				CancelSource.Cancel();
				lock (TaskMonitor)
				{
					Monitor.PulseAll(TaskMonitor);
				}
				StatusReportingTask.Wait();
				CancelSource.Dispose();
			}

			CrashReporterProcessServicer.WriteSlack("CRP stopped");
		}

		private static Dictionary<string, int> GetCountsInPeriod(Dictionary<string, int> InCounters, Dictionary<string, int> InCountersAtLastReport)
		{
			Dictionary<string, int> CountsInPeriod = new Dictionary<string, int>();

			foreach (var Counter in InCounters)
			{
				int LastCount = 0;
				if (!InCountersAtLastReport.TryGetValue(Counter.Key, out LastCount))
				{
					InCountersAtLastReport.Add(Counter.Key, 0);
				}
				CountsInPeriod.Add(Counter.Key, Counter.Value - LastCount);
				InCountersAtLastReport[Counter.Key] = Counter.Value;
			}

			return CountsInPeriod;
		}

		private Task StatusReportingTask;
		private readonly CancellationTokenSource CancelSource = new CancellationTokenSource();
		private readonly Dictionary<string, string> QueueLocations = new Dictionary<string, string>();
		private readonly Dictionary<string, int> QueueSizes = new Dictionary<string, int>();
		private readonly Dictionary<string, int> Counters = new Dictionary<string, int>();
		private int QueueSizeSumAtLastReport = 0;
		private readonly Dictionary<string, int> CountersAtLastReport = new Dictionary<string, int>();
		private readonly Object TaskMonitor = new Object();
		private readonly Object DataLock = new Object();
		private bool bQueueSizesAvail = false;
	}
}
