using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using UnrealBuildTool;

namespace AutomationTool
{
	[DebuggerDisplay("{Name}")]
	public abstract class BuildNodeTemplate
	{
		public string Name;
		public UnrealTargetPlatform AgentPlatform = UnrealTargetPlatform.Win64;
		public string AgentRequirements;
		public string AgentSharingGroup;
		public int FrequencyShift;
		public int AgentMemoryRequirement;
		public int TimeoutInMinutes;
		public float Priority;
		public string InputDependencyNames;
		public string OrderDependencyNames;
		public string RecipientsForFailureEmails;
		public bool AddSubmittersToFailureEmails;
		public bool SendSuccessEmail;
		public bool IsParallelAgentShareEditor;
		public bool IsSticky;
		public bool IsTest;
		public string DisplayGroupName;
		public string GameNameIfAnyForTempStorage;
		public string RootIfAnyForTempStorage;

		public abstract BuildNode Instantiate();
	}

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

		public HashSet<BuildNode> InputDependencies;
		public HashSet<BuildNode> OrderDependencies;
		public TriggerNode[] ControllingTriggers;
		public bool IsComplete;
		public string[] RecipientsForFailureEmails;
		public bool AddSubmittersToFailureEmails;
		public bool SendSuccessEmail;
		public bool IsParallelAgentShareEditor;
		public bool IsSticky;
		public bool IsTest;
		public string DisplayGroupName;
		public string GameNameIfAnyForTempStorage;
		public string RootIfAnyForTempStorage;

		public List<string> BuildProducts;

		public BuildNode(BuildNodeTemplate Template)
		{
			Name = Template.Name;
			AgentPlatform = Template.AgentPlatform;
			AgentRequirements = Template.AgentRequirements;
			AgentSharingGroup = Template.AgentSharingGroup;
			FrequencyShift = Template.FrequencyShift;
			AgentMemoryRequirement = Template.AgentMemoryRequirement;
			TimeoutInMinutes = Template.TimeoutInMinutes;
			Priority = Template.Priority;
			RecipientsForFailureEmails = Template.RecipientsForFailureEmails.Split(';');
			AddSubmittersToFailureEmails = Template.AddSubmittersToFailureEmails;
			SendSuccessEmail = Template.SendSuccessEmail;
			IsParallelAgentShareEditor = Template.IsParallelAgentShareEditor;
			IsSticky = Template.IsSticky;
			IsTest = Template.IsTest;
			DisplayGroupName = Template.DisplayGroupName;
			GameNameIfAnyForTempStorage = Template.GameNameIfAnyForTempStorage;
			RootIfAnyForTempStorage = Template.RootIfAnyForTempStorage;
		}

		public abstract void ArchiveBuildProducts(string GameNameIfAny, string StorageRootIfAny, TempStorageNodeInfo TempStorageNodeInfo, bool bLocalOnly);

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
	}

	[DebuggerDisplay("{Definition.Name}")]
	class BuildNodePair
	{
		public readonly BuildNodeTemplate Template;
		public readonly BuildNode Node;

		public BuildNodePair(BuildNodeTemplate InTemplate)
		{
			Template = InTemplate;
			Node = InTemplate.Instantiate();
		}
	}
}
