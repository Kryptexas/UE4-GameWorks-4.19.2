// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System.Collections.Generic;
using Tools.CrashReporter.CrashReportCommon;

namespace Tools.CrashReporter.CrashReportProcess
{
	interface IReportQueue
	{
		int CheckForNewReports();
		bool TryDequeueReport(out FGenericCrashContext Context);
		void CleanLandingZone();

		string QueueId { get; }
		string LandingZonePath { get; }
	}

	abstract class ReportQueueBase : IReportQueue
	{
		public abstract int CheckForNewReports();
		public abstract bool TryDequeueReport(out FGenericCrashContext Context);
		public abstract void CleanLandingZone();

		public abstract string QueueId { get; }
		public abstract string LandingZonePath { get; }
	}
}
