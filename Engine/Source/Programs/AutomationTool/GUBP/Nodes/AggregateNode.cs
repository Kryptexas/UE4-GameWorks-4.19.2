using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	class AggregateNode
	{
		public string Name;
		public GUBP.GUBPAggregateNode Node;
		public BuildNode[] Dependencies;
	}
}
