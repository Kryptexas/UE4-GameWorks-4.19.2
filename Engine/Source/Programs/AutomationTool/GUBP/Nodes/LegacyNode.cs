using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	class LegacyNode : BuildNode
	{
		public LegacyNode(GUBP.GUBPBranchConfig InBranchConfig, GUBP.GUBPNode InNode)
		{
			Name = InNode.GetFullName();
			Node = InNode;
			AgentSharingGroup = Node.AgentSharingGroup;
			AgentMemoryRequirement = Node.AgentMemoryRequirement(InBranchConfig);
			TimeoutInMinutes = Node.TimeoutInMinutes();
			SendSuccessEmail = Node.SendSuccessEmail();
		}
	}
}
