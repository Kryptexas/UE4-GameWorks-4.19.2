using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	public abstract class AggregateNode
	{
		public string Name;
		public BuildNode[] Dependencies;

		public AggregateNode(string InName)
		{
			Name = InName;
		}

		public abstract IEnumerable<string> DependencyNames
		{
			get;
		}

		public abstract bool IsPromotableAggregate
		{
			get;
		}

		public abstract bool IsSeparatePromotable
		{
			get;
		}
	}

	public class LegacyAggregateNode : AggregateNode
	{
		public GUBP.GUBPAggregateNode Node;

		public LegacyAggregateNode(GUBP.GUBPAggregateNode InNode) : base(InNode.GetFullName())
		{
			Node = InNode;
		}

		public override IEnumerable<string> DependencyNames
		{
			get { return Node.Dependencies; }
		}

		public override bool IsPromotableAggregate
		{
			get { return Node.IsPromotableAggregate(); }
		}

		public override bool IsSeparatePromotable
		{
			get { return Node.IsSeparatePromotable(); }
		}
	}
}
