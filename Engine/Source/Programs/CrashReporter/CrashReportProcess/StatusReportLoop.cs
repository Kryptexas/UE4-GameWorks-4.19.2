using System;
using System.Collections.Generic;
using System.Data.Linq.Mapping;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Tools.CrashReporter.CrashReportProcess
{
	class StatusReportLoop : IDisposable
	{
		public TimeSpan InitialPause { get; set; }

		public TimeSpan LoopPeriod { get; set; }

		public void Dispose()
		{
			CancelSource.Cancel();
			lock (TaskMonitor)
			{
				Monitor.PulseAll(TaskMonitor);
			}
			LoopTask.Wait();
			CancelSource.Dispose();
		}

		public StatusReportLoop(TimeSpan InLoopPeriod, Func<TimeSpan, string> LoopFunction)
		{
			InitialPause = TimeSpan.FromSeconds(30);
			LoopPeriod = InLoopPeriod;
			LoopTask = Task.Factory.StartNew(() =>
			{
				var Cancel = CancelSource.Token;

				// Small pause to allow the app to get up and running before we run the first report
				Thread.Sleep(InitialPause);
				DateTime PeriodStartTime = CreationTimeUtc;

				lock (TaskMonitor)
				{
					while (!Cancel.IsCancellationRequested)
					{
						DateTime PeriodEndTime = DateTime.UtcNow;
						string ReportMessage = LoopFunction.Invoke(PeriodEndTime - PeriodStartTime);
						PeriodStartTime = PeriodEndTime;

						if (!string.IsNullOrWhiteSpace(ReportMessage))
						{
							CrashReporterProcessServicer.WriteSlack(ReportMessage);
						}

						Monitor.Wait(TaskMonitor, LoopPeriod);
					}
				}
			});
		}

		private Task LoopTask;
		private readonly CancellationTokenSource CancelSource = new CancellationTokenSource();
		private readonly DateTime CreationTimeUtc = DateTime.UtcNow;
		private readonly Object TaskMonitor = new Object();
	}
}
