using System.IO;
using System.Linq;

namespace Tools.CrashReporter.CrashReportProcess
{
	/// <summary>
	/// A queue of pending reports in a specific folder
	/// </summary>
	class ReceiverReportQueue : ReportQueueBase
	{
		protected override string QueueProcessingStartedEventName
		{ 
			get
			{
				return StatusReportingConstants.ProcessingStartedReceiverEvent;
			}
		}

		/// <summary>
		/// Constructor taking the landing zone
		/// </summary>
		public ReceiverReportQueue(string InQueueName, string LandingZonePath)
			: base(InQueueName, LandingZonePath)
		{
		}

		protected override int GetTotalWaitingCount()
		{
			return LastQueueSizeOnDisk;
		}
	}
}