using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	public class LegacyNode : BuildNode
	{
		public LegacyNode(GUBP.GUBPNode InNode)
		{
			Name = InNode.GetFullName();
			Node = InNode;
			AgentSharingGroup = Node.AgentSharingGroup;
			AgentMemoryRequirement = Node.AgentMemoryRequirement();
			TimeoutInMinutes = Node.TimeoutInMinutes();
			SendSuccessEmail = Node.SendSuccessEmail();
			Priority = Node.Priority();
		}

		public override bool IsSticky
		{
			get { return Node.IsSticky(); }
		}
	}
}
