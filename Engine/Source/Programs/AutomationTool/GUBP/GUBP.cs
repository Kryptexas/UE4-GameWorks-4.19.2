// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using System.Reflection;
using System.Xml;
using System.Linq;
using System.Diagnostics;

[Help("Runs one, several or all of the GUBP nodes")]
[Help(typeof(UE4Build))]
[Help("NoMac", "Toggle to exclude the Mac host platform, default is Win64+Mac+Linux")]
[Help("NoLinux", "Toggle to exclude the Linux (PC, 64-bit) host platform, default is Win64+Mac+Linux")]
[Help("NoPC", "Toggle to exclude the PC host platform, default is Win64+Mac+Linux")]
[Help("CleanLocal", "delete the local temp storage before we start")]
[Help("Store=", "Sets the name of the temp storage block, normally, this is built for you.")]
[Help("StoreSuffix=", "Tacked onto a store name constructed from CL, branch, etc")]
[Help("TimeIndex=", "An integer used to determine subsets to run based on DependentCISFrequencyQuantumShift")]
[Help("UserTimeIndex=", "An integer used to determine subsets to run based on DependentCISFrequencyQuantumShift, this one overrides TimeIndex")]
[Help("PreflightUID=", "A unique integer tag from EC used as part of the tempstorage, builds and label names to distinguish multiple attempts.")]
[Help("Node=", "Nodes to process, -node=Node1+Node2+Node3, if no nodes or games are specified, defaults to all nodes.")]
[Help("TriggerNode=", "Trigger Nodes to process, -triggernode=Node.")]
[Help("Game=", "Games to process, -game=Game1+Game2+Game3, if no games or nodes are specified, defaults to all nodes.")]
[Help("ListOnly", "List Nodes in this branch")]
[Help("SaveGraph", "Save graph as an xml file")]
[Help("CommanderJobSetupOnly", "Set up the EC branch info via ectool and quit")]
[Help("FakeEC", "don't run ectool, rather just do it locally, emulating what EC would have done.")]
[Help("Fake", "Don't actually build anything, just store a record of success as the build product for each node.")]
[Help("AllPlatforms", "Regardless of what is installed on this machine, set up the graph for all platforms; true by default on build machines.")]
[Help("SkipTriggers", "ignore all triggers")]
[Help("CL", "force the CL to something, disregarding the P4 value.")]
[Help("History", "Like ListOnly, except gives you a full history. Must have -P4 for this to work.")]
[Help("Changes", "Like history, but also shows the P4 changes. Must have -P4 for this to work.")]
[Help("AllChanges", "Like changes except includes changes before the last green. Must have -P4 for this to work.")]
[Help("EmailOnly", "Only emails the folks given in the argument.")]
[Help("AddEmails", "Add these space delimited emails too all email lists.")]
[Help("ShowDependencies", "Show node dependencies.")]
[Help("ShowECDependencies", "Show EC node dependencies instead.")]
[Help("ShowECProc", "Show EC proc names.")]
[Help("ECProject", "From EC, the name of the project, used to get a version number.")]
[Help("CIS", "This is a CIS run, assign TimeIndex based on the history.")]
[Help("ForceIncrementalCompile", "make sure all compiles are incremental")]
[Help("AutomatedTesting", "Allow automated testing, currently disabled.")]
[Help("StompCheck", "Look for stomped build products.")]
public partial class GUBP : BuildCommand
{
	const string StartedTempStorageSuffix = "_Started";
	const string FailedTempStorageSuffix = "_Failed";
	const string SucceededTempStorageSuffix = "_Succeeded";

	class NodeHistory
	{
		public int LastSucceeded = 0;
		public int LastFailed = 0;
		public List<int> InProgress = new List<int>();
		public string InProgressString = "";
		public List<int> Failed = new List<int>();
		public string FailedString = "";
		public List<int> AllStarted = new List<int>();
		public List<int> AllSucceeded = new List<int>();
		public List<int> AllFailed = new List<int>();
	};

	/// <summary>
	/// Main entry point for GUBP
	/// </summary>
    public override void ExecuteBuild()
    {
        Log("************************* GUBP");

		bool bPreflightBuild = false;
		int PreflightShelveCL = 0;
        string PreflightShelveCLString = GetEnvVar("uebp_PreflightShelveCL");
        if ((!String.IsNullOrEmpty(PreflightShelveCLString) && IsBuildMachine) || ParseParam("PreflightTest"))
        {
            Log("**** Preflight shelve {0}", PreflightShelveCLString);
            if (!String.IsNullOrEmpty(PreflightShelveCLString))
            {
                PreflightShelveCL = int.Parse(PreflightShelveCLString);
                if (PreflightShelveCL < 2000000)
                {
                    throw new AutomationException(String.Format( "{0} does not look like a CL", PreflightShelveCL));
                }
            }
            bPreflightBuild = true;
        }
        
		List<UnrealTargetPlatform> HostPlatforms = new List<UnrealTargetPlatform>();
        if (!ParseParam("NoPC"))
        {
            HostPlatforms.Add(UnrealTargetPlatform.Win64);
        }
        if (!ParseParam("NoMac"))
        {
			HostPlatforms.Add(UnrealTargetPlatform.Mac);
        }
		if(!ParseParam("NoLinux") && UnrealBuildTool.BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Linux)
		{
			HostPlatforms.Add(UnrealTargetPlatform.Linux);
		}

        string StoreName = ParseParamValue("Store");
        string StoreSuffix = ParseParamValue("StoreSuffix", "");

		string PreflightMangleSuffix = "";
        if (bPreflightBuild)
        {
            int PreflightUID = ParseParamInt("PreflightUID", 0);
            PreflightMangleSuffix = String.Format("-PF-{0}-{1}", PreflightShelveCL, PreflightUID);
            StoreSuffix = StoreSuffix + PreflightMangleSuffix;
        }
        int CL = ParseParamInt("CL", 0);
        bool bCleanLocalTempStorage = ParseParam("CleanLocal");
        bool bSkipTriggers = ParseParam("SkipTriggers");
        bool bFake = ParseParam("fake");
        bool bFakeEC = ParseParam("FakeEC");

        bool bSaveSharedTempStorage = false;

        bool LocalOnly = true;
        string CLString = "";
        if (String.IsNullOrEmpty(StoreName))
        {
            if (P4Enabled)
            {
                if (CL == 0)
                {
                    CL = P4Env.Changelist;
                }
                CLString = String.Format("{0}", CL);
                StoreName = P4Env.BuildRootEscaped + "-" + CLString;
                bSaveSharedTempStorage = CommandUtils.IsBuildMachine;
                LocalOnly = false;
            }
            else
            {
                StoreName = "TempLocal";
                bSaveSharedTempStorage = false;
            }
        }
        StoreName = StoreName + StoreSuffix;
        if (bFakeEC)
        {
            LocalOnly = true;
        } 
        if (bSaveSharedTempStorage)
        {
            if (!TempStorage.HaveSharedTempStorage(true))
            {
                throw new AutomationException("Request to save to temp storage, but {0} is unavailable.", TempStorage.UE4TempStorageDirectory());
            }
        }
        else if (!LocalOnly && !TempStorage.HaveSharedTempStorage(false))
        {
            LogWarning("Looks like we want to use shared temp storage, but since we don't have it, we won't use it.");
            LocalOnly = true;
        }

        bool CommanderSetup = ParseParam("CommanderJobSetupOnly");

		string ExplicitTriggerName = "";
		if (CommanderSetup)
		{
			ExplicitTriggerName = ParseParamValue("TriggerNode", "");
		}

		List<BuildNode> AllNodes;
		List<AggregateNode> AllAggregates;
		int TimeQuantum = 20;
		AddNodesForBranch(CL, HostPlatforms, bPreflightBuild, PreflightMangleSuffix, out AllNodes, out AllAggregates, ref TimeQuantum);

		LinkGraph(AllAggregates, AllNodes);
		FindControllingTriggers(AllNodes);
		FindCompletionState(AllNodes, StoreName, LocalOnly);
		ComputeDependentFrequencies(AllNodes);

        if (bCleanLocalTempStorage)  // shared temp storage can never be wiped
        {
            TempStorage.DeleteLocalTempStorageManifests(CmdEnv);
        }

		int TimeIndex = ParseParamInt("TimeIndex", 0);
		if (TimeIndex == 0)
		{
			TimeIndex = ParseParamInt("UserTimeIndex", 0);
		}
		if (ParseParam("CIS") && ExplicitTriggerName == "" && CommanderSetup) // explicit triggers will already have a time index assigned
		{
			TimeIndex = UpdateCISCounter(TimeQuantum);
			Log("Setting TimeIndex to {0}", TimeIndex);
		}

		HashSet<BuildNode> NodesToDo = ParseNodesToDo(AllNodes, AllAggregates);
		CullNodesForTimeIndex(NodesToDo, TimeIndex);
		CullNodesForPreflight(NodesToDo, bPreflightBuild);

		TriggerNode ExplicitTrigger = null;
		if (CommanderSetup)
        {
            if (!String.IsNullOrEmpty(ExplicitTriggerName))
            {
                foreach (TriggerNode Node in AllNodes.OfType<TriggerNode>())
                {
                    if (Node.Name.Equals(ExplicitTriggerName, StringComparison.InvariantCultureIgnoreCase))
                    {
                        Node.Node.SetAsExplicitTrigger();
						ExplicitTrigger = Node;
                        break;
                    }
                }
                if (ExplicitTrigger == null)
                {
                    throw new AutomationException("Could not find trigger node named {0}", ExplicitTriggerName);
                }
            }
            else
            {
                if (bSkipTriggers)
                {
                    foreach (TriggerNode Node in AllNodes.OfType<TriggerNode>())
                    {
                        Node.Node.SetAsExplicitTrigger();
                    }
                }
            }
        }

        List<BuildNode> OrderedToDo = TopologicalSort(NodesToDo, ExplicitTrigger, false, false);

		List<TriggerNode> UnfinishedTriggers = FindUnfinishedTriggers(bSkipTriggers, ExplicitTrigger, OrderedToDo);

		PrintNodes(this, OrderedToDo, AllAggregates, UnfinishedTriggers, TimeQuantum);

        //check sorting
		CheckSortOrder(OrderedToDo);

		ElectricCommander EC = new ElectricCommander(this);

		string ShowHistoryParam = ParseParamValue("ShowHistory", null);
		if(ShowHistoryParam != null)
		{
			BuildNode Node = AllNodes.FirstOrDefault(x => x.Name.Equals(ShowHistoryParam, StringComparison.InvariantCultureIgnoreCase));
			if(Node == null)
			{
				throw new AutomationException("Couldn't find node {0}", ShowHistoryParam);
			}

			NodeHistory History = FindNodeHistory(Node, CLString, StoreName);
			if(History == null)
			{
				throw new AutomationException("Couldn't get history for {0}", ShowHistoryParam);
			}

			PrintDetailedChanges(History, P4Env.Changelist);
		}
		else
		{
			string FakeFail = ParseParamValue("FakeFail");
			if(CommanderSetup)
			{
				DoCommanderSetup(EC, AllNodes, AllAggregates, OrderedToDo, TimeIndex, TimeQuantum, bSkipTriggers, bFake, bFakeEC, CLString, ExplicitTrigger, UnfinishedTriggers, FakeFail, bPreflightBuild);
			}
			else if(ParseParam("SaveGraph"))
			{
				SaveGraphVisualization(OrderedToDo);
			}
			else if(!ParseParam("ListOnly"))
			{
				ExecuteNodes(EC, OrderedToDo, bFake, bFakeEC, bSaveSharedTempStorage, CLString, StoreName, FakeFail);
			}
		}
		PrintRunTime();
	}

	/// <summary>
	/// Recursively update the ControllingTriggers array for each of the nodes passed in. 
	/// </summary>
	/// <param name="NodesToDo">Nodes to update</param>
	static void FindControllingTriggers(IEnumerable<BuildNode> NodesToDo)
	{
		foreach(BuildNode NodeToDo in NodesToDo)
		{
			FindControllingTriggers(NodeToDo);
		}
	}

	/// <summary>
	/// Recursively find the controlling triggers for the given node.
	/// </summary>
	/// <param name="NodeToDo">Node to find the controlling triggers for</param>
    static void FindControllingTriggers(BuildNode NodeToDo)
    {
		if(NodeToDo.ControllingTriggers == null)
		{
			NodeToDo.ControllingTriggers = new TriggerNode[0];

			// Find all the dependencies of this node
			List<BuildNode> AllDependencies = new List<BuildNode>();
			AllDependencies.AddRange(NodeToDo.Dependencies);
			AllDependencies.AddRange(NodeToDo.PseudoDependencies);

			// Find the immediate trigger controlling this one
			List<TriggerNode> PreviousTriggers = new List<TriggerNode>();
			foreach (BuildNode Dependency in AllDependencies)
			{
				FindControllingTriggers(Dependency);

				TriggerNode PreviousTrigger = Dependency as TriggerNode;
				if(PreviousTrigger == null && Dependency.ControllingTriggers.Length > 0)
				{
					PreviousTrigger = Dependency.ControllingTriggers.Last();
				}
				
				if(PreviousTrigger != null && !PreviousTriggers.Contains(PreviousTrigger))
				{
					PreviousTriggers.Add(PreviousTrigger);
				}
			}

			// Remove previous triggers from the list that aren't the last in the chain. If there's a trigger chain of X.Y.Z, and a node has dependencies behind all three, the only trigger we care about is Z.
			PreviousTriggers.RemoveAll(x => PreviousTriggers.Any(y => y.ControllingTriggers.Contains(x)));

			// We only support one direct controlling trigger at the moment (though it may be in a chain with other triggers)
			if(PreviousTriggers.Count > 1)
			{
				throw new AutomationException("Node {0} has multiple controlling triggers: {1}", NodeToDo.Name, String.Join(", ", PreviousTriggers.Select(x => x.Name)));
			}

			// Update the list of controlling triggers
			if (PreviousTriggers.Count == 1)
			{
				List<TriggerNode> ControllingTriggers = new List<TriggerNode>();
				ControllingTriggers.AddRange(PreviousTriggers[0].ControllingTriggers);
				ControllingTriggers.Add(PreviousTriggers[0]);
				NodeToDo.ControllingTriggers = ControllingTriggers.ToArray();
			}
		}
	}

	/// <summary>
	/// Get a string describing a time interval, in the format "XhYm".
	/// </summary>
	/// <param name="Minutes">The time interval, in minutes</param>
	/// <returns>String for how frequently the node should be built</returns>
    static string GetTimeIntervalString(int Minutes)
    {
        if (Minutes < 60)
        {
            return String.Format("{0}m", Minutes);
        }
        else
        {
            return String.Format("{0}h{1}m", Minutes / 60, Minutes % 60);
        }
    }

	/// <summary>
	/// Update the frequency shift for all build nodes to ensure that each node's frequency is at least that of its dependencies.
	/// </summary>
	/// <param name="Nodes">Sequence of nodes to compute frequencies for</param>
	static void ComputeDependentFrequencies(IEnumerable<BuildNode> Nodes)
	{
		foreach(BuildNode Node in Nodes)
		{
			foreach(BuildNode IndirectDependency in Node.AllIndirectDependencies)
			{
				Node.FrequencyShift = Math.Max(Node.FrequencyShift, IndirectDependency.FrequencyShift);
			}
		}
	}

	static void FindCompletionState(IEnumerable<BuildNode> NodesToDo, string StoreName, bool LocalOnly)
	{
		foreach(BuildNode NodeToDo in NodesToDo)
		{
			string NodeStoreName = StoreName + "-" + NodeToDo.Name;
			string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();

			if (LocalOnly)
			{
				NodeToDo.IsComplete = TempStorage.LocalTempStorageExists(CmdEnv, NodeStoreName, bQuiet : true);
			}
			else
			{
				NodeToDo.IsComplete = TempStorage.TempStorageExists(CmdEnv, NodeStoreName, GameNameIfAny, bQuiet: true);
				if(GameNameIfAny != "" && !NodeToDo.IsComplete)
				{
					NodeToDo.IsComplete = TempStorage.TempStorageExists(CmdEnv, NodeStoreName, "", bQuiet: true);
				}
			}

            LogVerbose("** {0}", NodeToDo.Name);
			if (!NodeToDo.IsComplete)
			{
				LogVerbose("***** GUBP Trigger Node was already triggered {0} -> {1} : {2}", NodeToDo.Name, GameNameIfAny, NodeStoreName);
			}
			else
			{
				LogVerbose("***** GUBP Trigger Node was NOT yet triggered {0} -> {1} : {2}", NodeToDo.Name, GameNameIfAny, NodeStoreName);
			}
		}
    }

	/// <summary>
	/// Finds all the source code changes between the given range.
	/// </summary>
	/// <param name="MinimumCL">The minimum (inclusive) changelist to include</param>
	/// <param name="MaximumCL">The maximum (inclusive) changelist to include</param>
	/// <returns>Lists of changelist records in the given range</returns>
    static List<P4Connection.ChangeRecord> GetSourceChangeRecords(int MinimumCL, int MaximumCL)
    {
        if (MinimumCL < 1990000)
        {
            throw new AutomationException("That CL looks pretty far off {0}", MinimumCL);
		}

		// Query the changes from Perforce
		StringBuilder FileSpec = new StringBuilder();
		FileSpec.AppendFormat("{0}@{1},{2} ", CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "...", "Source", "..."), MinimumCL, MaximumCL);
		FileSpec.AppendFormat("{0}@{1},{2} ", CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "...", "Build", "..."), MinimumCL, MaximumCL);

        List<P4Connection.ChangeRecord> ChangeRecords;
		if (!P4.Changes(out ChangeRecords, FileSpec.ToString(), false, true, LongComment: true))
        {
            throw new AutomationException("Could not get changes; cmdline: p4 changes {0}", FileSpec.ToString());
		}

		// Filter out all the changes by the buildmachine user; the CIS counter or promoted builds aren't of interest to us.
		ChangeRecords.RemoveAll(x => x.User.Equals("buildmachine", StringComparison.InvariantCultureIgnoreCase));
		return ChangeRecords;
    }

	/// <summary>
	/// Print a list of source changes, along with the success state for other builds of this node.
	/// </summary>
	/// <param name="History">History for this node</param>
    static void PrintDetailedChanges(NodeHistory History, int CurrentCL)
    {
        DateTime StartTime = DateTime.UtcNow;

		// Find all the changelists that we're interested in
		SortedSet<int> BuildChanges = new SortedSet<int>();
		BuildChanges.UnionWith(History.AllStarted.Where(x => x >= History.LastSucceeded));
		BuildChanges.Add(CurrentCL);

		// Find all the changelists that we're interested in
        List<P4Connection.ChangeRecord> ChangeRecords = GetSourceChangeRecords(BuildChanges.First(), BuildChanges.Last());

		// Print all the changes in the set
		int LastCL = BuildChanges.First();
		foreach(int BuildCL in BuildChanges)
		{
			// Show all the changes in this range
			foreach(P4Connection.ChangeRecord Record in ChangeRecords.Where(x => x.CL > LastCL && x.CL <= BuildCL))
			{
				string[] Lines = Record.Summary.Split(new char[]{ '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
				Log("             {0} {1} {2}", Record.CL, Record.UserEmail, (Lines.Length > 0)? Lines[0] : "");
			}

			// Show the status of this build
			string BuildStatus;
			if(BuildCL == CurrentCL)
			{
				BuildStatus = "this sync";
			}
            else if (History.AllSucceeded.Contains(BuildCL))
            {
				BuildStatus = "built";
            }
            else if (History.AllFailed.Contains(BuildCL))
            {
				BuildStatus = "FAILED";
            }
			else
			{
				BuildStatus = "running";
			}
			Log(" {0} {1} {2}", (BuildCL == CurrentCL)? ">>>>" : "    ", BuildCL, BuildStatus.ToString());

			// Update the last CL now that we've output this one
			LastCL = BuildCL;
		}

        Log("Took {0:0.0}s to get P4 history", (DateTime.UtcNow - StartTime).TotalSeconds);
    }

    void PrintNodes(GUBP bp, List<BuildNode> Nodes, IEnumerable<AggregateNode> Aggregates, List<TriggerNode> UnfinishedTriggers, int TimeQuantum)
    {
		AggregateNode[] MatchingAggregates = Aggregates.Where(x => x.Dependencies.All(y => Nodes.Contains(y))).ToArray();
		if (MatchingAggregates.Length > 0)
		{
			Log("*********** Aggregates");
			foreach (AggregateNode Aggregate in MatchingAggregates.OrderBy(x => x.Name))
			{
				StringBuilder Note = new StringBuilder("    " + Aggregate.Name);
				if (Aggregate.Node.IsPromotableAggregate())
				{
					Note.Append(" (promotable)");
				}
				Log(Note.ToString());
			}
		}

        bool bShowDependencies = bp.ParseParam("ShowDependencies");
        bool AddEmailProps = bp.ParseParam("ShowEmails");
        bool ECProc = bp.ParseParam("ShowECProc");
        string LastControllingTrigger = "";
        string LastAgentGroup = "";

        Log("*********** Desired And Dependent Nodes, in order.");
        foreach (BuildNode NodeToDo in Nodes)
        {
            string MyControllingTrigger = NodeToDo.ControllingTriggerDotName;
            if (MyControllingTrigger != LastControllingTrigger)
            {
                LastControllingTrigger = MyControllingTrigger;
                if (MyControllingTrigger != "")
                {
                    string Finished = "";
                    if (UnfinishedTriggers != null)
                    {
                        if (NodeToDo.ControllingTriggers.Length > 0 && UnfinishedTriggers.Contains(NodeToDo.ControllingTriggers.Last()))
                        {
                            Finished = "(not yet triggered)";
                        }
                        else
                        {
                            Finished = "(already triggered)";
                        }
                    }
                    Log("  Controlling Trigger: {0}    {1}", MyControllingTrigger, Finished);
                }
            }

			if (NodeToDo.AgentSharingGroup != LastAgentGroup && NodeToDo.AgentSharingGroup != "")
            {
                Log("    Agent Group: {0}", NodeToDo.AgentSharingGroup);
            }
            LastAgentGroup = NodeToDo.AgentSharingGroup;

			StringBuilder Builder = new StringBuilder("      ");
			if(LastAgentGroup != "")
			{
				Builder.Append("  ");
			}
			Builder.AppendFormat("{0} ({1})", NodeToDo.Name, GetTimeIntervalString(TimeQuantum << NodeToDo.FrequencyShift));
			if(NodeToDo.IsComplete)
			{
				Builder.Append(" - (Completed)");
			}
			if(NodeToDo is TriggerNode)
			{
				Builder.Append(" - (TriggerNode)");
			}
			if(NodeToDo.IsSticky)
			{
				Builder.Append(" - (Sticky)");
			}

            string Agent = NodeToDo.Node.ECAgentString();
			if(ParseParamValue("AgentOverride") != "" && !NodeToDo.Node.GetFullName().Contains("Mac"))
			{
				Agent = ParseParamValue("AgentOverride");
			}
            if (!String.IsNullOrEmpty(Agent))
            {
				Builder.AppendFormat(" [{0}]", Agent);
            }
			if(NodeToDo.AgentMemoryRequirement != 0)
			{
				Builder.AppendFormat(" [{0}gb]", NodeToDo.AgentMemoryRequirement);
			}
            if (AddEmailProps)
            {
				Builder.AppendFormat(" {0}", String.Join(" ", NodeToDo.RecipientsForFailureEmails));
            }
			if(ECProc)
			{
				Builder.AppendFormat("  {0}", NodeToDo.Node.ECProcedure());
			}
			Log(Builder.ToString());

            if (bShowDependencies)
            {
                foreach (BuildNode Dep in NodeToDo.Dependencies)
                {
                    Log("            dep> {0}", Dep.Name);
                }
                foreach (BuildNode Dep in NodeToDo.PseudoDependencies)
                {
                    Log("           pdep> {0}", Dep.Name);
                }
            }
        }
    }

	/// <summary>
	/// Exports the build graph as a GEXF file, for visualization in an external tool (eg. Gephi).
	/// </summary>
	/// <param name="Nodes">The nodes in the graph</param>
	static void SaveGraphVisualization(IEnumerable<BuildNode> Nodes)
	{
		// Create a graph node for each GUBP node in the graph
		List<GraphNode> GraphNodes = new List<GraphNode>();
		Dictionary<BuildNode, GraphNode> NodeToGraphNodeMap = new Dictionary<BuildNode, GraphNode>();
		foreach(BuildNode Node in Nodes)
		{
			GraphNode GraphNode = new GraphNode();
			GraphNode.Id = NodeToGraphNodeMap.Count;
			GraphNode.Label = Node.Name;

			NodeToGraphNodeMap.Add(Node, GraphNode);
			GraphNodes.Add(GraphNode);
		}

		// Connect everything together
		List<GraphEdge> GraphEdges = new List<GraphEdge>();
		foreach(KeyValuePair<BuildNode, GraphNode> NodeToGraphNodePair in NodeToGraphNodeMap)
		{
			foreach (BuildNode Dependency in NodeToGraphNodePair.Key.Dependencies)
			{
				GraphNode PrerequisiteFileGraphNode;
				if (NodeToGraphNodeMap.TryGetValue(Dependency, out PrerequisiteFileGraphNode))
				{
					// Connect a file our action is dependent on, to our action itself
					GraphEdge NewGraphEdge = new GraphEdge();

					NewGraphEdge.Id = GraphEdges.Count;
					NewGraphEdge.Source = PrerequisiteFileGraphNode;
					NewGraphEdge.Target = NodeToGraphNodePair.Value;
					NewGraphEdge.Color = new GraphColor() { R = 0.0f, G = 0.0f, B = 0.0f, A = 0.75f };

					GraphEdges.Add(NewGraphEdge);
				}

			}
			foreach (BuildNode Dependency in NodeToGraphNodePair.Key.PseudoDependencies)
			{
				GraphNode PrerequisiteFileGraphNode;
				if (NodeToGraphNodeMap.TryGetValue(Dependency, out PrerequisiteFileGraphNode))
				{
					// Connect a file our action is dependent on, to our action itself
					GraphEdge NewGraphEdge = new GraphEdge();

					NewGraphEdge.Id = GraphEdges.Count;
					NewGraphEdge.Source = PrerequisiteFileGraphNode;
					NewGraphEdge.Target = NodeToGraphNodePair.Value;
					NewGraphEdge.Color = new GraphColor() { R = 0.0f, G = 0.0f, B = 0.0f, A = 0.25f };

					GraphEdges.Add(NewGraphEdge);
				}

			}
		}

		// Export the graph definition
		string Filename = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "GubpGraph.gexf");
		Log("Writing graph to {0}", Filename);
		GraphVisualization.WriteGraphFile(Filename, "GUBP Nodes", NodeToGraphNodeMap.Values.ToList(), GraphEdges);
		Log("Wrote graph to {0}", Filename);
	 }

    static List<int> ConvertCLToIntList(List<string> Strings)
    {
        List<int> Result = new List<int>();
        foreach (string ThisString in Strings)
        {
            int ThisInt = int.Parse(ThisString);
            if (ThisInt < 1960000 || ThisInt > 3000000)
            {
                Log("CL {0} appears to be out of range", ThisInt);
            }
            Result.Add(ThisInt);
        }
        Result.Sort();
        return Result;
    }

    int CountZeros(int Num)
    {
        if (Num < 0)
        {
            throw new AutomationException("Bad CountZeros");
        }
        if (Num == 0)
        {
            return 31;
        }
        int Result = 0;
        while ((Num & 1) == 0)
        {
            Result++;
            Num >>= 1;
        }
        return Result;
    }

    static List<BuildNode> TopologicalSort(HashSet<BuildNode> NodesToDo, BuildNode ExplicitTrigger, bool SubSort, bool DoNotConsiderCompletion)
    {
        DateTime StartTime = DateTime.UtcNow;

        List<BuildNode> OrdereredToDo = new List<BuildNode>();

        Dictionary<string, List<BuildNode>> SortedAgentGroupChains = new Dictionary<string, List<BuildNode>>();
        if (!SubSort)
        {
            Dictionary<string, List<BuildNode>> AgentGroupChains = new Dictionary<string, List<BuildNode>>();
            foreach (BuildNode NodeToDo in NodesToDo)
            {
                string MyAgentGroup = NodeToDo.AgentSharingGroup;
                if (MyAgentGroup != "")
                {
                    if (!AgentGroupChains.ContainsKey(MyAgentGroup))
                    {
                        AgentGroupChains.Add(MyAgentGroup, new List<BuildNode> { NodeToDo });
                    }
                    else
                    {
                        AgentGroupChains[MyAgentGroup].Add(NodeToDo);
                    }
                }
            }
            foreach (KeyValuePair<string, List<BuildNode>> Chain in AgentGroupChains)
            {
                SortedAgentGroupChains.Add(Chain.Key, TopologicalSort(new HashSet<BuildNode>(Chain.Value), ExplicitTrigger, true, DoNotConsiderCompletion));
            }
			foreach(KeyValuePair<string, List<BuildNode>> Chain in SortedAgentGroupChains)
			{
				string[] ControllingTriggers = Chain.Value.Select(x => x.ControllingTriggerDotName).Distinct().OrderBy(x => x).ToArray();
				if(ControllingTriggers.Length > 1)
				{
					string Triggers = String.Join(", ", ControllingTriggers.Select(x => String.Format("'{0}' ({1})", x, String.Join("+", Chain.Value.Where(y => y.ControllingTriggerDotName == x).Select(y => y.Name)))));
					throw new AutomationException("Agent sharing group '{0}' has multiple controlling triggers: {1}", Chain.Key, Triggers);
				}
			}
            Log("***************Done with recursion");
        }

        // here we do a topological sort of the nodes, subject to a lexographical and priority sort
        while (NodesToDo.Count > 0)
        {
            bool bProgressMade = false;
            float BestPriority = -1E20f;
            BuildNode BestNode = null;
            bool BestPseudoReady = false;
            HashSet<string> NonReadyAgentGroups = new HashSet<string>();
            HashSet<string> NonPeudoReadyAgentGroups = new HashSet<string>();
            HashSet<string> ExaminedAgentGroups = new HashSet<string>();
            foreach (BuildNode NodeToDo in NodesToDo)
            {
                bool bReady = true;
				bool bPseudoReady = true;
				bool bCompReady = true;
                if (!SubSort && NodeToDo.AgentSharingGroup != "")
                {
                    if (ExaminedAgentGroups.Contains(NodeToDo.AgentSharingGroup))
                    {
                        bReady = !NonReadyAgentGroups.Contains(NodeToDo.AgentSharingGroup);
                        bPseudoReady = !NonPeudoReadyAgentGroups.Contains(NodeToDo.AgentSharingGroup); //this might not be accurate if bReady==false
                    }
                    else
                    {
                        ExaminedAgentGroups.Add(NodeToDo.AgentSharingGroup);
                        foreach (BuildNode ChainNode in SortedAgentGroupChains[NodeToDo.AgentSharingGroup])
                        {
                            foreach (BuildNode Dep in ChainNode.Dependencies)
                            {
                                if (!SortedAgentGroupChains[NodeToDo.AgentSharingGroup].Contains(Dep) && NodesToDo.Contains(Dep))
                                {
                                    bReady = false;
                                    break;
                                }
                            }
                            if (!bReady)
                            {
                                NonReadyAgentGroups.Add(NodeToDo.AgentSharingGroup);
                                break;
                            }
                            foreach (BuildNode Dep in ChainNode.PseudoDependencies)
                            {
                                if (!SortedAgentGroupChains[NodeToDo.AgentSharingGroup].Contains(Dep) && NodesToDo.Contains(Dep))
                                {
                                    bPseudoReady = false;
                                    NonPeudoReadyAgentGroups.Add(NodeToDo.AgentSharingGroup);
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    foreach (BuildNode Dep in NodeToDo.Dependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            bReady = false;
                            break;
                        }
                    }
                    foreach (BuildNode Dep in NodeToDo.PseudoDependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            bPseudoReady = false;
                            break;
                        }
                    }
                }
                float Priority = NodeToDo.Priority;

                if (bReady && BestNode != null)
                {
                    if (String.Compare(BestNode.ControllingTriggerDotName, NodeToDo.ControllingTriggerDotName) < 0) //sorted by controlling trigger
                    {
                        bReady = false;
                    }
                    else if (String.Compare(BestNode.ControllingTriggerDotName, NodeToDo.ControllingTriggerDotName) == 0) //sorted by controlling trigger
                    {
                        if (BestNode.IsSticky && !NodeToDo.IsSticky) //sticky nodes first
                        {
                            bReady = false;
                        }
                        else if (BestNode.IsSticky == NodeToDo.IsSticky)
                        {
                            if (BestPseudoReady && !bPseudoReady)
                            {
                                bReady = false;
                            }
                            else if (BestPseudoReady == bPseudoReady)
                            {
                                bool IamLateTrigger = !DoNotConsiderCompletion && (NodeToDo is TriggerNode) && NodeToDo != ExplicitTrigger && !NodeToDo.IsComplete;
                                bool BestIsLateTrigger = !DoNotConsiderCompletion && (BestNode is TriggerNode) && BestNode != ExplicitTrigger && !BestNode.IsComplete;
                                if (BestIsLateTrigger && !IamLateTrigger)
                                {
                                    bReady = false;
                                }
                                else if (BestIsLateTrigger == IamLateTrigger)
                                {

                                    if (Priority < BestPriority)
                                    {
                                        bReady = false;
                                    }
                                    else if (Priority == BestPriority)
                                    {
                                        if (BestNode.Name.CompareTo(NodeToDo.Name) < 0)
                                        {
                                            bReady = false;
                                        }
                                    }
									if (!bCompReady)
									{
										bReady = false;
									}
                                }
                            }
                        }
                    }
                }
                if (bReady)
                {
                    BestPriority = Priority;
                    BestNode = NodeToDo;
                    BestPseudoReady = bPseudoReady;
                    bProgressMade = true;
                }
            }
            if (bProgressMade)
            {
                if (!SubSort && BestNode.AgentSharingGroup != "")
                {
                    foreach (BuildNode ChainNode in SortedAgentGroupChains[BestNode.AgentSharingGroup])
                    {
                        OrdereredToDo.Add(ChainNode);
                        NodesToDo.Remove(ChainNode);
                    }
                }
                else
                {
                    OrdereredToDo.Add(BestNode);
                    NodesToDo.Remove(BestNode);
                }
            }

            if (!bProgressMade && NodesToDo.Count > 0)
            {
                Log("Cycle in GUBP, could not resolve:");
                foreach (BuildNode NodeToDo in NodesToDo)
                {
                    string Deps = "";
                    if (!SubSort && NodeToDo.AgentSharingGroup != "")
                    {
                        foreach (BuildNode ChainNode in SortedAgentGroupChains[NodeToDo.AgentSharingGroup])
                        {
                            foreach (BuildNode Dep in ChainNode.Dependencies)
                            {
                                if (!SortedAgentGroupChains[NodeToDo.AgentSharingGroup].Contains(Dep) && NodesToDo.Contains(Dep))
                                {
                                    Deps = Deps + Dep.Name + "[" + ChainNode.Name + "->" + NodeToDo.AgentSharingGroup + "]" + " ";
                                }
                            }
                        }
                    }
                    foreach (BuildNode Dep in NodeToDo.Dependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            Deps = Deps + Dep.Name + " ";
                        }
                    }
                    foreach (BuildNode Dep in NodeToDo.PseudoDependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            Deps = Deps + Dep.Name + " ";
                        }
                    }
                    Log("  {0}    deps: {1}", NodeToDo.Name, Deps);
                }
                throw new AutomationException("Cycle in GUBP");
            }
        }
        if (!SubSort)
        {
            double BuildDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds;
            Log("Took {0}s to sort {1} nodes", BuildDuration / 1000, OrdereredToDo.Count);
        }

        return OrdereredToDo;
    }

    static NodeHistory FindNodeHistory(BuildNode NodeToDo, string CLString, string StoreName)
    {
		NodeHistory History = null;

        if (!(NodeToDo is TriggerNode) && CLString != "")
        {
            string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();
            string NodeStoreWildCard = StoreName.Replace(CLString, "*") + "-" + NodeToDo.Name;
            History = new NodeHistory();

            History.AllStarted = ConvertCLToIntList(TempStorage.FindTempStorageManifests(CmdEnv, NodeStoreWildCard + StartedTempStorageSuffix, false, true, GameNameIfAny));
            History.AllSucceeded = ConvertCLToIntList(TempStorage.FindTempStorageManifests(CmdEnv, NodeStoreWildCard + SucceededTempStorageSuffix, false, true, GameNameIfAny));
            History.AllFailed = ConvertCLToIntList(TempStorage.FindTempStorageManifests(CmdEnv, NodeStoreWildCard + FailedTempStorageSuffix, false, true, GameNameIfAny));

			int CL;
			int.TryParse(CLString, out CL);

            if (History.AllFailed.Count > 0)
            {
                History.LastFailed = History.AllFailed.LastOrDefault(x => x < CL);
            }
            if (History.AllSucceeded.Count > 0)
            {
                History.LastSucceeded = History.AllSucceeded.LastOrDefault(x => x < CL);

                foreach (int Failed in History.AllFailed)
                {
                    if (Failed > History.LastSucceeded)
                    {
                        History.Failed.Add(Failed);
                        History.FailedString = GUBPNode.MergeSpaceStrings(History.FailedString, String.Format("{0}", Failed));
                    }
                }
                foreach (int Started in History.AllStarted)
                {
                    if (Started > History.LastSucceeded && !History.Failed.Contains(Started))
                    {
                        History.InProgress.Add(Started);
                        History.InProgressString = GUBPNode.MergeSpaceStrings(History.InProgressString, String.Format("{0}", Started));
                    }
                }
			}
        }

		return History;
    }
	void GetFailureEmails(ElectricCommander EC, BuildNode NodeToDo, NodeHistory History, string CLString, string StoreName, bool OnlyLateUpdates = false)
	{
        string FailCauserEMails = "";
        string EMailNote = "";
        bool SendSuccessForGreenAfterRed = false;
        int NumPeople = 0;
        if (History != null)
        {
            if (History.LastSucceeded > 0 && History.LastSucceeded < P4Env.Changelist)
            {
                int LastNonDuplicateFail = P4Env.Changelist;
                try
                {
                    if (OnlyLateUpdates)
                    {
						LastNonDuplicateFail = FindLastNonDuplicateFail(NodeToDo, History, CLString, StoreName);
                        if (LastNonDuplicateFail < P4Env.Changelist)
                        {
                            Log("*** Red-after-red spam reduction, changed CL {0} to CL {1} because the errors didn't change.", P4Env.Changelist, LastNonDuplicateFail);
                        }
                    }
                }
                catch (Exception Ex)
                {
                    LastNonDuplicateFail = P4Env.Changelist;
                    Log(System.Diagnostics.TraceEventType.Warning, "Failed to FindLastNonDuplicateFail.");
                    Log(System.Diagnostics.TraceEventType.Warning, LogUtils.FormatException(Ex));
                }

				if(LastNonDuplicateFail > History.LastSucceeded)
				{
					List<P4Connection.ChangeRecord> ChangeRecords = GetSourceChangeRecords(History.LastSucceeded + 1, LastNonDuplicateFail);
					foreach (P4Connection.ChangeRecord Record in ChangeRecords)
					{
						FailCauserEMails = GUBPNode.MergeSpaceStrings(FailCauserEMails, Record.UserEmail);
					}
				}

                if (!String.IsNullOrEmpty(FailCauserEMails))
                {
                    NumPeople++;
                    foreach (char AChar in FailCauserEMails.ToCharArray())
                    {
                        if (AChar == ' ')
                        {
                            NumPeople++;
                        }
                    }
                    if (NumPeople > 50)
                    {
                        EMailNote = String.Format("This step has been broken for more than 50 changes. It last succeeded at CL {0}. ", History.LastSucceeded);
                    }
                }
            }
            else if (History.LastSucceeded <= 0)
            {
                EMailNote = String.Format("This step has been broken for more than a few days, so there is no record of it ever succeeding. ");
            }
            if (EMailNote != "" && !String.IsNullOrEmpty(History.FailedString))
            {
                EMailNote += String.Format("It has failed at CLs {0}. ", History.FailedString);
            }
            if (EMailNote != "" && !String.IsNullOrEmpty(History.InProgressString))
            {
                EMailNote += String.Format("These CLs are being built right now {0}. ", History.InProgressString);
            }
            if (History.LastSucceeded > 0 && History.LastSucceeded < P4Env.Changelist && History.LastFailed > History.LastSucceeded && History.LastFailed < P4Env.Changelist)
            {
                SendSuccessForGreenAfterRed = ParseParam("CIS");
            }
		}

		if (History == null)
		{
			EC.UpdateEmailProperties(NodeToDo, 0, "", FailCauserEMails, EMailNote, SendSuccessForGreenAfterRed);
		}
		else
		{
			EC.UpdateEmailProperties(NodeToDo, History.LastSucceeded, History.FailedString, FailCauserEMails, EMailNote, SendSuccessForGreenAfterRed);
		}
	}

    bool HashSetEqual(HashSet<string> A, HashSet<string> B)
    {
        if (A.Count != B.Count)
        {
            return false;
        }
        foreach (string Elem in A)
        {
            if (!B.Contains(Elem))
            {
                return false;
            }
        }
        foreach (string Elem in B)
        {
            if (!A.Contains(Elem))
            {
                return false;
            }
        }
        return true;
    }

	int FindLastNonDuplicateFail(BuildNode NodeToDo, NodeHistory History, string CLString, string StoreName)
    {
        int Result = P4Env.Changelist;

        string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();
        string NodeStore = StoreName + "-" + NodeToDo.Name + FailedTempStorageSuffix;

        List<int> BackwardsFails = new List<int>(History.AllFailed);
        BackwardsFails.Add(P4Env.Changelist);
        BackwardsFails.Sort();
        BackwardsFails.Reverse();
        HashSet<string> CurrentErrors = null;
        foreach (int CL in BackwardsFails)
        {
            if (CL > P4Env.Changelist)
            {
                continue;
            }
            if (CL <= History.LastSucceeded)
            {
                break;
            }
            string ThisNodeStore = NodeStore.Replace(CLString, String.Format("{0}", CL));
            TempStorage.DeleteLocalTempStorage(CmdEnv, ThisNodeStore, true); // these all clash locally, which is fine we just retrieve them from shared

            List<string> Files = null;
            try
            {
                bool WasLocal;
                Files = TempStorage.RetrieveFromTempStorage(CmdEnv, ThisNodeStore, out WasLocal, GameNameIfAny); // this will fail on our CL if we didn't fail or we are just setting up the branch
            }
            catch (Exception)
            {
            }
            if (Files == null)
            {
                continue;
            }
            if (Files.Count != 1)
            {
                throw new AutomationException("Unexpected number of files for fail record {0}", Files.Count);
            }
            string ErrorFile = Files[0];
            HashSet<string> ThisErrors = ECJobPropsUtils.ErrorsFromProps(ErrorFile);
            if (CurrentErrors == null)
            {
                CurrentErrors = ThisErrors;
            }
            else
            {
                if (CurrentErrors.Count == 0 || !HashSetEqual(CurrentErrors, ThisErrors))
                {
                    break;
                }
                Result = CL;
            }
        }
        return Result;
    }

	/// <summary>
	/// Resolves the names of each node and aggregates' dependencies, and links them together into the build graph.
	/// </summary>
	/// <param name="AggregateNameToInfo">Map of aggregate names to their info objects</param>
	/// <param name="NodeNameToInfo">Map of node names to their info objects</param>
	private static void LinkGraph(IEnumerable<AggregateNode> Aggregates, IEnumerable<BuildNode> Nodes)
	{
		Dictionary<string, BuildNode> NodeNameToInfo = new Dictionary<string,BuildNode>();
		foreach(BuildNode Node in Nodes)
		{
			NodeNameToInfo.Add(Node.Name, Node);
		}

		Dictionary<string, AggregateNode> AggregateNameToInfo = new Dictionary<string, AggregateNode>();
		foreach(AggregateNode Aggregate in Aggregates)
		{
			AggregateNameToInfo.Add(Aggregate.Name, Aggregate);
		}

		int NumErrors = 0;
		foreach (AggregateNode AggregateNode in AggregateNameToInfo.Values)
		{
			LinkAggregate(AggregateNode, AggregateNameToInfo, NodeNameToInfo, ref NumErrors);
		}
		foreach (BuildNode BuildNode in NodeNameToInfo.Values)
		{
			LinkNode(BuildNode, AggregateNameToInfo, NodeNameToInfo, ref NumErrors);
		}
		if(NumErrors > 0)
		{
			throw new AutomationException("Failed to link graph ({0} errors).", NumErrors);
		}
	}

	/// <summary>
	/// Resolves the dependency names in an aggregate to NodeInfo instances, filling in the AggregateInfo.Dependenices array. Any referenced aggregates will also be linked, recursively.
	/// </summary>
	/// <param name="Aggregate">The aggregate to link</param>
	/// <param name="AggregateNameToInfo">Map of other aggregate names to their corresponding instance.</param>
	/// <param name="NodeNameToInfo">Map from node names to their corresponding instance.</param>
	/// <param name="NumErrors">The number of errors output so far. Incremented if resolving this aggregate fails.</param>
	private static void LinkAggregate(AggregateNode Aggregate, Dictionary<string, AggregateNode> AggregateNameToInfo, Dictionary<string, BuildNode> NodeNameToInfo, ref int NumErrors)
	{
		if (Aggregate.Dependencies == null)
		{
			Aggregate.Dependencies = new BuildNode[0];

			HashSet<BuildNode> Dependencies = new HashSet<BuildNode>();
			foreach (string DependencyName in Aggregate.Node.Dependencies)
			{
				AggregateNode AggregateDependency;
				if(AggregateNameToInfo.TryGetValue(DependencyName, out AggregateDependency))
				{
					LinkAggregate(AggregateDependency, AggregateNameToInfo, NodeNameToInfo, ref NumErrors);
					Dependencies.UnionWith(AggregateDependency.Dependencies);
					continue;
				}

				BuildNode Dependency;
				if(NodeNameToInfo.TryGetValue(DependencyName, out Dependency))
				{
					Dependencies.Add(Dependency);
					continue;
				}

				CommandUtils.LogError("Node {0} is not in the graph. It is a dependency of {1}.", DependencyName, Aggregate.Name);
				NumErrors++;
			}
			Aggregate.Dependencies = Dependencies.ToArray();
		}
	}

	/// <summary>
	/// Resolve a node's dependency names to arrays of NodeInfo instances, filling in the appropriate fields in the NodeInfo object. 
	/// </summary>
	/// <param name="Node"></param>
	/// <param name="AggregateNameToInfo">Map of other aggregate names to their corresponding instance.</param>
	/// <param name="NodeNameToInfo">Map from node names to their corresponding instance.</param>
	/// <param name="NumErrors">The number of errors output so far. Incremented if resolving this aggregate fails.</param>
	private static void LinkNode(BuildNode Node, Dictionary<string, AggregateNode> AggregateNameToInfo, Dictionary<string, BuildNode> NodeNameToInfo, ref int NumErrors)
	{
		if(Node.Dependencies == null)
		{
			// Find all the dependencies
			HashSet<BuildNode> Dependencies = new HashSet<BuildNode>();
			foreach (string DependencyName in Node.Node.FullNamesOfDependencies)
			{
				if (!ResolveDependencies(DependencyName, AggregateNameToInfo, NodeNameToInfo, Dependencies))
				{
					CommandUtils.LogError("Node {0} is not in the graph. It is a dependency of {1}.", DependencyName, Node.Name);
					NumErrors++;
				}
			}
			Node.Dependencies = Dependencies.ToArray();

			// Find all the pseudo-dependencies
			HashSet<BuildNode> PseudoDependencies = new HashSet<BuildNode>();
			foreach (string PseudoDependencyName in Node.Node.FullNamesOfPseudosependencies)
			{
				if (!ResolveDependencies(PseudoDependencyName, AggregateNameToInfo, NodeNameToInfo, PseudoDependencies))
				{
					CommandUtils.LogError("Node {0} is not in the graph. It is a pseudodependency of {1}.", PseudoDependencyName, Node.Name);
					NumErrors++;
				}
			}
			Node.PseudoDependencies = PseudoDependencies.ToArray();

			// Set the direct dependencies list
			Node.AllDirectDependencies = Node.Dependencies.Union(Node.PseudoDependencies).ToArray();

			// Recursively find the dependencies for all the dependencies
			HashSet<BuildNode> IndirectDependenices = new HashSet<BuildNode>(Node.AllDirectDependencies);
			foreach(BuildNode DirectDependency in Node.AllDirectDependencies)
			{
				LinkNode(DirectDependency, AggregateNameToInfo, NodeNameToInfo, ref NumErrors);
				IndirectDependenices.UnionWith(DirectDependency.AllIndirectDependencies);
			}
			Node.AllIndirectDependencies = IndirectDependenices.ToArray();

			// Check the node doesn't reference itself
			if(Node.AllIndirectDependencies.Contains(Node))
			{
				CommandUtils.LogError("Node {0} has a dependency on itself.", Node.Name);
				NumErrors++;
			}
		}
	}

	/// <summary>
	/// Adds all the nodes matching a given name to a hash set, expanding any aggregates to their dependencices.
	/// </summary>
	/// <param name="Name">The name to look for</param>
	/// <param name="AggregateNameToInfo">Map of other aggregate names to their corresponding info instance.</param>
	/// <param name="NodeNameToInfo">Map from node names to their corresponding info instance.</param>
	/// <param name="Dependencies">The set of dependencies to add to.</param>
	/// <returns>True if the name was found (and the dependencies list was updated), false otherwise.</returns>
	private static bool ResolveDependencies(string Name, Dictionary<string, AggregateNode> AggregateNameToInfo, Dictionary<string, BuildNode> NodeNameToInfo, HashSet<BuildNode> Dependencies)
	{
		AggregateNode AggregateDependency;
		if (AggregateNameToInfo.TryGetValue(Name, out AggregateDependency))
		{
			Dependencies.UnionWith(AggregateDependency.Dependencies);
			return true;
		}

		BuildNode NodeDependency;
		if (NodeNameToInfo.TryGetValue(Name, out NodeDependency))
		{
			Dependencies.Add(NodeDependency);
			return true;
		}

		return false;
	}

	/// <summary>
	/// Determine which nodes to build from the command line, or return everything if there's nothing specified explicitly.
	/// </summary>
	/// <param name="PotentialNodes">The nodes in the graph</param>
	/// <param name="PotentialAggregates">The aggregates in the graph</param>
	/// <returns>Set of all nodes to execute, recursively including their dependencies.</returns>
	private HashSet<BuildNode> ParseNodesToDo(IEnumerable<BuildNode> PotentialNodes, IEnumerable<AggregateNode> PotentialAggregates)
	{
		// Parse all the node names that we want
		List<string> NamesToDo = new List<string>();

		string NodeParam = ParseParamValue("Node", null);
		if (NodeParam != null)
		{
			NamesToDo.AddRange(NodeParam.Split('+'));
		}

		string GameParam = ParseParamValue("Game", null);
		if (GameParam != null)
		{
			NamesToDo.AddRange(GameParam.Split('+').Select(x => FullGameAggregateNode.StaticGetFullName(x)));
		}

		// Resolve all those node names to a list of build nodes to execute
		HashSet<BuildNode> NodesToDo = new HashSet<BuildNode>();
		foreach (string NameToDo in NamesToDo)
		{
			int FoundNames = 0;
			foreach (BuildNode PotentialNode in PotentialNodes)
			{
				if (String.Compare(PotentialNode.Name, NameToDo, StringComparison.InvariantCultureIgnoreCase) == 0 || String.Compare(PotentialNode.AgentSharingGroup, NameToDo, StringComparison.InvariantCultureIgnoreCase) == 0)
				{
					NodesToDo.Add(PotentialNode);
					FoundNames++;
				}
			}
			foreach (AggregateNode PotentialAggregate in PotentialAggregates)
			{
				if (String.Compare(PotentialAggregate.Name, NameToDo, StringComparison.InvariantCultureIgnoreCase) == 0)
				{
					NodesToDo.UnionWith(PotentialAggregate.Dependencies);
					FoundNames++;
				}
			}
			if (FoundNames == 0)
			{
				throw new AutomationException("Could not find node named {0}", FoundNames);
			}
		}

		// Add everything if there was nothing specified explicitly
		if (NodesToDo.Count == 0)
		{
			NodesToDo.UnionWith(PotentialNodes);
		}

		// Recursively find the complete set of all nodes we want to execute
		HashSet<BuildNode> RecursiveNodesToDo = new HashSet<BuildNode>(NodesToDo);
		foreach(BuildNode NodeToDo in NodesToDo)
		{
			RecursiveNodesToDo.UnionWith(NodeToDo.AllIndirectDependencies);
		}

		return RecursiveNodesToDo;
	}

	/// <summary>
	/// Culls the list of nodes to do depending on the current time index.
	/// </summary>
	/// <param name="NodesToDo">The list of nodes to do</param>
	/// <param name="TimeIndex">The current time index. All nodes are run for TimeIndex=0, otherwise they are culled based on their FrequencyShift parameter.</param>
	private void CullNodesForTimeIndex(HashSet<BuildNode> NodesToDo, int TimeIndex)
	{
		if (TimeIndex != 0)
		{
			List<BuildNode> NodesToCull = new List<BuildNode>();
			foreach (BuildNode NodeToDo in NodesToDo)
			{
				if ((TimeIndex % (1 << NodeToDo.FrequencyShift)) != 0)
				{
					NodesToCull.Add(NodeToDo);
				}
			}
			NodesToDo.ExceptWith(NodesToCull);
		}
	}

	/// <summary>
	/// Culls everything downstream of a trigger if we're running a preflight build.
	/// </summary>
	/// <param name="NodesToDo">The current list of nodes to do</param>
	/// <param name="IsPreflightBuild">Whether this is a preflight build or not</param>
	private void CullNodesForPreflight(HashSet<BuildNode> NodesToDo, bool IsPreflightBuild)
	{
		if (IsPreflightBuild)
		{
			HashSet<BuildNode> NodesToCull = new HashSet<BuildNode>();
			foreach(BuildNode NodeToDo in NodesToDo)
			{
				if((NodeToDo is TriggerNode) || NodeToDo.ControllingTriggers.Length > 0)
				{
					LogVerbose(" Culling {0} due to being downstream of trigger in preflight", NodeToDo.Name);
					NodesToCull.Add(NodeToDo);
				}
			}
			NodesToDo.ExceptWith(NodesToCull);
		}
	}

	private int UpdateCISCounter(int TimeQuantum)
	{
		if (!P4Enabled)
		{
			throw new AutomationException("Can't have -CIS without P4 support");
		}
		string P4IndexFileP4 = CombinePaths(PathSeparator.Slash, CommandUtils.P4Env.BuildRootP4, "Engine", "Build", "CISCounter.txt");
		string P4IndexFileLocal = CombinePaths(CmdEnv.LocalRoot, "Engine", "Build", "CISCounter.txt");
		for(int Retry = 0; Retry < 20; Retry++)
		{
			int NowMinutes = (int)((DateTime.UtcNow - new DateTime(2014, 1, 1)).TotalMinutes);
			if (NowMinutes < 3 * 30 * 24)
			{
				throw new AutomationException("bad date calc");
			}
			if (!FileExists_NoExceptions(P4IndexFileLocal))
			{
				LogVerbose("{0} doesn't exist, checking in a new one", P4IndexFileP4);
				WriteAllText(P4IndexFileLocal, "-1 0");
				int WorkingCL = -1;
				try
				{
					WorkingCL = P4.CreateChange(P4Env.Client, "Adding new CIS Counter");
					P4.Add(WorkingCL, P4IndexFileP4);
					int SubmittedCL;
					P4.Submit(WorkingCL, out SubmittedCL);
				}
				catch (Exception)
				{
					LogWarning("Add of CIS counter failed, assuming it now exists.");
					if (WorkingCL > 0)
					{
						P4.DeleteChange(WorkingCL);
					}
				}
			}
			P4.Sync("-f " + P4IndexFileP4 + "#head");
			if (!FileExists_NoExceptions(P4IndexFileLocal))
			{
				LogVerbose("{0} doesn't exist, checking in a new one", P4IndexFileP4);
				WriteAllText(P4IndexFileLocal, "-1 0");
				int WorkingCL = -1;
				try
				{
					WorkingCL = P4.CreateChange(P4Env.Client, "Adding new CIS Counter");
					P4.Add(WorkingCL, P4IndexFileP4);
					int SubmittedCL;
					P4.Submit(WorkingCL, out SubmittedCL);
				}
				catch (Exception)
				{
					LogWarning("Add of CIS counter failed, assuming it now exists.");
					if (WorkingCL > 0)
					{
						P4.DeleteChange(WorkingCL);
					}
				}
			}
			string Data = ReadAllText(P4IndexFileLocal);
			string[] Parts = Data.Split(" ".ToCharArray());
			int Index = int.Parse(Parts[0]);
			int Minutes = int.Parse(Parts[1]);

			int DeltaMinutes = NowMinutes - Minutes;

			int NewIndex = Index + 1;

			if (DeltaMinutes > TimeQuantum * 2)
			{
				if (DeltaMinutes > TimeQuantum * (1 << 8))
				{
					// it has been forever, lets just start over
					NewIndex = 0;
				}
				else
				{
					int WorkingIndex = NewIndex + 1;
					for (int WorkingDelta = DeltaMinutes - TimeQuantum; WorkingDelta > 0; WorkingDelta -= TimeQuantum, WorkingIndex++)
					{
						if (CountZeros(NewIndex) < CountZeros(WorkingIndex))
						{
							NewIndex = WorkingIndex;
						}
					}
				}
			}
			{
				string Line = String.Format("{0} {1}", NewIndex, NowMinutes);
				LogVerbose("Attempting to write {0} with {1}", P4IndexFileP4, Line);
				int WorkingCL = -1;
				try
				{
					WorkingCL = P4.CreateChange(P4Env.Client, "Updating CIS Counter");
					P4.Edit(WorkingCL, P4IndexFileP4);
					WriteAllText(P4IndexFileLocal, Line);
					int SubmittedCL;
					P4.Submit(WorkingCL, out SubmittedCL);
					return NewIndex;
				}
				catch (Exception)
				{
					LogWarning("Edit of CIS counter failed, assuming someone else checked in, retrying.");
					if (WorkingCL > 0)
					{
						P4.DeleteChange(WorkingCL);
					}
					System.Threading.Thread.Sleep(30000);
				}
			}
		}
		throw new AutomationException("Failed to update the CIS counter after 20 tries.");
	}

	private List<TriggerNode> FindUnfinishedTriggers(bool bSkipTriggers, BuildNode ExplicitTrigger, List<BuildNode> OrdereredToDo)
	{
		// find all unfinished triggers, excepting the one we are triggering right now
		List<TriggerNode> UnfinishedTriggers = new List<TriggerNode>();
		if (!bSkipTriggers)
		{
			foreach (TriggerNode NodeToDo in OrdereredToDo.OfType<TriggerNode>())
			{
				if (!NodeToDo.IsComplete && ExplicitTrigger != NodeToDo)
				{
					UnfinishedTriggers.Add(NodeToDo);
				}
			}
		}
		return UnfinishedTriggers;
	}

	/// <summary>
	/// Validates that the given nodes are sorted correctly, so that all dependencies are met first
	/// </summary>
	/// <param name="OrdereredToDo">The sorted list of nodes</param>
	private void CheckSortOrder(List<BuildNode> OrdereredToDo)
	{
		foreach (BuildNode NodeToDo in OrdereredToDo)
		{
			if ((NodeToDo is TriggerNode) && (NodeToDo.IsSticky || NodeToDo.IsComplete)) // these sticky triggers are ok, everything is already completed anyway
			{
				continue;
			}
			int MyIndex = OrdereredToDo.IndexOf(NodeToDo);
			foreach (BuildNode Dep in NodeToDo.Dependencies)
			{
				int DepIndex = OrdereredToDo.IndexOf(Dep);
				if (DepIndex >= MyIndex)
				{
					throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
				}
			}
			foreach (BuildNode Dep in NodeToDo.PseudoDependencies)
			{
				int DepIndex = OrdereredToDo.IndexOf(Dep);
				if (DepIndex >= MyIndex)
				{
					throw new AutomationException("Topological sort error, node {0} has a pseduodependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
				}
			}
		}
	}

	private void DoCommanderSetup(ElectricCommander EC, IEnumerable<BuildNode> AllNodes, IEnumerable<AggregateNode> AllAggregates, List<BuildNode> OrdereredToDo, int TimeIndex, int TimeQuantum, bool bSkipTriggers, bool bFake, bool bFakeEC, string CLString, TriggerNode ExplicitTrigger, List<TriggerNode> UnfinishedTriggers, string FakeFail, bool bPreflightBuild)
	{
		List<BuildNode> SortedNodes = TopologicalSort(new HashSet<BuildNode>(AllNodes), null, SubSort: false, DoNotConsiderCompletion: true);
		Log("******* {0} GUBP Nodes", SortedNodes.Count);

		List<BuildNode> FilteredOrdereredToDo = new List<BuildNode>();
		using(TelemetryStopwatch StartFilterTimer = new TelemetryStopwatch("FilterNodes"))
		{
			// remove nodes that have unfinished triggers
			foreach (BuildNode NodeToDo in OrdereredToDo)
			{
				if (NodeToDo.ControllingTriggers.Length == 0 || !UnfinishedTriggers.Contains(NodeToDo.ControllingTriggers.Last()))
				{
					// if we are triggering, then remove nodes that are not controlled by the trigger or are dependencies of this trigger
					if (ExplicitTrigger != null && ExplicitTrigger != NodeToDo && !ExplicitTrigger.DependsOn(NodeToDo) && !NodeToDo.DependsOn(ExplicitTrigger))
					{
						continue; // this wasn't on the chain related to the trigger we are triggering, so it is not relevant
					}

					// in preflight builds, we are either skipping triggers (and running things downstream) or we just stop at triggers and don't make them available for triggering.
					if (bPreflightBuild && !bSkipTriggers && (NodeToDo is TriggerNode))
					{
						continue;
					}

					FilteredOrdereredToDo.Add(NodeToDo);
				}
			}
		}
		using(TelemetryStopwatch PrintNodesTimer = new TelemetryStopwatch("SetupCommanderPrint"))
		{
			Log("*********** EC Nodes, in order.");
			PrintNodes(this, FilteredOrdereredToDo, AllAggregates, UnfinishedTriggers, TimeQuantum);
		}

		EC.DoCommanderSetup(AllNodes, AllAggregates, FilteredOrdereredToDo, SortedNodes, TimeIndex, TimeQuantum, bSkipTriggers, bFake, bFakeEC, CLString, ExplicitTrigger, UnfinishedTriggers, FakeFail);
	}

	void ExecuteNodes(ElectricCommander EC, List<BuildNode> OrdereredToDo, bool bFake, bool bFakeEC, bool bSaveSharedTempStorage, string CLString, string StoreName, string FakeFail)
	{
        Dictionary<string, BuildNode> BuildProductToNodeMap = new Dictionary<string, BuildNode>();
		foreach (BuildNode NodeToDo in OrdereredToDo)
        {
            if (NodeToDo.Node.BuildProducts != null || NodeToDo.Node.AllDependencyBuildProducts != null)
            {
                throw new AutomationException("topological sort error");
            }

            NodeToDo.Node.AllDependencyBuildProducts = new List<string>();
            NodeToDo.Node.AllDependencies = new List<string>();
            foreach (BuildNode Dep in NodeToDo.Dependencies)
            {
                NodeToDo.Node.AddAllDependent(Dep.Name);

                if (Dep.Node.AllDependencies == null)
                {
					throw new AutomationException("Node {0} was not processed yet?  Processing {1}", Dep, NodeToDo.Name);
                }

				foreach (string DepDep in Dep.Node.AllDependencies)
				{
					NodeToDo.Node.AddAllDependent(DepDep);
				}
				
                if (Dep.Node.BuildProducts == null)
                {
                    throw new AutomationException("Node {0} was not processed yet? Processing {1}", Dep, NodeToDo.Name);
                }

				foreach (string Prod in Dep.Node.BuildProducts)
                {
                    NodeToDo.Node.AddDependentBuildProduct(Prod);
                }

                if (Dep.Node.AllDependencyBuildProducts == null)
                {
                    throw new AutomationException("Node {0} was not processed yet2?  Processing {1}", Dep.Name, NodeToDo.Name);
                }

                foreach (string Prod in Dep.Node.AllDependencyBuildProducts)
                {
                    NodeToDo.Node.AddDependentBuildProduct(Prod);
                }
            }

            string NodeStoreName = StoreName + "-" + NodeToDo.Name;
            
            string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();
            string StorageRootIfAny = NodeToDo.Node.RootIfAnyForTempStorage();
					
            if (bFake)
            {
                StorageRootIfAny = ""; // we don't rebase fake runs since those are entirely "records of success", which are always in the logs folder
            }

            // this is kinda complicated
            bool SaveSuccessRecords = (IsBuildMachine || bFakeEC) && // no real reason to make these locally except for fakeEC tests
                (!(NodeToDo is TriggerNode) || NodeToDo.IsSticky); // trigger nodes are run twice, one to start the new workflow and once when it is actually triggered, we will save reconds for the latter

            Log("***** Running GUBP Node {0} -> {1} : {2}", NodeToDo.Name, GameNameIfAny, NodeStoreName);
            if (NodeToDo.IsComplete)
            {
                if (NodeToDo.Name == VersionFilesNode.StaticGetFullName() && !IsBuildMachine)
                {
                    Log("***** NOT ****** Retrieving GUBP Node {0} from {1}; it is the version files.", NodeToDo.Name, NodeStoreName);
                    NodeToDo.Node.BuildProducts = new List<string>();

                }
                else
                {
                    Log("***** Retrieving GUBP Node {0} from {1}", NodeToDo.Name, NodeStoreName);
                    bool WasLocal;
					try
					{
						NodeToDo.Node.BuildProducts = TempStorage.RetrieveFromTempStorage(CmdEnv, NodeStoreName, out WasLocal, GameNameIfAny, StorageRootIfAny);
					}
					catch
					{
						if(GameNameIfAny != "")
						{
							NodeToDo.Node.BuildProducts = TempStorage.RetrieveFromTempStorage(CmdEnv, NodeStoreName, out WasLocal, "", StorageRootIfAny);
						}
						else
						{
							throw new AutomationException("Build Products cannot be found for node {0}", NodeToDo.Name);
						}
					}
                    if (!WasLocal)
                    {
                        NodeToDo.Node.PostLoadFromSharedTempStorage(this);
                    }
                }
            }
            else
            {
                if (SaveSuccessRecords) 
                {
                    EC.SaveStatus(NodeToDo, StartedTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
                }
				double BuildDuration = 0.0;
                try
                {
                    if (!String.IsNullOrEmpty(FakeFail) && FakeFail.Equals(NodeToDo.Name, StringComparison.InvariantCultureIgnoreCase))
                    {
                        throw new AutomationException("Failing node {0} by request.", NodeToDo.Name);
                    }
                    if (bFake)
                    {
                        Log("***** FAKE!! Building GUBP Node {0} for {1}", NodeToDo.Name, NodeStoreName);
                        NodeToDo.Node.DoFakeBuild(this);
                    }
                    else
                    {                        
						Log("***** Building GUBP Node {0} for {1}", NodeToDo.Name, NodeStoreName);
						DateTime StartTime = DateTime.UtcNow;
						using(TelemetryStopwatch DoBuildStopwatch = new TelemetryStopwatch("DoBuild.{0}", NodeToDo.Name))
						{
							NodeToDo.Node.DoBuild(this);
						}
						BuildDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds / 1000;
						
                    }

					using(TelemetryStopwatch StoreBuildProductsStopwatch = new TelemetryStopwatch("StoreBuildProducts"))
					{
						double StoreDuration = 0.0;
						DateTime StartTime = DateTime.UtcNow;
						TempStorage.StoreToTempStorage(CmdEnv, NodeStoreName, NodeToDo.Node.BuildProducts, !bSaveSharedTempStorage, GameNameIfAny, StorageRootIfAny);
						StoreDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds / 1000;
						Log("Took {0} seconds to store build products", StoreDuration);
						if (IsBuildMachine)
						{
							EC.RunECTool(String.Format("setProperty \"/myJobStep/StoreDuration\" \"{0}\"", StoreDuration.ToString()));
						}
					}
                    if (ParseParam("StompCheck"))
                    {
                        foreach (string Dep in NodeToDo.Node.AllDependencies)
                        {
                            try
                            {
                                bool WasLocal;
								using(TelemetryStopwatch RetrieveBuildProductsStopwatch = new TelemetryStopwatch("RetrieveBuildProducts"))
								{
									TempStorage.RetrieveFromTempStorage(CmdEnv, NodeStoreName, out WasLocal, GameNameIfAny, StorageRootIfAny);
								}
								if (!WasLocal)
								{
									throw new AutomationException("Retrieve was not local?");
								}
																	                                    
                            }
                            catch(Exception Ex)
                            {
                                throw new AutomationException("Node {0} stomped Node {1}   Ex: {2}", NodeToDo.Name, Dep, LogUtils.FormatException(Ex));
                            }
                        }
                    }
                }
                catch (Exception Ex)
                {
					NodeHistory History = null;

                    if (SaveSuccessRecords)
                    {
						using(TelemetryStopwatch UpdateNodeHistoryStopwatch = new TelemetryStopwatch("UpdateNodeHistory"))
						{
							History = FindNodeHistory(NodeToDo, CLString, StoreName);
						}
						using(TelemetryStopwatch SaveNodeStatusStopwatch = new TelemetryStopwatch("SaveNodeStatus"))
						{
							EC.SaveStatus(NodeToDo, FailedTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny, ParseParamValue("MyJobStepId"));
						}
						using(TelemetryStopwatch UpdateECPropsStopwatch = new TelemetryStopwatch("UpdateECProps"))
						{
							EC.UpdateECProps(NodeToDo);
						}
                        
						if (IsBuildMachine)
						{
							using(TelemetryStopwatch GetFailEmailsStopwatch = new TelemetryStopwatch("GetFailEmails"))
							{
								GetFailureEmails(EC, NodeToDo, History, CLString, StoreName);
							}
						}
						EC.UpdateECBuildTime(NodeToDo, BuildDuration);
                    }

                    Log("{0}", ExceptionToString(Ex));


                    if (History != null)
                    {
                        Log("Changes since last green *********************************");
                        Log("");
                        Log("");
                        Log("");
                        PrintDetailedChanges(History, P4Env.Changelist);
                        Log("End changes since last green");
                    }

                    string FailInfo = "";
                    FailInfo += "********************************* Main log file";
                    FailInfo += Environment.NewLine + Environment.NewLine;
                    FailInfo += LogUtils.GetLogTail();
                    FailInfo += Environment.NewLine + Environment.NewLine + Environment.NewLine;



                    string OtherLog = "See logfile for details: '";
                    if (FailInfo.Contains(OtherLog))
                    {
                        string LogFile = FailInfo.Substring(FailInfo.IndexOf(OtherLog) + OtherLog.Length);
                        if (LogFile.Contains("'"))
                        {
                            LogFile = CombinePaths(CmdEnv.LogFolder, LogFile.Substring(0, LogFile.IndexOf("'")));
                            if (FileExists_NoExceptions(LogFile))
                            {
                                FailInfo += "********************************* Sub log file " + LogFile;
                                FailInfo += Environment.NewLine + Environment.NewLine;

                                FailInfo += LogUtils.GetLogTail(LogFile);
                                FailInfo += Environment.NewLine + Environment.NewLine + Environment.NewLine;
                            }
                        }
                    }

                    string Filename = CombinePaths(CmdEnv.LogFolder, "LogTailsAndChanges.log");
                    WriteAllText(Filename, FailInfo);

                    throw(Ex);
                }
                if (SaveSuccessRecords) 
                {
					NodeHistory History = null;
					using(TelemetryStopwatch UpdateNodeHistoryStopwatch = new TelemetryStopwatch("UpdateNodeHistory"))
					{
						History = FindNodeHistory(NodeToDo, CLString, StoreName);
					}
					using(TelemetryStopwatch SaveNodeStatusStopwatch = new TelemetryStopwatch("SaveNodeStatus"))
					{
						EC.SaveStatus(NodeToDo, SucceededTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
					}
					using(TelemetryStopwatch UpdateECPropsStopwatch = new TelemetryStopwatch("UpdateECProps"))
					{
						EC.UpdateECProps(NodeToDo);
					}
                    
					if (IsBuildMachine)
					{
						using(TelemetryStopwatch GetFailEmailsStopwatch = new TelemetryStopwatch("GetFailEmails"))
						{
							GetFailureEmails(EC, NodeToDo, History, CLString, StoreName);
						}
					}
					EC.UpdateECBuildTime(NodeToDo, BuildDuration);
                }
            }
            foreach (string Product in NodeToDo.Node.BuildProducts)
            {
                if (BuildProductToNodeMap.ContainsKey(Product))
                {
                    throw new AutomationException("Overlapping build product: {0} and {1} both produce {2}", BuildProductToNodeMap[Product], NodeToDo.Name, Product);
                }
                BuildProductToNodeMap.Add(Product, NodeToDo);
            }
        }        
        PrintRunTime();
    }   
}
