using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	class BuildNode
	{
		public string Name;
		public GUBP.GUBPNode Node;
		public BuildNode[] Dependencies;
		public BuildNode[] PseudoDependencies;
		public BuildNode[] AllDirectDependencies;
		public BuildNode[] AllIndirectDependencies;
		public BuildNode[] ControllingTriggers;
		public int FrequencyShift;
		public bool IsComplete;
		public string[] RecipientsForFailureEmails;
		public bool AddSubmittersToFailureEmails;
		public int AgentMemoryRequirement;

		public string ControllingTriggerDotName
		{
			get { return String.Join(".", ControllingTriggers.Select(x => x.Name)); }
		}

		public bool DependsOn(BuildNode Node)
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
