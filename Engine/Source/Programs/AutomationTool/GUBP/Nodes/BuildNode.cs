using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using UnrealBuildTool;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	public abstract class BuildNode
	{
		public readonly string Name;
		public UnrealTargetPlatform AgentPlatform = UnrealTargetPlatform.Win64;
		public string AgentRequirements;
		public string AgentSharingGroup;
		public int FrequencyShift;
		public int AgentMemoryRequirement;
		public int TimeoutInMinutes;
		public float Priority;

		public GUBP.GUBPNode Node;
		public HashSet<BuildNode> InputDependencies;
		public HashSet<BuildNode> OrderDependencies;
		public TriggerNode[] ControllingTriggers;
		public bool IsComplete;
		public string[] RecipientsForFailureEmails;
		public bool AddSubmittersToFailureEmails;
		public bool SendSuccessEmail;
		public bool IsParallelAgentShareEditor;

		public BuildNode(string InName)
		{
			Name = InName;
		}

		public abstract bool IsSticky
		{
			get;
		}

		public abstract IEnumerable<string> InputDependencyNames
		{
			get;
		}

		public abstract IEnumerable<string> OrderDependencyNames
		{
			get;
		}

		public abstract void RetrieveBuildProducts(string GameNameIfAny, string StorageRootIfAny, TempStorageNodeInfo TempStorageNodeInfo);

		public abstract void DoBuild();

		public abstract void DoFakeBuild();

		public string ControllingTriggerDotName
		{
			get { return String.Join(".", ControllingTriggers.Select(x => x.Name)); }
		}

		public bool DependsOn(BuildNode Node)
		{
			return OrderDependencies.Contains(Node);
		}

		public override string ToString()
		{
			System.Diagnostics.Trace.TraceWarning("Implicit conversion from NodeInfo to string\n{0}", Environment.StackTrace);
			return Name;
		}

		// Legacy stuff that should probably be removed

		public abstract bool IsTest
		{
			get;
		}

		public abstract string DisplayGroupName
		{
			get;
		}

		public abstract string GameNameIfAnyForTempStorage
		{
			get;
		}

		public abstract string RootIfAnyForTempStorage
		{
			get;
		}
	}

	public class LegacyBuildNode : BuildNode
	{
		GUBP Owner;

		public LegacyBuildNode(GUBP InOwner, GUBP.GUBPNode InNode) : base(InNode.GetFullName())
		{
			Owner = InOwner;
			Node = InNode;
			AgentRequirements = Node.ECAgentString();
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

		public override IEnumerable<string> InputDependencyNames
		{
			get { return Node.FullNamesOfDependencies; }
		}

		public override IEnumerable<string> OrderDependencyNames
		{
			get { return Node.FullNamesOfPseudodependencies; }
		}

		public override void RetrieveBuildProducts(string GameNameIfAny, string StorageRootIfAny, TempStorageNodeInfo TempStorageNodeInfo)
		{
			if (Name == GUBP.VersionFilesNode.StaticGetFullName() && !CommandUtils.IsBuildMachine)
			{
				CommandUtils.LogConsole("***** NOT ****** Retrieving GUBP Node {0} from {1}; it is the version files.", Name, TempStorageNodeInfo.GetRelativeDirectory());
				Node.BuildProducts = new List<string>();

			}
			else
			{
				CommandUtils.LogConsole("***** Retrieving GUBP Node {0} from {1}", Name, TempStorageNodeInfo.GetRelativeDirectory());
				bool WasLocal;
				try
				{
					Node.BuildProducts = TempStorage.RetrieveFromTempStorage(TempStorageNodeInfo, out WasLocal, GameNameIfAny, StorageRootIfAny);
				}
				catch (Exception Ex)
				{
					if (GameNameIfAny != "")
					{
						Node.BuildProducts = TempStorage.RetrieveFromTempStorage(TempStorageNodeInfo, out WasLocal, "", StorageRootIfAny);
					}
					else
					{
						throw new AutomationException(Ex, "Build Products cannot be found for node {0}", Name);
					}
				}
				if (!WasLocal)
				{
					Node.PostLoadFromSharedTempStorage(Owner);
				}
			}
		}

		public override void DoBuild()
		{
			Node.AllDependencyBuildProducts = new List<string>();
			foreach (BuildNode InputDependency in InputDependencies)
			{
				foreach (string BuildProduct in InputDependency.Node.BuildProducts)
				{
					Node.AddDependentBuildProduct(BuildProduct);
				}
			}

			Node.DoBuild(Owner);
		}

		public override void DoFakeBuild()
		{
			Node.DoFakeBuild(Owner);
		}

		public override bool IsTest
		{
			get { return Node.IsTest(); }
		}

		public override string DisplayGroupName
		{
			get { return Node.GetDisplayGroupName(); }
		}

		public override string GameNameIfAnyForTempStorage
		{
			get { return Node.GameNameIfAnyForTempStorage(); }
		}

		public override string RootIfAnyForTempStorage
		{
			get { return Node.RootIfAnyForTempStorage(); }
		}
	}
}
