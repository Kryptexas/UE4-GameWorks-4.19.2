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
		public const string ProcessingStartedReceiverEvent = "Started processing (from CRR)";
		public const string ProcessingStartedDataRouterEvent = "Started processing (from Data Router)";
		public const string QueuedEvent = "Queued for processing";
	}
}
