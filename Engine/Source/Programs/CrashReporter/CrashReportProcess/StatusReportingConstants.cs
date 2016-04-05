using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Tools.CrashReporter.CrashReportProcess
{
	static class StatusReportingConstants
	{
		public const string ProcessingFailedEvent = "Processing failed";
		public const string ProcessingSucceededEvent = "Processing succeeded";
		public const string ExceptionEvent = "Handled exceptions";
		public const string ProcessingStartedEvent = "Started processing";
		public const string QueuedEvent = "Queued for processing";
	}
}
