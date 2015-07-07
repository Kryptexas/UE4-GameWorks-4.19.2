using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	class NodeInfo
	{
		public string Name;
		public GUBP.GUBPNode Node;
		public NodeInfo[] Dependencies;
		public NodeInfo[] PseudoDependencies;
		public NodeInfo[] AllDirectDependencies;
		public NodeInfo[] AllIndirectDependencies;
		public NodeInfo[] ControllingTriggers;
		public int FrequencyShift;
		public bool IsComplete;
		public string[] RecipientsForFailureEmails;
		public bool AddSubmittersToFailureEmails;

		public string ControllingTriggerDotName
		{
			get { return String.Join(".", ControllingTriggers.Select(x => x.Name)); }
		}

		public bool DependsOn(NodeInfo Node)
		{
			return AllIndirectDependencies.Contains(Node);
		}

		public override string ToString()
		{
			System.Diagnostics.Trace.TraceWarning("Implicit conversion from NodeInfo to string\n{0}", Environment.StackTrace);
			return Name;
		}
	}
}
