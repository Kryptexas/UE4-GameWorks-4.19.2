using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	public class TriggerNode : LegacyNode
	{
		public TriggerNode(GUBP.WaitForUserInput InNode) : base(InNode)
		{
			AddSubmittersToFailureEmails = false;
		}
	}
}
