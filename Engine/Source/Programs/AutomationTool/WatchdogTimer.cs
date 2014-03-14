// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Timers;
using System.Runtime.CompilerServices;
using UnrealBuildTool;

namespace AutomationTool
{
	/// <summary>
	/// Timer that exits the application of the execution of the command takes too long.
	/// </summary>
	public class WatchdogTimer : IDisposable
	{
		private Timer CountownTimer;
		private System.Diagnostics.StackFrame TimerFrame;

		[MethodImplAttribute(MethodImplOptions.NoInlining)]
		public WatchdogTimer(int Seconds)
		{
			TimerFrame = new System.Diagnostics.StackFrame(1);  
			CountownTimer = new Timer(Seconds * 1000.0);
			CountownTimer.Elapsed += Elapsed;
			CountownTimer.Start();
		}

		void Elapsed(object sender, ElapsedEventArgs e)
		{
			var Method = TimerFrame.GetMethod();
			Log.TraceError("BUILD FAILED: WatchdogTimer timed out after {0}s in {1}.{2}", (int)(CountownTimer.Interval * 0.001), Method.DeclaringType.Name, Method.Name);
			Environment.Exit(1);
		}

		public void Dispose()
		{
			if (CountownTimer != null)
			{
				CountownTimer.Dispose();
				CountownTimer = null;
			}
		}
	}
}
