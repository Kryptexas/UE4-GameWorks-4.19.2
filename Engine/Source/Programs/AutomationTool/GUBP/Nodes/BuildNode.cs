using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	abstract class BuildNode
	{
		public string Name;
		public string AgentSharingGroup;
		public int FrequencyShift;
		public int AgentMemoryRequirement;
		public int TimeoutInMinutes;

		public GUBP.GUBPNode Node;
		public BuildNode[] Dependencies;
		public BuildNode[] PseudoDependencies;
		public BuildNode[] AllDirectDependencies;
		public BuildNode[] AllIndirectDependencies;
		public BuildNode[] ControllingTriggers;
		public bool IsComplete;
		public string[] RecipientsForFailureEmails;
		public bool AddSubmittersToFailureEmails;
		public bool SendSuccessEmail;

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
