using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using UnrealBuildTool;

namespace AutomationTool
{
	public class LegacyNodeTemplate : BuildNodeTemplate
	{
		public GUBP Owner;
		public GUBP.GUBPNode Node;

		public LegacyNodeTemplate(GUBP InOwner, GUBP.GUBPNode InNode)
		{
			Owner = InOwner;
			Node = InNode;

			SetStandardProperties(InNode, this);
		}

		public static void SetStandardProperties(GUBP.GUBPNode Node, BuildNodeTemplate Definition)
		{
			Definition.Name = Node.GetFullName();
			Definition.AgentRequirements = Node.ECAgentString();
			Definition.AgentSharingGroup = Node.AgentSharingGroup;
			Definition.AgentMemoryRequirement = Node.AgentMemoryRequirement();
			Definition.TimeoutInMinutes = Node.TimeoutInMinutes();
			Definition.SendSuccessEmail = Node.SendSuccessEmail();
			Definition.Priority = Node.Priority();
			Definition.IsSticky = Node.IsSticky();
			Definition.InputDependencyNames = String.Join(";", Node.FullNamesOfDependencies);
			Definition.OrderDependencyNames = String.Join(";", Node.FullNamesOfPseudodependencies);
			Definition.IsTest = Node.IsTest();
			Definition.DisplayGroupName = Node.GetDisplayGroupName();
			Definition.GameNameIfAnyForTempStorage = Node.GameNameIfAnyForTempStorage();
			Definition.RootIfAnyForTempStorage = Node.RootIfAnyForTempStorage();
		}

		public override BuildNode Instantiate()
		{
			return new LegacyNode(this);
		}
	}

	public class LegacyNode : BuildNode
	{
		GUBP Owner;
		GUBP.GUBPNode Node;

		public LegacyNode(LegacyNodeTemplate Template)
			: base(Template)
		{
			Owner = Template.Owner;
			Node = Template.Node;
		}

		public override void ArchiveBuildProducts(string GameNameIfAny, string StorageRootIfAny, TempStorageNodeInfo TempStorageNodeInfo, bool bLocalOnly)
		{
			TempStorage.StoreToTempStorage(TempStorageNodeInfo, BuildProducts, bLocalOnly, GameNameIfAny, StorageRootIfAny);
		}

		public override void RetrieveBuildProducts(string GameNameIfAny, string StorageRootIfAny, TempStorageNodeInfo TempStorageNodeInfo)
		{
			CommandUtils.LogConsole("***** Retrieving GUBP Node {0} from {1}", Name, TempStorageNodeInfo.GetRelativeDirectory());
			bool WasLocal;
			try
			{
				BuildProducts = TempStorage.RetrieveFromTempStorage(TempStorageNodeInfo, out WasLocal, GameNameIfAny, StorageRootIfAny);
			}
			catch (Exception Ex)
			{
				if (GameNameIfAny != "")
				{
					BuildProducts = TempStorage.RetrieveFromTempStorage(TempStorageNodeInfo, out WasLocal, "", StorageRootIfAny);
				}
				else
				{
					throw new AutomationException(Ex, "Build Products cannot be found for node {0}", Name);
				}
			}
			Node.BuildProducts = BuildProducts;
		}

		public override void DoBuild()
		{
			Node.AllDependencyBuildProducts = new List<string>();
			foreach (BuildNode InputDependency in InputDependencies)
			{
				foreach (string BuildProduct in InputDependency.BuildProducts)
				{
					Node.AddDependentBuildProduct(BuildProduct);
				}
			}

			Node.DoBuild(Owner);
			BuildProducts = Node.BuildProducts;
		}

		public override void DoFakeBuild()
		{
			Node.DoFakeBuild(Owner);
			BuildProducts = Node.BuildProducts;
		}
	}
}
