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

public partial class GUBP : BuildCommand
{
	const string StartedTempStorageSuffix = "_Started";
	const string FailedTempStorageSuffix = "_Failed";
	const string SucceededTempStorageSuffix = "_Succeeded";

	public string BranchName;
    public int CL = 0;
    public bool bSignBuildProducts = false;
    public BranchInfo Branch = null;
    public List<UnrealTargetPlatform> HostPlatforms;
    public bool bForceIncrementalCompile = false;
    public bool bPreflightBuild = false;
    public string PreflightMangleSuffix = "";
    public GUBPBranchHacker.BranchOptions BranchOptions = null;	

	Dictionary<string, NodeInfo> GUBPNodes;
	Dictionary<string, AggregateInfo> GUBPAggregates;

	[DebuggerDisplay("{Name}")]
	class NodeInfo
	{
		public string Name;
		public GUBPNode Node;
		public NodeInfo[] Dependencies;
		public NodeInfo[] PseudoDependencies;
		public NodeInfo[] ControllingTriggers;
		public int FrequencyShift = -1;
		public bool IsComplete;

		public string ControllingTriggerDotName
		{
			get { return String.Join(".", ControllingTriggers.Select(x => x.Name)); }
		}

		public override string ToString()
		{
			System.Diagnostics.Trace.TraceWarning("Implicit conversion from NodeInfo to string\n{0}", Environment.StackTrace);
			return Name;
		}
	}

	[DebuggerDisplay("{Name}")]
	class AggregateInfo
	{
		public string Name;
		public AggregateNode Node;
		public NodeInfo[] Dependencies;
	}

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

    public string AddNode(GUBPNode Node)
    {
        string Name = Node.GetFullName();
		if (GUBPNodes.ContainsKey(Name) || GUBPAggregates.ContainsKey(Name))
		{
            throw new AutomationException("Attempt to add a duplicate node {0}", Node.GetFullName());
        }
        GUBPNodes.Add(Name, new NodeInfo{ Name = Name, Node = Node });
        return Name;
    }

	public string AddNode(AggregateNode Node)
	{
		string Name = Node.GetFullName();
		if (GUBPNodes.ContainsKey(Name) || GUBPAggregates.ContainsKey(Name))
		{
			throw new AutomationException("Attempt to add a duplicate node {0}", Node.GetFullName());
		}
		GUBPAggregates.Add(Name, new AggregateInfo { Name = Name, Node = Node });
		return Name;
	}

    public bool HasNode(string Node)
    {
        return GUBPNodes.ContainsKey(Node) || GUBPAggregates.ContainsKey(Node);
    }

    public GUBPNode FindNode(string Node)
    {
        return GUBPNodes[Node].Node;
    }

    public AggregateNode FindAggregateNode(string Node)
    {
        return GUBPAggregates[Node].Node;
    }

	public GUBPNode TryFindNode(string Node)
	{
		NodeInfo Result;
		GUBPNodes.TryGetValue(Node, out Result);
		return (Result == null)? null : Result.Node;
	}

    public void RemovePseudodependencyFromNode(string Node, string Dep)
    {
        if (!GUBPNodes.ContainsKey(Node))
        {
            throw new AutomationException("Node {0} not found", Node);
        }
        GUBPNodes[Node].Node.RemovePseudodependency(Dep);
    }

    public void RemoveAllPseudodependenciesFromNode(string Node)
    {
        if (!GUBPNodes.ContainsKey(Node))
        {
            throw new AutomationException("Node {0} not found", Node);
        }
        GUBPNodes[Node].Node.FullNamesOfPseudosependencies.Clear();
    }

    static List<NodeInfo> GetDependencies(NodeInfo NodeToDo, bool bFlat = false, bool ECOnly = false)
    {
        List<NodeInfo> Result = new List<NodeInfo>();
        foreach (NodeInfo Dependency in NodeToDo.Dependencies)
        {
            bool Usable = Dependency.Node.RunInEC() || !ECOnly;
            if (Usable)
            {
                if (!Result.Contains(Dependency))
                {
                    Result.Add(Dependency);
                }
            }
            if (bFlat || !Usable)
            {
                foreach (NodeInfo RNode in GetDependencies(Dependency, bFlat, ECOnly))
                {
                    if (!Result.Contains(RNode))
                    {
                        Result.Add(RNode);
                    }
                }
            }
        }
        foreach (NodeInfo PseudoDependency in NodeToDo.PseudoDependencies)
        {
            bool Usable = PseudoDependency.Node.RunInEC() || !ECOnly;
            if (Usable)
            {
                if (!Result.Contains(PseudoDependency))
                {
                    Result.Add(PseudoDependency);
                }
            }
            if (bFlat || !Usable)
            {
                foreach (NodeInfo RNode in GetDependencies(PseudoDependency, bFlat, ECOnly))
                {
                    if (!Result.Contains(RNode))
                    {
                        Result.Add(RNode);
                    }
                }
            }
        }
        return Result;
    }

	static List<NodeInfo> GetECDependencies(NodeInfo NodeToDo, bool bFlat = false)
    {
        return GetDependencies(NodeToDo, bFlat, true);
    }
    bool NodeDependsOn(NodeInfo Rootward, NodeInfo Leafward)
    {
        List<NodeInfo> Deps = GetDependencies(Leafward, true);
        return Deps.Contains(Rootward);
    }

	static void FindControllingTriggers(IEnumerable<NodeInfo> NodesToDo)
	{
		foreach(NodeInfo NodeToDo in NodesToDo)
		{
			FindControllingTriggers(NodeToDo);
		}
	}

    static void FindControllingTriggers(NodeInfo NodeToDo)
    {
		if(NodeToDo.ControllingTriggers == null)
		{
			NodeToDo.ControllingTriggers = new NodeInfo[0];

			// Find all the dependencies of this node
			List<NodeInfo> AllDependencies = new List<NodeInfo>();
			AllDependencies.AddRange(NodeToDo.Dependencies);
			AllDependencies.AddRange(NodeToDo.PseudoDependencies);

			// Find the immediate trigger controlling this one
			List<NodeInfo> PreviousTriggers = new List<NodeInfo>();
			foreach (NodeInfo Dependency in AllDependencies)
			{
				FindControllingTriggers(Dependency);

				NodeInfo PreviousTrigger = null;
				if(Dependency.Node.TriggerNode())
				{
					PreviousTrigger = Dependency;
				}
				else if(Dependency.ControllingTriggers.Length > 0)
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
				List<NodeInfo> ControllingTriggers = new List<NodeInfo>();
				ControllingTriggers.AddRange(PreviousTriggers[0].ControllingTriggers);
				ControllingTriggers.Add(PreviousTriggers[0]);
				NodeToDo.ControllingTriggers = ControllingTriggers.ToArray();
			}
		}
	}

    string CISFrequencyQuantumShiftString(NodeInfo NodeToDo)
    {
        string FrequencyString = "";
        int Quantum = NodeToDo.FrequencyShift;
		if (Quantum > 0)
        {
			int TimeQuantum = 20;
			if(BranchOptions.QuantumOverride != 0)
			{
				TimeQuantum = BranchOptions.QuantumOverride;
			}
            int Minutes = TimeQuantum * (1 << Quantum);
            if (Minutes < 60)
            {
                FrequencyString = string.Format(" ({0}m)", Minutes);
            }
            else
            {
                FrequencyString = string.Format(" ({0}h{1}m)", Minutes / 60, Minutes % 60);
            }
        }
        return FrequencyString;
    }

    int ComputeDependentCISFrequencyQuantumShift(NodeInfo NodeToDo, Dictionary<NodeInfo, int> FrequencyOverrides)
    {
        int Result = NodeToDo.FrequencyShift;        
        if (Result < 0)
        {
            Result = NodeToDo.Node.CISFrequencyQuantumShift(this);
            Result = GetFrequencyForNode(this, NodeToDo, Result);

			int FrequencyOverride;
			if(FrequencyOverrides.TryGetValue(NodeToDo, out FrequencyOverride) && Result > FrequencyOverride)
			{
				Result = FrequencyOverride;
			}

            foreach (NodeInfo Dep in NodeToDo.Dependencies)
            {
                Result = Math.Max(ComputeDependentCISFrequencyQuantumShift(Dep, FrequencyOverrides), Result);
            }
            foreach (NodeInfo Dep in NodeToDo.PseudoDependencies)
            {
                Result = Math.Max(ComputeDependentCISFrequencyQuantumShift(Dep, FrequencyOverrides), Result);
            }
            if (Result < 0)
            {
                throw new AutomationException("Failed to compute shift.");
            }
            NodeToDo.FrequencyShift = Result;
        }
        return Result;
    }

	static void FindCompletionState(IEnumerable<NodeInfo> NodesToDo, string StoreName, bool LocalOnly)
	{
		foreach(NodeInfo NodeToDo in NodesToDo)
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

    string RunECTool(string Args, bool bQuiet = false)
    {
        if (ParseParam("FakeEC"))
        {
            LogWarning("***** Would have ran ectool {0}", Args);
            return "We didn't actually run ectool";
        }
        else
        {
            ERunOptions Opts = ERunOptions.Default;
            if (bQuiet)
            {
                Opts = (Opts & ~ERunOptions.AllowSpew) | ERunOptions.NoLoggingOfRunCommand;
            }
            return RunAndLog("ectool", "--timeout 900 " + Args, Options: Opts);
        }
    }
    void WriteECPerl(List<string> Args)
    {
        Args.Add("$batch->submit();");
        string ECPerlFile = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "jobsteps.pl");
        WriteAllLines_NoExceptions(ECPerlFile, Args.ToArray());
    }
    string GetEMailListForNode(GUBP bp, NodeInfo NodeToDo, string Emails, string Causers)
    {        
        string BranchForEmail = "";
        if (P4Enabled)
        {
            BranchForEmail = P4Env.BuildRootP4;
        }
        return HackEmails(Emails, Causers, BranchForEmail, NodeToDo);
    }
    int GetFrequencyForNode(GUBP bp, NodeInfo NodeToDo, int BaseFrequency)
    {
        return HackFrequency(bp, BranchName, NodeToDo, BaseFrequency);
    }

    List<P4Connection.ChangeRecord> GetChanges(int LastOutputForChanges, int TopCL, int LastGreen)
    {
        List<P4Connection.ChangeRecord> Result = new List<P4Connection.ChangeRecord>();
        if (TopCL > LastGreen)
        {
            if (LastOutputForChanges > 1990000)
            {
                string Cmd = String.Format("{0}@{1},{2} {3}@{4},{5}",
                    CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "...", "Source", "..."), LastOutputForChanges + 1, TopCL,
                    CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "...", "Build", "..."), LastOutputForChanges + 1, TopCL
                    );
                List<P4Connection.ChangeRecord> ChangeRecords;
				if (P4.Changes(out ChangeRecords, Cmd, false, true, LongComment: true))
                {
                    foreach (P4Connection.ChangeRecord Record in ChangeRecords)
                    {
                        if (!Record.User.Equals("buildmachine", StringComparison.InvariantCultureIgnoreCase))
                        {
                            Result.Add(Record);
                        }
                    }
                }
                else
                {
                    throw new AutomationException("Could not get changes; cmdline: p4 changes {0}", Cmd);
                }
            }
            else
            {
                throw new AutomationException("That CL looks pretty far off {0}", LastOutputForChanges);
            }
        }
        return Result;
    }

    int PrintChanges(int LastOutputForChanges, int TopCL, int LastGreen)
    {
        List<P4Connection.ChangeRecord> ChangeRecords = GetChanges(LastOutputForChanges, TopCL, LastGreen);
        foreach (P4Connection.ChangeRecord Record in ChangeRecords)
        {
            string Summary = Record.Summary.Replace("\r", "\n");
            if (Summary.IndexOf("\n") > 0)
            {
                Summary = Summary.Substring(0, Summary.IndexOf("\n"));
            }
            Log("         {0} {1} {2}", Record.CL, Record.UserEmail, Summary);
        }
        return TopCL;
    }

    void PrintDetailedChanges(NodeHistory History, bool bShowAllChanges = false)
    {
        DateTime StartTime = DateTime.UtcNow;

        string Me = String.Format("{0}   <<<< local sync", P4Env.Changelist);
        int LastOutputForChanges = 0;
        int LastGreen = History.LastSucceeded;
        if (bShowAllChanges)
        {
            if (History.AllStarted.Count > 0)
            {
                LastGreen = History.AllStarted[0];
            }
        }
        foreach (int cl in History.AllStarted)
        {
            if (cl < LastGreen)
            {
                continue;
            }
            if (P4Env.Changelist < cl && Me != "")
            {
                LastOutputForChanges = PrintChanges(LastOutputForChanges, P4Env.Changelist, LastGreen);
                Log("         {0}", Me);
                Me = "";
            }
            string Status = "In Process";
            if (History.AllSucceeded.Contains(cl))
            {
                Status = "ok";
            }
            if (History.AllFailed.Contains(cl))
            {
                Status = "FAIL";
            }
            LastOutputForChanges = PrintChanges(LastOutputForChanges, cl, LastGreen);
            Log("         {0}   {1}", cl, Status);
        }
        if (Me != "")
        {
            LastOutputForChanges = PrintChanges(LastOutputForChanges, P4Env.Changelist, LastGreen);
            Log("         {0}", Me);
        }
        double BuildDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds;
        Log("Took {0}s to get P4 history", BuildDuration / 1000);

    }
    void PrintNodes(GUBP bp, List<NodeInfo> Nodes, Dictionary<string, NodeHistory> GUBPNodesHistory, List<NodeInfo> UnfinishedTriggers = null)
    {
        bool bShowAllChanges = bp.ParseParam("AllChanges") && GUBPNodesHistory != null;
        bool bShowChanges = (bp.ParseParam("Changes") && GUBPNodesHistory != null) || bShowAllChanges;
        bool bShowDetailedHistory = (bp.ParseParam("History") && GUBPNodesHistory != null) || bShowChanges;
        bool bShowDependencies = bp.ParseParam("ShowDependencies");
        bool bShowECDependencies = bp.ParseParam("ShowECDependencies");
        bool bShowHistory = !bp.ParseParam("NoHistory") && GUBPNodesHistory != null;
        bool AddEmailProps = bp.ParseParam("ShowEmails");
        bool ECProc = bp.ParseParam("ShowECProc");
        bool ECOnly = bp.ParseParam("ShowECOnly");
        bool bShowTriggers = true;
        string LastControllingTrigger = "";
        string LastAgentGroup = "";
        foreach (NodeInfo NodeToDo in Nodes)
        {
            if (ECOnly && !NodeToDo.Node.RunInEC())
            {
                continue;
            }
            string EMails = "";
            if (AddEmailProps)
            {
                EMails = GetEMailListForNode(bp, NodeToDo, "", "");
            }
            if (bShowTriggers)
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
            }
            if (NodeToDo.Node.AgentSharingGroup != LastAgentGroup && NodeToDo.Node.AgentSharingGroup != "")
            {
                Log("    Agent Group: {0}", NodeToDo.Node.AgentSharingGroup);
            }
            LastAgentGroup = NodeToDo.Node.AgentSharingGroup;

            string Agent = NodeToDo.Node.ECAgentString();
			if(ParseParamValue("AgentOverride") != "" && !NodeToDo.Node.GetFullName().Contains("Mac"))
			{
				Agent = ParseParamValue("AgentOverride");
			}
            if (Agent != "")
            {
                Agent = "[" + Agent + "]";
            }
            string MemoryReq = "[" + NodeToDo.Node.AgentMemoryRequirement(bp).ToString() + "]";            
            if(MemoryReq == "[0]")
            {
                MemoryReq = "";                
            }
            string FrequencyString = CISFrequencyQuantumShiftString(NodeToDo);

            Log("      {0}{1}{2}{3}{4}{5}{6} {7}  {8}",
                (LastAgentGroup != "" ? "  " : ""),
                NodeToDo.Name,
                FrequencyString,
                NodeToDo.IsComplete ? " - (Completed)" : "",
                NodeToDo.Node.TriggerNode() ? " - (TriggerNode)" : "",
                NodeToDo.Node.IsSticky() ? " - (Sticky)" : "",				
                Agent,
                MemoryReq,
                EMails,
                ECProc ? NodeToDo.Node.ECProcedure() : ""
                );
            if (bShowHistory && GUBPNodesHistory.ContainsKey(NodeToDo.Name))
            {
                NodeHistory History = GUBPNodesHistory[NodeToDo.Name];

                if (bShowDetailedHistory)
                {
                    PrintDetailedChanges(History, bShowAllChanges);
                }
                else
                {
                    Log("         Last Success: {0}", History.LastSucceeded);
                    Log("         Last Fail   : {0}", History.LastFailed);
                    Log("          Fails Since: {0}", History.FailedString);
                    Log("     InProgress Since: {0}", History.InProgressString);					
                }
            }
            if (bShowDependencies)
            {
                foreach (NodeInfo Dep in NodeToDo.Dependencies)
                {
                    Log("            dep> {0}", Dep.Name);
                }
                foreach (NodeInfo Dep in NodeToDo.PseudoDependencies)
                {
                    Log("           pdep> {0}", Dep.Name);
                }
            }
            if (bShowECDependencies)
            {
                foreach (NodeInfo Dep in GetECDependencies(NodeToDo))
                {
                    Log("           {0}", Dep.Name);
                }
            }
        }
    }
    void SaveGraphVisualization(List<NodeInfo> Nodes)
    {
        List<GraphNode> GraphNodes = new List<GraphNode>();

        Dictionary<NodeInfo, GraphNode> NodeToGraphNodeMap = new Dictionary<NodeInfo, GraphNode>();

        for (int NodeIndex = 0; NodeIndex < Nodes.Count; ++NodeIndex)
        {
            NodeInfo Node = Nodes[NodeIndex];

            GraphNode GraphNode = new GraphNode()
            {
                Id = GraphNodes.Count,
                Label = Node.Name
            };
            GraphNodes.Add(GraphNode);
            NodeToGraphNodeMap.Add(Node, GraphNode);
        }

        // Connect everything together
        List<GraphEdge> GraphEdges = new List<GraphEdge>();

        for (int NodeIndex = 0; NodeIndex < Nodes.Count; ++NodeIndex)
        {
            NodeInfo Node = Nodes[NodeIndex];
            GraphNode NodeGraphNode = NodeToGraphNodeMap[Node];

            foreach (NodeInfo Dep in Node.Dependencies)
            {
                GraphNode PrerequisiteFileGraphNode;
                if (NodeToGraphNodeMap.TryGetValue(Dep, out PrerequisiteFileGraphNode))
                {
                    // Connect a file our action is dependent on, to our action itself
                    GraphEdge NewGraphEdge = new GraphEdge()
                    {
                        Id = GraphEdges.Count,
                        Source = PrerequisiteFileGraphNode,
                        Target = NodeGraphNode,
                        Color = new GraphColor() { R = 0.0f, G = 0.0f, B = 0.0f, A = 0.75f }
                    };

                    GraphEdges.Add(NewGraphEdge);
                }

            }
            foreach (NodeInfo Dep in Node.PseudoDependencies)
            {
                GraphNode PrerequisiteFileGraphNode;
                if (NodeToGraphNodeMap.TryGetValue(Dep, out PrerequisiteFileGraphNode))
                {
                    // Connect a file our action is dependent on, to our action itself
                    GraphEdge NewGraphEdge = new GraphEdge()
                    {
                        Id = GraphEdges.Count,
                        Source = PrerequisiteFileGraphNode,
                        Target = NodeGraphNode,
                        Color = new GraphColor() { R = 0.0f, G = 0.0f, B = 0.0f, A = 0.25f }
                    };

                    GraphEdges.Add(NewGraphEdge);
                }

            }
        }

        string Filename = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "GubpGraph.gexf");
        Log("Writing graph to {0}", Filename);
        GraphVisualization.WriteGraphFile(Filename, "GUBP Nodes", GraphNodes, GraphEdges);
        Log("Wrote graph to {0}", Filename);
     }


    // when the host is win64, this is win32 because those are also "host platforms"
    static public UnrealTargetPlatform GetAltHostPlatform(UnrealTargetPlatform HostPlatform)
    {
        UnrealTargetPlatform AltHostPlatform = UnrealTargetPlatform.Unknown; // when the host is win64, this is win32 because those are also "host platforms"
        if (HostPlatform == UnrealTargetPlatform.Win64)
        {
            AltHostPlatform = UnrealTargetPlatform.Win32;
        }
        return AltHostPlatform;
    }

    List<int> ConvertCLToIntList(List<string> Strings)
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
    void SaveStatus(NodeInfo NodeToDo, string Suffix, string NodeStoreName, bool bSaveSharedTempStorage, string GameNameIfAny, string JobStepIDForFailure = null)
    {
        string Contents = "Just a status record: " + Suffix;
        if (!String.IsNullOrEmpty(JobStepIDForFailure) && IsBuildMachine)
        {
            try
            {
                Contents = RunECTool(String.Format("getProperties --jobStepId {0} --recurse 1", JobStepIDForFailure), true);
            }
            catch (Exception Ex)
            {
                Log(System.Diagnostics.TraceEventType.Warning, "Failed to get properties for jobstep to save them.");
                Log(System.Diagnostics.TraceEventType.Warning, LogUtils.FormatException(Ex));
            }
        }
        string RecordOfSuccess = CombinePaths(CommandUtils.CmdEnv.LocalRoot, "Engine", "Saved", "Logs", NodeToDo.Name + Suffix +".log");
        CreateDirectory(Path.GetDirectoryName(RecordOfSuccess));
        WriteAllText(RecordOfSuccess, Contents);		
		TempStorage.StoreToTempStorage(CmdEnv, NodeStoreName + Suffix, new List<string> { RecordOfSuccess }, !bSaveSharedTempStorage, GameNameIfAny);		
    }
	string GetPropertyFromStep(string PropertyPath)
	{
		string Property = "";
		Property = RunECTool("getProperty \"" + PropertyPath + "\"");
		Property = Property.TrimEnd('\r', '\n');
		return Property;
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

    List<NodeInfo> TopologicalSort(HashSet<NodeInfo> NodesToDo, NodeInfo ExplicitTrigger, bool SubSort, bool DoNotConsiderCompletion)
    {
        DateTime StartTime = DateTime.UtcNow;

        List<NodeInfo> OrdereredToDo = new List<NodeInfo>();

        Dictionary<string, List<NodeInfo>> SortedAgentGroupChains = new Dictionary<string, List<NodeInfo>>();
        if (!SubSort)
        {
            Dictionary<string, List<NodeInfo>> AgentGroupChains = new Dictionary<string, List<NodeInfo>>();
            foreach (NodeInfo NodeToDo in NodesToDo)
            {
                string MyAgentGroup = NodeToDo.Node.AgentSharingGroup;
                if (MyAgentGroup != "")
                {
                    if (!AgentGroupChains.ContainsKey(MyAgentGroup))
                    {
                        AgentGroupChains.Add(MyAgentGroup, new List<NodeInfo> { NodeToDo });
                    }
                    else
                    {
                        AgentGroupChains[MyAgentGroup].Add(NodeToDo);
                    }
                }
            }
            foreach (KeyValuePair<string, List<NodeInfo>> Chain in AgentGroupChains)
            {
                SortedAgentGroupChains.Add(Chain.Key, TopologicalSort(new HashSet<NodeInfo>(Chain.Value), ExplicitTrigger, true, DoNotConsiderCompletion));
            }
            Log("***************Done with recursion");
        }

        // here we do a topological sort of the nodes, subject to a lexographical and priority sort
        while (NodesToDo.Count > 0)
        {
            bool bProgressMade = false;
            float BestPriority = -1E20f;
            NodeInfo BestNode = null;
            bool BestPseudoReady = false;
            HashSet<string> NonReadyAgentGroups = new HashSet<string>();
            HashSet<string> NonPeudoReadyAgentGroups = new HashSet<string>();
            HashSet<string> ExaminedAgentGroups = new HashSet<string>();
            foreach (NodeInfo NodeToDo in NodesToDo)
            {
                bool bReady = true;
				bool bPseudoReady = true;
				bool bCompReady = true;
                if (!SubSort && NodeToDo.Node.AgentSharingGroup != "")
                {
                    if (ExaminedAgentGroups.Contains(NodeToDo.Node.AgentSharingGroup))
                    {
                        bReady = !NonReadyAgentGroups.Contains(NodeToDo.Node.AgentSharingGroup);
                        bPseudoReady = !NonPeudoReadyAgentGroups.Contains(NodeToDo.Node.AgentSharingGroup); //this might not be accurate if bReady==false
                    }
                    else
                    {
                        ExaminedAgentGroups.Add(NodeToDo.Node.AgentSharingGroup);
                        foreach (NodeInfo ChainNode in SortedAgentGroupChains[NodeToDo.Node.AgentSharingGroup])
                        {
                            foreach (NodeInfo Dep in ChainNode.Dependencies)
                            {
                                if (!SortedAgentGroupChains[NodeToDo.Node.AgentSharingGroup].Contains(Dep) && NodesToDo.Contains(Dep))
                                {
                                    bReady = false;
                                    break;
                                }
                            }
                            if (!bReady)
                            {
                                NonReadyAgentGroups.Add(NodeToDo.Node.AgentSharingGroup);
                                break;
                            }
                            foreach (NodeInfo Dep in ChainNode.PseudoDependencies)
                            {
                                if (!SortedAgentGroupChains[NodeToDo.Node.AgentSharingGroup].Contains(Dep) && NodesToDo.Contains(Dep))
                                {
                                    bPseudoReady = false;
                                    NonPeudoReadyAgentGroups.Add(NodeToDo.Node.AgentSharingGroup);
                                    break;
                                }
                            }
                        }
                    }
                }
                else
                {
                    foreach (NodeInfo Dep in NodeToDo.Dependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            bReady = false;
                            break;
                        }
                    }
                    foreach (NodeInfo Dep in NodeToDo.PseudoDependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            bPseudoReady = false;
                            break;
                        }
                    }
                }
                float Priority = NodeToDo.Node.Priority();

                if (bReady && BestNode != null)
                {
                    if (String.Compare(BestNode.ControllingTriggerDotName, NodeToDo.ControllingTriggerDotName) < 0) //sorted by controlling trigger
                    {
                        bReady = false;
                    }
                    else if (String.Compare(BestNode.ControllingTriggerDotName, NodeToDo.ControllingTriggerDotName) == 0) //sorted by controlling trigger
                    {
                        if (BestNode.Node.IsSticky() && !NodeToDo.Node.IsSticky()) //sticky nodes first
                        {
                            bReady = false;
                        }
                        else if (BestNode.Node.IsSticky() == NodeToDo.Node.IsSticky())
                        {
                            if (BestPseudoReady && !bPseudoReady)
                            {
                                bReady = false;
                            }
                            else if (BestPseudoReady == bPseudoReady)
                            {
                                bool IamLateTrigger = !DoNotConsiderCompletion && NodeToDo.Node.TriggerNode() && NodeToDo != ExplicitTrigger && !NodeToDo.IsComplete;
                                bool BestIsLateTrigger = !DoNotConsiderCompletion && BestNode.Node.TriggerNode() && BestNode != ExplicitTrigger && !BestNode.IsComplete;
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
                if (!SubSort && BestNode.Node.AgentSharingGroup != "")
                {
                    foreach (NodeInfo ChainNode in SortedAgentGroupChains[BestNode.Node.AgentSharingGroup])
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
                foreach (NodeInfo NodeToDo in NodesToDo)
                {
                    string Deps = "";
                    if (!SubSort && NodeToDo.Node.AgentSharingGroup != "")
                    {
                        foreach (NodeInfo ChainNode in SortedAgentGroupChains[NodeToDo.Node.AgentSharingGroup])
                        {
                            foreach (NodeInfo Dep in ChainNode.Dependencies)
                            {
                                if (!SortedAgentGroupChains[NodeToDo.Node.AgentSharingGroup].Contains(Dep) && NodesToDo.Contains(Dep))
                                {
                                    Deps = Deps + Dep.Name + "[" + ChainNode.Name + "->" + NodeToDo.Node.AgentSharingGroup + "]" + " ";
                                }
                            }
                        }
                    }
                    foreach (NodeInfo Dep in NodeToDo.Dependencies)
                    {
                        if (NodesToDo.Contains(Dep))
                        {
                            Deps = Deps + Dep.Name + " ";
                        }
                    }
                    foreach (NodeInfo Dep in NodeToDo.PseudoDependencies)
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

    string GetJobStepPath(string Dep)
    {
        if (Dep != "Noop" && GUBPNodes[Dep].Node.AgentSharingGroup != "")
        {
            return "jobSteps[" + GUBPNodes[Dep].Node.AgentSharingGroup + "]/jobSteps[" + Dep + "]";
        }
        return "jobSteps[" + Dep + "]";
    }
    string GetJobStep(string ParentPath, string Dep)
    {
        return ParentPath + "/" + GetJobStepPath(Dep);
    }

    void UpdateNodeHistory(Dictionary<string, NodeHistory> GUBPNodesHistory, NodeInfo NodeToDo, string CLString, string StoreName)
    {
        if (NodeToDo.Node.RunInEC() && !NodeToDo.Node.TriggerNode() && CLString != "")
        {
            string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();
            string NodeStoreWildCard = StoreName.Replace(CLString, "*") + "-" + NodeToDo.Node.GetFullName();
            NodeHistory History = new NodeHistory();

            History.AllStarted = ConvertCLToIntList(TempStorage.FindTempStorageManifests(CmdEnv, NodeStoreWildCard + StartedTempStorageSuffix, false, true, GameNameIfAny));
            History.AllSucceeded = ConvertCLToIntList(TempStorage.FindTempStorageManifests(CmdEnv, NodeStoreWildCard + SucceededTempStorageSuffix, false, true, GameNameIfAny));
            History.AllFailed = ConvertCLToIntList(TempStorage.FindTempStorageManifests(CmdEnv, NodeStoreWildCard + FailedTempStorageSuffix, false, true, GameNameIfAny));

            if (History.AllFailed.Count > 0)
            {
                History.LastFailed = History.AllFailed[History.AllFailed.Count - 1];
            }
            if (History.AllSucceeded.Count > 0)
            {
                History.LastSucceeded = History.AllSucceeded[History.AllSucceeded.Count - 1];

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
			if (GUBPNodesHistory.ContainsKey(NodeToDo.Name))
			{
				GUBPNodesHistory.Remove(NodeToDo.Name);
			}
			GUBPNodesHistory.Add(NodeToDo.Name, History);
        }
    }
	void GetFailureEmails(Dictionary<string, NodeHistory> GUBPNodesHistory, NodeInfo NodeToDo, string CLString, string StoreName, bool OnlyLateUpdates = false)
	{
        string EMails;
        string FailCauserEMails = "";
        string EMailNote = "";
        bool SendSuccessForGreenAfterRed = false;
        int NumPeople = 0;
        if (GUBPNodesHistory.ContainsKey(NodeToDo.Name))
        {
            NodeHistory History = GUBPNodesHistory[NodeToDo.Name];
			RunECTool(String.Format("setProperty \"/myWorkflow/LastGreen/{0}\" \"{1}\"", NodeToDo.Name, History.LastSucceeded), true);
			RunECTool(String.Format("setProperty \"/myWorkflow/LastGreen/{0}\" \"{1}\"", NodeToDo.Name, History.FailedString), true);

            if (History.LastSucceeded > 0 && History.LastSucceeded < P4Env.Changelist)
            {
                int LastNonDuplicateFail = P4Env.Changelist;
                try
                {
                    if (OnlyLateUpdates)
                    {
                        LastNonDuplicateFail = FindLastNonDuplicateFail(GUBPNodesHistory, NodeToDo, CLString, StoreName);
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

                List<P4Connection.ChangeRecord> ChangeRecords = GetChanges(History.LastSucceeded, LastNonDuplicateFail, History.LastSucceeded);
                foreach (P4Connection.ChangeRecord Record in ChangeRecords)
                {
                    FailCauserEMails = GUBPNode.MergeSpaceStrings(FailCauserEMails, Record.UserEmail);
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
        else
        {
			RunECTool(String.Format("setProperty \"/myWorkflow/LastGreen/{0}\" \"{1}\"", NodeToDo.Name, "0"));
			RunECTool(String.Format("setProperty \"/myWorkflow/RedsSince/{0}\" \"{1}\"", NodeToDo.Name, ""));
        }
		RunECTool(String.Format("setProperty \"/myWorkflow/FailCausers/{0}\" \"{1}\"", NodeToDo.Name, FailCauserEMails));
		RunECTool(String.Format("setProperty \"/myWorkflow/EmailNotes/{0}\" \"{1}\"", NodeToDo.Name, EMailNote));
        {
            string AdditionalEmails = "";
			string Causers = "";
            if (ParseParam("CIS") && !NodeToDo.Node.SendSuccessEmail() && !NodeToDo.Node.TriggerNode())
            {
				Causers = FailCauserEMails;
           }
            string AddEmails = ParseParamValue("AddEmails");
            if (!String.IsNullOrEmpty(AddEmails))
            {
                AdditionalEmails = GUBPNode.MergeSpaceStrings(AddEmails, AdditionalEmails);
            }
			EMails = GetEMailListForNode(this, NodeToDo, AdditionalEmails, Causers);
			RunECTool(String.Format("setProperty \"/myWorkflow/FailEmails/{0}\" \"{1}\"", NodeToDo.Name, EMails));            
        }
		if (NodeToDo.Node.SendSuccessEmail() || SendSuccessForGreenAfterRed)
		{
			RunECTool(String.Format("setProperty \"/myWorkflow/SendSuccessEmail/{0}\" \"{1}\"", NodeToDo.Name, "1"));
		}
		else
		{
			RunECTool(String.Format("setProperty \"/myWorkflow/SendSuccessEmail/{0}\" \"{1}\"", NodeToDo.Name, "0"));
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

    int FindLastNonDuplicateFail(Dictionary<string, NodeHistory> GUBPNodesHistory, NodeInfo NodeToDo, string CLString, string StoreName)
    {
        NodeHistory History = GUBPNodesHistory[NodeToDo.Name];
        int Result = P4Env.Changelist;

        string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();
        string NodeStore = StoreName + "-" + NodeToDo.Node.GetFullName() + FailedTempStorageSuffix;

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
    List<string> GetECPropsForNode(NodeInfo NodeToDo, string CLString, out string EMails, bool OnlyLateUpdates = false)
    {
        List<string> ECProps = new List<string>();
        EMails = "";		
		string AdditonalEmails = "";
		string Causers = "";				
		string AddEmails = ParseParamValue("AddEmails");
		if (!String.IsNullOrEmpty(AddEmails))
		{
			AdditonalEmails = GUBPNode.MergeSpaceStrings(AddEmails, AdditonalEmails);
		}
		EMails = GetEMailListForNode(this, NodeToDo, AdditonalEmails, Causers);
		ECProps.Add("FailEmails/" + NodeToDo.Name + "=" + EMails);
	
		if (!OnlyLateUpdates)
		{
			string AgentReq = NodeToDo.Node.ECAgentString();
			if(ParseParamValue("AgentOverride") != "" && !NodeToDo.Node.GetFullName().Contains("OnMac"))
			{
				AgentReq = ParseParamValue("AgentOverride");
			}
			ECProps.Add(string.Format("AgentRequirementString/{0}={1}", NodeToDo.Name, AgentReq));
			ECProps.Add(string.Format("RequiredMemory/{0}={1}", NodeToDo.Name, NodeToDo.Node.AgentMemoryRequirement(this)));
			ECProps.Add(string.Format("Timeouts/{0}={1}", NodeToDo.Name, NodeToDo.Node.TimeoutInMinutes()));
			ECProps.Add(string.Format("JobStepPath/{0}={1}", NodeToDo.Name, GetJobStepPath(NodeToDo.Name)));
		}
		
        return ECProps;
    }

    void UpdateECProps(NodeInfo NodeToDo, string CLString)
    {
        try
        {
            Log("Updating node props for node {0}", NodeToDo.Name);
			string EMails = "";
            List<string> Props = GetECPropsForNode(NodeToDo, CLString, out EMails, true);
            foreach (string Prop in Props)
            {
                string[] Parts = Prop.Split("=".ToCharArray());
                RunECTool(String.Format("setProperty \"/myWorkflow/{0}\" \"{1}\"", Parts[0], Parts[1]), true);
            }			
        }
        catch (Exception Ex)
        {
            Log(System.Diagnostics.TraceEventType.Warning, "Failed to UpdateECProps.");
            Log(System.Diagnostics.TraceEventType.Warning, LogUtils.FormatException(Ex));
        }
    }
	void UpdateECBuildTime(NodeInfo NodeToDo, double BuildDuration)
	{
		try
		{
			Log("Updating duration prop for node {0}", NodeToDo.Name);
			RunECTool(String.Format("setProperty \"/myWorkflow/NodeDuration/{0}\" \"{1}\"", NodeToDo.Name, BuildDuration.ToString()));
			RunECTool(String.Format("setProperty \"/myJobStep/NodeDuration\" \"{0}\"", BuildDuration.ToString()));
		}
		catch (Exception Ex)
		{
			Log(System.Diagnostics.TraceEventType.Warning, "Failed to UpdateECBuildTime.");
			Log(System.Diagnostics.TraceEventType.Warning, LogUtils.FormatException(Ex));
		}
	}

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
    [Help("SetupNode=", "Like -Node, but only applies with CommanderJobSetupOnly")]
    [Help("RelatedToNode=", "Nodes to process, -RelatedToNode=Node1+Node2+Node3, use all nodes that either depend on these nodes or these nodes depend on them.")]
    [Help("SetupRelatedToNode=", "Like -RelatedToNode, but only applies with CommanderJobSetupOnly")]
    [Help("OnlyNode=", "Nodes to process NO dependencies, -OnlyNode=Node1+Node2+Node3, if no nodes or games are specified, defaults to all nodes.")]
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
    [Help("BuildRocket", "Build in rocket mode.")]
    [Help("ShowECOnly", "Only show EC nodes.")]
    [Help("ECProject", "From EC, the name of the project, used to get a version number.")]
    [Help("CIS", "This is a CIS run, assign TimeIndex based on the history.")]
    [Help("ForceIncrementalCompile", "make sure all compiles are incremental")]
    [Help("AutomatedTesting", "Allow automated testing, currently disabled.")]
    [Help("StompCheck", "Look for stomped build products.")]

    public override void ExecuteBuild()
    {
        Log("************************* GUBP");

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
        
        HostPlatforms = new List<UnrealTargetPlatform>();
        if (!ParseParam("NoPC"))
        {
            HostPlatforms.Add(UnrealTargetPlatform.Win64);
        }
        if (P4Enabled)
        {
            BranchName = P4Env.BuildRootP4;
        }
		else
		{ 
			BranchName = ParseParamValue("BranchName", "");
        }
        BranchOptions = GetBranchOptions(BranchName);
        bool WithMac = !BranchOptions.PlatformsToRemove.Contains(UnrealTargetPlatform.Mac);
        if (ParseParam("NoMac"))
        {
            WithMac = false;
        }
        if (WithMac)
        {
            HostPlatforms.Add(UnrealTargetPlatform.Mac);
        }

		bool WithLinux = !BranchOptions.PlatformsToRemove.Contains(UnrealTargetPlatform.Linux);
		bool WithoutLinux = ParseParam("NoLinux");
		// @TODO: exclude temporarily unless running on a Linux machine to prevent spurious GUBP failures
		if (UnrealBuildTool.BuildHostPlatform.Current.Platform != UnrealTargetPlatform.Linux || ParseParam("NoLinux"))
		{
			WithLinux = false;
		}
		if (WithLinux)
		{
			HostPlatforms.Add(UnrealTargetPlatform.Linux);
		}

        bForceIncrementalCompile = ParseParam("ForceIncrementalCompile");
        bool bNoAutomatedTesting = ParseParam("NoAutomatedTesting") || BranchOptions.bNoAutomatedTesting;		
        string StoreName = ParseParamValue("Store");
        string StoreSuffix = ParseParamValue("StoreSuffix", "");

        if (bPreflightBuild)
        {
            int PreflightUID = ParseParamInt("PreflightUID", 0);
            PreflightMangleSuffix = String.Format("-PF-{0}-{1}", PreflightShelveCL, PreflightUID);
            StoreSuffix = StoreSuffix + PreflightMangleSuffix;
        }
        CL = ParseParamInt("CL", 0);
        bool bCleanLocalTempStorage = ParseParam("CleanLocal");
        bool bChanges = ParseParam("Changes") || ParseParam("AllChanges");
        bool bHistory = ParseParam("History") || bChanges;
        bool bListOnly = ParseParam("ListOnly") || bHistory;
        bool bSkipTriggers = ParseParam("SkipTriggers");
        bool bFake = ParseParam("fake");
        bool bFakeEC = ParseParam("FakeEC");
        int TimeIndex = ParseParamInt("TimeIndex", 0);
        if (TimeIndex == 0)
        {
            TimeIndex = ParseParamInt("UserTimeIndex", 0);
        }

        bool bSaveSharedTempStorage = false;

        if (bHistory && !P4Enabled)
        {
            throw new AutomationException("-Changes and -History require -P4.");
        }
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
            bSignBuildProducts = true;
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
            ExplicitTriggerName = ParseParamValue("TriggerNode");
            if (ExplicitTriggerName == null)
            {
                ExplicitTriggerName = "";
            }
        }

        if (ParseParam("CIS") && ExplicitTriggerName == "" && CommanderSetup) // explicit triggers will already have a time index assigned
        {
			TimeIndex = UpdateCISCounter();
            Log("Setting TimeIndex to {0}", TimeIndex);
        }

        LogVerbose("************************* CL:			                    {0}", CL);
		LogVerbose("************************* P4Enabled:			            {0}", P4Enabled);
        foreach (UnrealTargetPlatform HostPlatform in HostPlatforms)
        {
			LogVerbose("*************************   HostPlatform:		            {0}", HostPlatform.ToString()); 
        }
		LogVerbose("************************* StoreName:		                {0}", StoreName.ToString());
		LogVerbose("************************* bCleanLocalTempStorage:		    {0}", bCleanLocalTempStorage);
		LogVerbose("************************* bSkipTriggers:		            {0}", bSkipTriggers);
		LogVerbose("************************* bSaveSharedTempStorage:		    {0}", bSaveSharedTempStorage);
		LogVerbose("************************* bSignBuildProducts:		        {0}", bSignBuildProducts);
		LogVerbose("************************* bFake:           		        {0}", bFake);
		LogVerbose("************************* bFakeEC:           		        {0}", bFakeEC);
		LogVerbose("************************* bHistory:           		        {0}", bHistory);
		LogVerbose("************************* TimeIndex:           		    {0}", TimeIndex);        


        GUBPNodes = new Dictionary<string, NodeInfo>();
		GUBPAggregates = new Dictionary<string, AggregateInfo>();
        Branch = new BranchInfo(HostPlatforms);        

		AddNodesForBranch(TimeIndex, bNoAutomatedTesting);

		LinkGraph(GUBPAggregates, GUBPNodes);
		FindControllingTriggers(GUBPNodes.Values);
		FindCompletionState(GUBPNodes.Values, StoreName, LocalOnly);

        if (bCleanLocalTempStorage)  // shared temp storage can never be wiped
        {
            TempStorage.DeleteLocalTempStorageManifests(CmdEnv);
        }

		// Compute all the frequencies
		Dictionary<NodeInfo, int> FrequencyOverrides = ApplyFrequencyBarriers(GUBPNodes, BranchOptions.FrequencyBarriers);
        foreach (KeyValuePair<string, NodeInfo> NodeToDo in GUBPNodes)
        {
            ComputeDependentCISFrequencyQuantumShift(NodeToDo.Value, FrequencyOverrides);
        }

		HashSet<NodeInfo> NodesToDo = ParseNodesToDo(WithoutLinux, TimeIndex, CommanderSetup);

		NodeInfo ExplicitTrigger = null;
		if (CommanderSetup)
        {
            if (!String.IsNullOrEmpty(ExplicitTriggerName))
            {
                foreach (KeyValuePair<string, NodeInfo> Node in GUBPNodes)
                {
                    if (Node.Value.Node.GetFullName().Equals(ExplicitTriggerName, StringComparison.InvariantCultureIgnoreCase))
                    {
                        if (Node.Value.Node.TriggerNode() && Node.Value.Node.RunInEC())
                        {
                            Node.Value.Node.SetAsExplicitTrigger();
							ExplicitTrigger = Node.Value;
                            break;
                        }
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
                    foreach (KeyValuePair<string, NodeInfo> Node in GUBPNodes)
                    {
                        if (Node.Value.Node.TriggerNode() && Node.Value.Node.RunInEC())
                        {
                            Node.Value.Node.SetAsExplicitTrigger();
                        }
                    }
                }
            }
        }

        Dictionary<string, NodeHistory> GUBPNodesHistory = new Dictionary<string, NodeHistory>();

        /*if (CLString != "" && StoreName.Contains(CLString) && !ParseParam("NoHistory"))
        {
            Log("******* Updating history");
            var StartTime = DateTime.UtcNow;
            foreach (var Node in NodesToDo)
            {
                if (!NodeIsAlreadyComplete(Node, LocalOnly))
                {
                    UpdateNodeHistory(Node, CLString);
                }				
            }
            var BuildDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds;
            Log("Took {0}s to get history for {1} nodes", BuildDuration / 1000, NodesToDo.Count);
        }*/

        List<NodeInfo> OrderedToDo = TopologicalSort(NodesToDo, ExplicitTrigger, false, false);

		List<NodeInfo> UnfinishedTriggers = FindUnfinishedTriggers(bSkipTriggers, ExplicitTrigger, OrderedToDo);

        LogVerbose("*********** Desired And Dependent Nodes, in order.");
        PrintNodes(this, OrderedToDo, GUBPNodesHistory, UnfinishedTriggers);		
        //check sorting
		CheckSortOrder(OrderedToDo);

        string FakeFail = ParseParamValue("FakeFail");
        if(CommanderSetup)
        {
			Dictionary<string, string> FullNodeDependedOnBy = GetFullNodeDependedOnBy(NodesToDo);

			List<AggregateInfo> SeparatePromotables = FindPromotables(GUBPAggregates.Values);
			Dictionary<NodeInfo, List<AggregateInfo>> DependentPromotions = FindDependentPromotables(GUBPNodes.Values, SeparatePromotables);

			Dictionary<string, string> FullNodeList;
			Dictionary<string, string> FullNodeDirectDependencies;
			GetFullNodeListAndDirectDependencies(out FullNodeList, out FullNodeDirectDependencies);

			Dictionary<string, int> FullNodeListSortKey = GetDisplayOrder(FullNodeList.Keys.ToList(), FullNodeDirectDependencies, GUBPNodes);

			DoCommanderSetup(TimeIndex, bSkipTriggers, bFake, bFakeEC, CLString, ExplicitTrigger, FullNodeList, FullNodeDirectDependencies, FullNodeDependedOnBy, DependentPromotions, SeparatePromotables, FullNodeListSortKey, GUBPNodesHistory, OrderedToDo, UnfinishedTriggers, FakeFail);
        }
		else if(ParseParam("SaveGraph"))
		{
			SaveGraphVisualization(OrderedToDo);
		}
		else if(!bListOnly)
		{
			ExecuteNodes(OrderedToDo, bFake, bFakeEC, bSaveSharedTempStorage, GUBPNodesHistory, CLString, StoreName, FakeFail);
		}
        PrintRunTime();
	}

	private static List<AggregateInfo> FindPromotables(IEnumerable<AggregateInfo> NodesToDo)
	{
		List<AggregateInfo> SeparatePromotables = new List<AggregateInfo>();
		foreach (AggregateInfo NodeToDo in NodesToDo)
		{
			if (NodeToDo.Node.IsSeparatePromotable())
			{
				SeparatePromotables.Add(NodeToDo);
			}
		}
		return SeparatePromotables;
	}

	private static Dictionary<NodeInfo, List<AggregateInfo>> FindDependentPromotables(IEnumerable<NodeInfo> NodesToDo, IEnumerable<AggregateInfo> SeparatePromotions)
	{
		Dictionary<NodeInfo, List<AggregateInfo>> DependentPromotions = NodesToDo.ToDictionary(x => x, x => new List<AggregateInfo>());
		foreach (AggregateInfo SeparatePromotion in SeparatePromotions)
		{
			NodeInfo[] Dependencies = SeparatePromotion.Dependencies.SelectMany(x => GetDependencies(x)).Distinct().ToArray();
			foreach (NodeInfo Dependency in Dependencies)
			{
				DependentPromotions[Dependency].Add(SeparatePromotion);
			}
		}
		return DependentPromotions;
	}

	private static void LinkGraph(Dictionary<string, AggregateInfo> AggregateNodes, Dictionary<string, NodeInfo> BuildNodes)
	{
		int NumErrors = 0;
		foreach (AggregateInfo AggregateNode in AggregateNodes.Values)
		{
			LinkAggregate(AggregateNode, AggregateNodes, BuildNodes, ref NumErrors);
		}
		foreach (NodeInfo BuildNode in BuildNodes.Values)
		{
			LinkNode(BuildNode, AggregateNodes, BuildNodes, ref NumErrors);
		}
		if(NumErrors > 0)
		{
			throw new AutomationException("Failed to link graph ({0} errors).", NumErrors);
		}
	}

	private static void LinkAggregate(AggregateInfo Aggregate, Dictionary<string, AggregateInfo> AggregateNodes, Dictionary<string, NodeInfo> BuildNodes, ref int NumErrors)
	{
		if (Aggregate.Dependencies == null)
		{
			Aggregate.Dependencies = new NodeInfo[0];

			HashSet<NodeInfo> Dependencies = new HashSet<NodeInfo>();
			foreach (string DependencyName in Aggregate.Node.Dependencies)
			{
				AggregateInfo AggregateDependency;
				if(AggregateNodes.TryGetValue(DependencyName, out AggregateDependency))
				{
					LinkAggregate(AggregateDependency, AggregateNodes, BuildNodes, ref NumErrors);
					Dependencies.UnionWith(AggregateDependency.Dependencies);
					continue;
				}

				NodeInfo Dependency;
				if(BuildNodes.TryGetValue(DependencyName, out Dependency))
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

	private static void LinkNode(NodeInfo BuildNode, Dictionary<string, AggregateInfo> GUBPAggregates, Dictionary<string, NodeInfo> GUBPNodes, ref int NumErrors)
	{
		// Find all the dependencies
		HashSet<NodeInfo> Dependencies = new HashSet<NodeInfo>();
		foreach (string DependencyName in BuildNode.Node.FullNamesOfDependencies)
		{
			if (!FindDependencies(DependencyName, GUBPAggregates, GUBPNodes, Dependencies))
			{
				CommandUtils.LogError("Node {0} is not in the graph. It is a dependency of {1}.", DependencyName, BuildNode.Name);
				NumErrors++;
			}
		}
		BuildNode.Dependencies = Dependencies.ToArray();

		// Find all the pseudo-dependencies
		HashSet<NodeInfo> PseudoDependencies = new HashSet<NodeInfo>();
		foreach (string PseudoDependencyName in BuildNode.Node.FullNamesOfPseudosependencies)
		{
			if (!FindDependencies(PseudoDependencyName, GUBPAggregates, GUBPNodes, PseudoDependencies))
			{
				CommandUtils.LogError("Node {0} is not in the graph. It is a pseudodependency of {1}.", PseudoDependencyName, BuildNode.Name);
				NumErrors++;
			}
		}
		BuildNode.PseudoDependencies = PseudoDependencies.ToArray();
	}

	private static bool FindDependencies(string Name, Dictionary<string, AggregateInfo> NameToAggregate, Dictionary<string, NodeInfo> NameToNode, HashSet<NodeInfo> Dependencies)
	{
		AggregateInfo AggregateDependency;
		if (NameToAggregate.TryGetValue(Name, out AggregateDependency))
		{
			Dependencies.UnionWith(AggregateDependency.Dependencies);
			return true;
		}

		NodeInfo NodeDependency;
		if (NameToNode.TryGetValue(Name, out NodeDependency))
		{
			Dependencies.Add(NodeDependency);
			return true;
		}

		return false;
	}

	private static Dictionary<NodeInfo, int> ApplyFrequencyBarriers(Dictionary<string, NodeInfo> GUBPNodes, Dictionary<string, sbyte> FrequencyBarriers)
	{
		// Make sure that everything that's listed as a frequency barrier is completed with the given interval
		Dictionary<NodeInfo, int> FrequencyOverrides = new Dictionary<NodeInfo, int>();
		foreach (KeyValuePair<string, sbyte> Barrier in FrequencyBarriers)
		{
			// Find the barrier node
			NodeInfo BarrierNode;
			if(!GUBPNodes.TryGetValue(Barrier.Key, out BarrierNode))
			{
				throw new AutomationException("Couldn't find node '{0}' for frequency barrier", Barrier.Key);
			}

			// All the nodes which are dependencies of the barrier node
			HashSet<NodeInfo> IncludedNodes = new HashSet<NodeInfo> { BarrierNode };

			// Find all the nodes which are indirect dependencies of this node
			List<NodeInfo> SearchNodes = new List<NodeInfo> { BarrierNode };
			for (int Idx = 0; Idx < SearchNodes.Count; Idx++)
			{
				NodeInfo Node = SearchNodes[Idx];
				foreach (NodeInfo Dependency in Node.Dependencies.Union(Node.PseudoDependencies))
				{
					if (!IncludedNodes.Contains(Dependency))
					{
						IncludedNodes.Add(Dependency);
						SearchNodes.Add(Dependency);
					}
				}
			}

			// Make sure that everything included in this list is before the cap, and everything not in the list is after it
			foreach (NodeInfo Node in GUBPNodes.Values)
			{
				if (IncludedNodes.Contains(Node))
				{
					int Frequency;
					if (FrequencyOverrides.TryGetValue(Node, out Frequency))
					{
						Frequency = Math.Min(Frequency, Barrier.Value);
					}
					else
					{
						Frequency = Barrier.Value;
					}
					FrequencyOverrides[Node] = Frequency;
				}
			}
		}
		return FrequencyOverrides;
	}

	private HashSet<NodeInfo> ParseNodesToDo(bool WithoutLinux, int TimeIndex, bool CommanderSetup)
	{
		HashSet<NodeInfo> NodesToDo = new HashSet<NodeInfo>();

		string NodeSpec = ParseParamValue("Node");
		if (!String.IsNullOrEmpty(NodeSpec))
		{
			List<string> Nodes = new List<string>(NodeSpec.Split('+'));
			foreach (string NodeArg in Nodes)
			{
				string NodeName = NodeArg.Trim();
				bool bFoundAnything = false;
				if (!String.IsNullOrEmpty(NodeName))
				{
					foreach (KeyValuePair<string, NodeInfo> Node in GUBPNodes)
					{
						if (Node.Value.Node.GetFullName().Equals(NodeArg, StringComparison.InvariantCultureIgnoreCase) ||
							Node.Value.Node.AgentSharingGroup.Equals(NodeArg, StringComparison.InvariantCultureIgnoreCase)
							)
						{
							if (!NodesToDo.Contains(Node.Value))
							{
								NodesToDo.Add(Node.Value);
							}
							bFoundAnything = true;
						}
					}
					foreach (KeyValuePair<string, AggregateInfo> AggregateNode in GUBPAggregates)
					{
						if(AggregateNode.Key.Equals(NodeArg, StringComparison.InvariantCultureIgnoreCase))
						{
							foreach(NodeInfo Node in AggregateNode.Value.Dependencies)
							{
								if(!NodesToDo.Contains(Node))
								{
									NodesToDo.Add(Node);
								}
								bFoundAnything = true;
							}
						}
					}
					if (!bFoundAnything)
					{
						throw new AutomationException("Could not find node named {0}", NodeName);
					}
				}
			}
		}

		string GameSpec = ParseParamValue("Game");
		if (!String.IsNullOrEmpty(GameSpec))
		{
			List<string> Games = new List<string>(GameSpec.Split('+'));
			foreach (string GameArg in Games)
			{
				string GameName = GameArg.Trim();
				if (!String.IsNullOrEmpty(GameName))
				{
					foreach (BranchInfo.BranchUProject GameProj in Branch.CodeProjects)
					{
						if (GameProj.GameName.Equals(GameName, StringComparison.InvariantCultureIgnoreCase))
						{
							if (GameProj.Options(UnrealTargetPlatform.Win64).bIsPromotable)
							{
								foreach (NodeInfo Dependency in GUBPAggregates[GameAggregatePromotableNode.StaticGetFullName(GameProj)].Dependencies)
								{
									NodesToDo.Add(Dependency);
								}
							}
							foreach (KeyValuePair<string, NodeInfo> Node in GUBPNodes)
							{
								if (Node.Value.Node.GameNameIfAnyForTempStorage() == GameProj.GameName)
								{
									NodesToDo.Add(Node.Value);
								}
							}

							GameName = null;
						}
					}
					if (GameName != null)
					{
						foreach (BranchInfo.BranchUProject GameProj in Branch.NonCodeProjects)
						{
							if (GameProj.GameName.Equals(GameName, StringComparison.InvariantCultureIgnoreCase))
							{
								foreach (KeyValuePair<string, NodeInfo> Node in GUBPNodes)
								{
									if (Node.Value.Node.GameNameIfAnyForTempStorage() == GameProj.GameName)
									{
										NodesToDo.Add(Node.Value);
									}
								}
								GameName = null;
							}
						}
					}
					if (GameName != null)
					{
						throw new AutomationException("Could not find game named {0}", GameName);
					}
				}
			}
		}

		if (NodesToDo.Count == 0)
		{
			LogVerbose("No nodes specified, adding all nodes");
			foreach (KeyValuePair<string, NodeInfo> Node in GUBPNodes)
			{
				NodesToDo.Add(Node.Value);
			}
		}
		else if (TimeIndex != 0)
		{
			LogVerbose("Check to make sure we didn't ask for nodes that will be culled by time index");
			foreach (NodeInfo NodeToDo in NodesToDo)
			{
				if (TimeIndex % (1 << NodeToDo.FrequencyShift) != 0)
				{
					throw new AutomationException("You asked specifically for node {0}, but it is culled by the time quantum: TimeIndex = {1}, DependentCISFrequencyQuantumShift = {2}.", NodeToDo.Name, TimeIndex, NodeToDo.FrequencyShift);
				}
			}
		}

		LogVerbose("Desired Nodes");
		foreach (NodeInfo NodeToDo in NodesToDo)
		{
			LogVerbose("  {0}", NodeToDo.Name);
		}

		// find things that our nodes depend on
		bool bDoneWithDependencies = false;
		while (!bDoneWithDependencies)
		{
			bDoneWithDependencies = true;
			HashSet<NodeInfo> Fringe = new HashSet<NodeInfo>();
			foreach (NodeInfo NodeToDo in NodesToDo)
			{
				if (BranchOptions.NodesToRemovePseudoDependencies.Count > 0)
				{
					List<string> ListOfRetainedNodesToRemovePseudoDependencies = BranchOptions.NodesToRemovePseudoDependencies;
					foreach (string NodeToRemovePseudoDependencies in BranchOptions.NodesToRemovePseudoDependencies)
					{
						if (NodeToDo.Name == NodeToRemovePseudoDependencies)
						{
							RemoveAllPseudodependenciesFromNode(NodeToDo.Name);
							ListOfRetainedNodesToRemovePseudoDependencies.Remove(NodeToDo.Name);
							break;
						}
					}
					BranchOptions.NodesToRemovePseudoDependencies = ListOfRetainedNodesToRemovePseudoDependencies;
				}
				foreach (NodeInfo Dep in NodeToDo.Dependencies)
				{
					if (!NodesToDo.Contains(Dep))
					{
						Fringe.Add(Dep);
						bDoneWithDependencies = false;
					}
				}
			}
			NodesToDo.UnionWith(Fringe);
		}

		if (TimeIndex != 0)
		{
			LogVerbose("Culling based on time index");
			HashSet<NodeInfo> NewNodesToDo = new HashSet<NodeInfo>();
			foreach (NodeInfo NodeToDo in NodesToDo)
			{
				if (TimeIndex % (1 << NodeToDo.FrequencyShift) == 0)
				{
					LogVerbose("  Keeping {0}", NodeToDo.Name);
					NewNodesToDo.Add(NodeToDo);
				}
				else
				{
					LogVerbose("  Rejecting {0}", NodeToDo.Name);
				}
			}
			NodesToDo = NewNodesToDo;
		}
		//Remove Plat if specified
		if (WithoutLinux)
		{
			HashSet<NodeInfo> NewNodesToDo = new HashSet<NodeInfo>();
			foreach (NodeInfo NodeToDo in NodesToDo)
			{
				if (!NodeToDo.Node.GetFullName().Contains("Linux"))
				{
					NewNodesToDo.Add(NodeToDo);
				}
				else
				{
					LogVerbose(" Rejecting {0} because -NoLinux was requested", NodeToDo);
				}
			}
			NodesToDo = NewNodesToDo;
		}
		if (bPreflightBuild)
		{
			LogVerbose("Culling triggers and downstream for preflight builds ");
			HashSet<NodeInfo> NewNodesToDo = new HashSet<NodeInfo>();
			foreach (NodeInfo NodeToDo in NodesToDo)
			{
				string TriggerDot = NodeToDo.ControllingTriggerDotName;
				if (TriggerDot == "" && !NodeToDo.Node.TriggerNode())
				{
					LogVerbose("  Keeping {0}", NodeToDo.Name);
					NewNodesToDo.Add(NodeToDo);
				}
				else
				{
					LogVerbose("  Rejecting {0}", NodeToDo.Name);
				}
			}
			NodesToDo = NewNodesToDo;
		}
		return NodesToDo;
	}

	private static Dictionary<string, string> GetFullNodeDependedOnBy(HashSet<NodeInfo> NodesToDo)
	{
		//find things that depend on our nodes and setup commander dictionary
		Dictionary<string, string> FullNodeDependedOnBy = new Dictionary<string, string>();
		foreach (NodeInfo NodeToDo in NodesToDo)
		{
			FullNodeDependedOnBy[NodeToDo.Name] = "";
		}
		foreach (NodeInfo NodeToDo in NodesToDo)
		{
			if (!NodeToDo.Node.IsAggregate() && !NodeToDo.Node.IsTest())
			{
				List<NodeInfo> ECDependencies = GetECDependencies(NodeToDo);
				foreach (NodeInfo Dep in ECDependencies)
				{
					string CurrentValue;
					if (!FullNodeDependedOnBy.TryGetValue(Dep.Name, out CurrentValue) || CurrentValue.Length == 0)
					{
						FullNodeDependedOnBy[Dep.Name] = NodeToDo.Name;
					}
					else
					{
						FullNodeDependedOnBy[Dep.Name] += " " + NodeToDo.Name;
					}
				}
			}
		}
		return FullNodeDependedOnBy;
	}

	private void GetFullNodeListAndDirectDependencies(out Dictionary<string, string> FullNodeList, out Dictionary<string, string> FullNodeDirectDependencies)
	{
		using(TelemetryStopwatch Timer = new TelemetryStopwatch("GetDependencies"))
		{
			FullNodeList = new Dictionary<string,string>();
			FullNodeDirectDependencies = new Dictionary<string,string>();

			Log("******* {0} GUBP Nodes", GUBPNodes.Count);
			List<NodeInfo> SortedNodes = TopologicalSort(new HashSet<NodeInfo>(GUBPNodes.Values), null, SubSort: false, DoNotConsiderCompletion: true);
			foreach (NodeInfo Node in SortedNodes)
			{
				string Note = Node.ControllingTriggerDotName;
				if (Note == "")
				{
					Note = CISFrequencyQuantumShiftString(Node);
				}
				if (Note == "")
				{
					Note = "always";
				}
				if (GUBPNodes[Node.Name].Node.RunInEC())
				{
					List<NodeInfo> Deps = GetECDependencies(Node);
					string All = "";
					foreach (NodeInfo Dep in Deps)
					{
						if (All != "")
						{
							All += " ";
						}
						All += Dep.Name;
					}
					LogVerbose("  {0}: {1}      {2}", Node.Name, Note, All);
					FullNodeList.Add(Node.Name, Note);
					FullNodeDirectDependencies.Add(Node.Name, All);
				}
				else
				{
					LogVerbose("  {0}: {1} [Aggregate]", Node.Name, Note);
				}
			}
		}
	}

	private int UpdateCISCounter()
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

			int TimeQuantum = 20;
			if (BranchOptions.QuantumOverride != 0)
			{
				TimeQuantum = BranchOptions.QuantumOverride;
			}
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

	private List<NodeInfo> FindUnfinishedTriggers(bool bSkipTriggers, NodeInfo ExplicitTrigger, List<NodeInfo> OrdereredToDo)
	{
		// find all unfinished triggers, excepting the one we are triggering right now
		List<NodeInfo> UnfinishedTriggers = new List<NodeInfo>();
		if (!bSkipTriggers)
		{
			foreach (NodeInfo NodeToDo in OrdereredToDo)
			{
				if (NodeToDo.Node.TriggerNode() && !NodeToDo.IsComplete)
				{
					if (ExplicitTrigger != NodeToDo)
					{
						UnfinishedTriggers.Add(NodeToDo);
					}
				}
			}
		}
		return UnfinishedTriggers;
	}

	private void CheckSortOrder(List<NodeInfo> OrdereredToDo)
	{
		foreach (NodeInfo NodeToDo in OrdereredToDo)
		{
			if (NodeToDo.Node.TriggerNode() && (NodeToDo.Node.IsSticky() || NodeToDo.IsComplete)) // these sticky triggers are ok, everything is already completed anyway
			{
				continue;
			}
			int MyIndex = OrdereredToDo.IndexOf(NodeToDo);
			foreach (NodeInfo Dep in NodeToDo.Dependencies)
			{
				int DepIndex = OrdereredToDo.IndexOf(Dep);
				if (DepIndex >= MyIndex)
				{
					throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
				}
			}
			foreach (NodeInfo Dep in NodeToDo.PseudoDependencies)
			{
				int DepIndex = OrdereredToDo.IndexOf(Dep);
				if (DepIndex >= MyIndex)
				{
					throw new AutomationException("Topological sort error, node {0} has a pseduodependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
				}
			}
		}
	}

	private void DoCommanderSetup(int TimeIndex, bool bSkipTriggers, bool bFake, bool bFakeEC, string CLString, NodeInfo ExplicitTrigger, Dictionary<string, string> FullNodeList, Dictionary<string, string> FullNodeDirectDependencies, Dictionary<string, string> FullNodeDependedOnBy, Dictionary<NodeInfo, List<AggregateInfo>> DependentPromotions, List<AggregateInfo> SeparatePromotables, Dictionary<string, int> FullNodeListSortKey, Dictionary<string, NodeHistory> GUBPNodesHistory, List<NodeInfo> OrdereredToDo, List<NodeInfo> UnfinishedTriggers, string FakeFail)
	{

		if (OrdereredToDo.Count == 0)
		{
			throw new AutomationException("No nodes to do!");
		}
		List<string> ECProps = new List<string>();
		ECProps.Add(String.Format("TimeIndex={0}", TimeIndex));
		foreach (KeyValuePair<string, string> NodePair in FullNodeList)
		{
			ECProps.Add(string.Format("AllNodes/{0}={1}", NodePair.Key, NodePair.Value));
		}
		foreach (KeyValuePair<string, string> NodePair in FullNodeDirectDependencies)
		{
			ECProps.Add(string.Format("DirectDependencies/{0}={1}", NodePair.Key, NodePair.Value));
		}
		foreach (KeyValuePair<string, int> NodePair in FullNodeListSortKey)
		{
			ECProps.Add(string.Format("SortKey/{0}={1}", NodePair.Key, NodePair.Value));
		}
		foreach (KeyValuePair<string, string> NodePair in FullNodeDependedOnBy)
		{
			ECProps.Add(string.Format("DependedOnBy/{0}={1}", NodePair.Key, NodePair.Value));
		}
		foreach (KeyValuePair<NodeInfo, List<AggregateInfo>> NodePair in DependentPromotions)
		{
			ECProps.Add(string.Format("DependentPromotions/{0}={1}", NodePair.Key.Name, String.Join(" ", NodePair.Value.Select(x => x.Name))));
		}
		foreach (AggregateInfo Node in SeparatePromotables)
		{
			ECProps.Add(string.Format("PossiblePromotables/{0}={1}", Node.Name, ""));
		}
		List<string> ECJobProps = new List<string>();
		if (ExplicitTrigger != null)
		{
			ECJobProps.Add("IsRoot=0");
		}
		else
		{
			ECJobProps.Add("IsRoot=1");
		}

		using(TelemetryStopwatch StartFilterTimer = new TelemetryStopwatch("FilterNodes"))
		{
			List<NodeInfo> FilteredOrdereredToDo = new List<NodeInfo>();
			// remove nodes that have unfinished triggers
			foreach (NodeInfo NodeToDo in OrdereredToDo)
			{
				NodeInfo ControllingTrigger = (NodeToDo.ControllingTriggers.Length > 0)? NodeToDo.ControllingTriggers.Last() : null;
				bool bNoUnfinishedTriggers = !UnfinishedTriggers.Contains(ControllingTrigger);

				if (bNoUnfinishedTriggers)
				{
					// if we are triggering, then remove nodes that are not controlled by the trigger or are dependencies of this trigger
					if (ExplicitTrigger != null)
					{
						if (ExplicitTrigger != NodeToDo && !NodeDependsOn(NodeToDo, ExplicitTrigger) && !NodeDependsOn(ExplicitTrigger, NodeToDo))
						{
							continue; // this wasn't on the chain related to the trigger we are triggering, so it is not relevant
						}
					}
					if (bPreflightBuild && !bSkipTriggers && NodeToDo.Node.TriggerNode())
					{
						// in preflight builds, we are either skipping triggers (and running things downstream) or we just stop at triggers and don't make them available for triggering.
						continue;
					}
					FilteredOrdereredToDo.Add(NodeToDo);
				}
			}
			OrdereredToDo = FilteredOrdereredToDo;
		}
		using(TelemetryStopwatch PrintNodesTimer = new TelemetryStopwatch("SetupCommanderPrint"))
		{
			Log("*********** EC Nodes, in order.");
			PrintNodes(this, OrdereredToDo, GUBPNodesHistory, UnfinishedTriggers);
		}
		// here we are just making sure everything before the explicit trigger is completed.
		if (ExplicitTrigger != null)
		{
			foreach (NodeInfo NodeToDo in OrdereredToDo)
			{
				if (NodeToDo.Node.RunInEC() && !NodeToDo.IsComplete && NodeToDo != ExplicitTrigger && !NodeDependsOn(ExplicitTrigger, NodeToDo)) // if something is already finished, we don't put it into EC
				{
					throw new AutomationException("We are being asked to process node {0}, however, this is an explicit trigger {1}, so everything before it should already be handled. It seems likely that you waited too long to run the trigger. You will have to do a new build from scratch.", NodeToDo.Name, ExplicitTrigger.Name);
				}
			}
		}

		NodeInfo LastSticky = null;
		bool HitNonSticky = false;
		bool bHaveECNodes = false;
		List<string> StepList = new List<string>();
		StepList.Add("use strict;");
		StepList.Add("use diagnostics;");
		StepList.Add("use ElectricCommander();");
		StepList.Add("my $ec = new ElectricCommander;");
		StepList.Add("$ec->setTimeout(600);");
		StepList.Add("my $batch = $ec->newBatch(\"serial\");");
		// sticky nodes are ones that we run on the main agent. We run then first and they must not be intermixed with parallel jobs
		foreach (NodeInfo NodeToDo in OrdereredToDo)
		{
			if (NodeToDo.Node.RunInEC() && !NodeToDo.IsComplete) // if something is already finished, we don't put it into EC
			{
				bHaveECNodes = true;
				if (NodeToDo.Node.IsSticky())
				{
					LastSticky = NodeToDo;
					if (HitNonSticky && !bSkipTriggers)
					{
						throw new AutomationException("Sticky and non-sticky jobs did not sort right.");
					}
				}
				else
				{
					HitNonSticky = true;
				}
			}
		}

		List<string> FakeECArgs = new List<string>();
		using(TelemetryStopwatch PerlOutputStopwatch = new TelemetryStopwatch("PerlOutput"))
		{
			string ParentPath = ParseParamValue("ParentPath");
			string BaseArgs = String.Format("$batch->createJobStep({{parentPath => '{0}'", ParentPath);

			bool bHasNoop = false;
			if (LastSticky == null && bHaveECNodes)
			{
				// if we don't have any sticky nodes and we have other nodes, we run a fake noop just to release the resource 
				string Args = String.Format("{0}, subprocedure => 'GUBP_UAT_Node', parallel => '0', jobStepName => 'Noop', actualParameter => [{{actualParameterName => 'NodeName', value => 'Noop'}}, {{actualParameterName => 'Sticky', value =>'1' }}], releaseMode => 'release'}});", BaseArgs);
				StepList.Add(Args);
				bHasNoop = true;
			}

			Dictionary<string, List<NodeInfo>> AgentGroupChains = new Dictionary<string, List<NodeInfo>>();
			List<NodeInfo> StickyChain = new List<NodeInfo>();
			foreach (NodeInfo NodeToDo in OrdereredToDo)
			{
				if (NodeToDo.Node.RunInEC() && !NodeToDo.IsComplete) // if something is already finished, we don't put it into EC  
				{
					string MyAgentGroup = NodeToDo.Node.AgentSharingGroup;
					if (MyAgentGroup != "")
					{
						if (!AgentGroupChains.ContainsKey(MyAgentGroup))
						{
							AgentGroupChains.Add(MyAgentGroup, new List<NodeInfo> { NodeToDo });
						}
						else
						{
							AgentGroupChains[MyAgentGroup].Add(NodeToDo);
						}
					}
				}
				if (NodeToDo.Node.IsSticky())
				{
					if (!StickyChain.Contains(NodeToDo))
					{
						StickyChain.Add(NodeToDo);
					}
				}
			}
			foreach (NodeInfo NodeToDo in OrdereredToDo)
			{
				if (NodeToDo.Node.RunInEC() && !NodeToDo.IsComplete) // if something is already finished, we don't put it into EC  
				{
					string EMails;
					List<string> NodeProps = GetECPropsForNode(NodeToDo, CLString, out EMails);
					ECProps.AddRange(NodeProps);

					bool Sticky = NodeToDo.Node.IsSticky();
					bool DoParallel = !Sticky;
					if (NodeToDo.Node.ECProcedure() == "GUBP_UAT_Node_Parallel_AgentShare_Editor")
					{
						DoParallel = true;
					}
					if (Sticky && NodeToDo.Node.ECAgentString() != "")
					{
						throw new AutomationException("Node {1} is sticky but has agent requirements.", NodeToDo.Name);
					}
					string Procedure = NodeToDo.Node.ECProcedure();
					if (NodeToDo.Node.IsSticky() && NodeToDo == LastSticky)
					{
						Procedure = Procedure + "_Release";
					}
					string Args = String.Format("{0}, subprocedure => '{1}', parallel => '{2}', jobStepName => '{3}', actualParameter => [{{actualParameterName => 'NodeName', value =>'{4}'}}",
						BaseArgs, Procedure, DoParallel ? 1 : 0, NodeToDo.Name, NodeToDo.Name);
					string ProcedureParams = NodeToDo.Node.ECProcedureParams();
					if (!String.IsNullOrEmpty(ProcedureParams))
					{
						Args = Args + ProcedureParams;
					}

					if ((Procedure == "GUBP_UAT_Trigger" || Procedure == "GUBP_Hardcoded_Trigger") && !String.IsNullOrEmpty(EMails))
					{
						Args = Args + ", {actualParameterName => 'EmailsForTrigger', value => \'" + EMails + "\'}";
					}
					Args = Args + "]";
					string PreCondition = "";
					string RunCondition = "";
					List<string> UncompletedEcDeps = new List<string>();
					List<string> PreConditionUncompletedEcDeps = new List<string>();
					string MyAgentGroup = NodeToDo.Node.AgentSharingGroup;
					bool bDoNestedJobstep = false;
					bool bDoFirstNestedJobstep = false;


					string NodeParentPath = ParentPath;
					string PreconditionParentPath;

					PreconditionParentPath = ParentPath;
					{
						List<NodeInfo> EcDeps = GetECDependencies(NodeToDo);
						foreach (NodeInfo Dep in EcDeps)
						{
							if (Dep.Node.RunInEC() && !Dep.IsComplete && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
							{
								if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(NodeToDo))
								{
									throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
								}
								UncompletedEcDeps.Add(Dep.Name);
							}
						}
					}

					foreach (string Dep in UncompletedEcDeps)
					{
						PreConditionUncompletedEcDeps.Add(Dep);
					}
					if (MyAgentGroup != "")
					{
						bDoNestedJobstep = true;
						NodeParentPath = ParentPath + "/jobSteps[" + MyAgentGroup + "]";

						PreConditionUncompletedEcDeps = new List<string>();

						List<NodeInfo> MyChain = AgentGroupChains[MyAgentGroup];
						int MyIndex = MyChain.IndexOf(NodeToDo);
						if (MyIndex > 0)
						{
							PreConditionUncompletedEcDeps.Add(MyChain[MyIndex - 1].Name);
						}
						else
						{
							bDoFirstNestedJobstep = bDoNestedJobstep;
							// to avoid idle agents (and also EC doesn't actually reserve our agent!), we promote all dependencies to the first one
							foreach (NodeInfo Chain in MyChain)
							{
								List<NodeInfo> EcDeps = GetECDependencies(Chain);
								foreach (NodeInfo Dep in EcDeps)
								{
									if (GUBPNodes[Dep.Name].Node.RunInEC() && !Dep.IsComplete && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
									{
										if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(Chain))
										{
											throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", Chain.Name, Dep.Name);
										}
										if (!MyChain.Contains(Dep) && !PreConditionUncompletedEcDeps.Contains(Dep.Name))
										{
											PreConditionUncompletedEcDeps.Add(Dep.Name);
										}
									}
								}
							}
						}
					}
					if (NodeToDo.Node.IsSticky())
					{
						List<NodeInfo> MyChain = StickyChain;
						int MyIndex = MyChain.IndexOf(NodeToDo);
						if (MyIndex > 0)
						{
							if (!PreConditionUncompletedEcDeps.Contains(MyChain[MyIndex - 1].Name) && !MyChain[MyIndex - 1].IsComplete)
							{
								PreConditionUncompletedEcDeps.Add(MyChain[MyIndex - 1].Name);
							}
						}
						else
						{
							List<NodeInfo> EcDeps = GetECDependencies(NodeToDo);
							foreach (NodeInfo Dep in EcDeps)
							{
								if (Dep.Node.RunInEC() && !Dep.IsComplete && OrdereredToDo.Contains(Dep)) // if something is already finished, we don't put it into EC
								{
									if (OrdereredToDo.IndexOf(Dep) > OrdereredToDo.IndexOf(NodeToDo))
									{
										throw new AutomationException("Topological sort error, node {0} has a dependency of {1} which sorted after it.", NodeToDo.Name, Dep.Name);
									}
									if (!MyChain.Contains(Dep) && !PreConditionUncompletedEcDeps.Contains(Dep.Name))
									{
										PreConditionUncompletedEcDeps.Add(Dep.Name);
									}
								}
							}
						}
					}
					if (bHasNoop && PreConditionUncompletedEcDeps.Count == 0)
					{
						PreConditionUncompletedEcDeps.Add("Noop");
					}
					if (PreConditionUncompletedEcDeps.Count > 0)
					{
						PreCondition = "\"\\$\" . \"[/javascript if(";
						// these run "parallel", but we add preconditions to serialize them
						int Index = 0;
						foreach (string Dep in PreConditionUncompletedEcDeps)
						{
							PreCondition = PreCondition + "getProperty('" + GetJobStep(PreconditionParentPath, Dep) + "/status\') == \'completed\'";
							Index++;
							if (Index != PreConditionUncompletedEcDeps.Count)
							{
								PreCondition = PreCondition + " && ";
							}
						}
						PreCondition = PreCondition + ") true;]\"";
					}

					if (UncompletedEcDeps.Count > 0)
					{
						RunCondition = "\"\\$\" . \"[/javascript if(";
						int Index = 0;
						foreach (string Dep in UncompletedEcDeps)
						{
							RunCondition = RunCondition + "((\'\\$\" . \"[" + GetJobStep(PreconditionParentPath, Dep) + "/outcome]\' == \'success\') || ";
							RunCondition = RunCondition + "(\'\\$\" . \"[" + GetJobStep(PreconditionParentPath, Dep) + "/outcome]\' == \'warning\'))";

							Index++;
							if (Index != UncompletedEcDeps.Count)
							{
								RunCondition = RunCondition + " && ";
							}
						}
						RunCondition = RunCondition + ")true; else false;]\"";
					}

					if (bDoNestedJobstep)
					{
						if (bDoFirstNestedJobstep)
						{
							{
								string NestArgs = String.Format("$batch->createJobStep({{parentPath => '{0}', jobStepName => '{1}', parallel => '1'",
									ParentPath, MyAgentGroup);
								if (!String.IsNullOrEmpty(PreCondition))
								{
									NestArgs = NestArgs + ", precondition => " + PreCondition;
								}
								NestArgs = NestArgs + "});";
								StepList.Add(NestArgs);
							}
							{
								string NestArgs = String.Format("$batch->createJobStep({{parentPath => '{0}/jobSteps[{1}]', jobStepName => '{2}_GetPool', subprocedure => 'GUBP{3}_AgentShare_GetPool', parallel => '1', actualParameter => [{{actualParameterName => 'AgentSharingGroup', value => '{4}'}}, {{actualParameterName => 'NodeName', value => '{5}'}}]",
									ParentPath, MyAgentGroup, MyAgentGroup, NodeToDo.Node.ECProcedureInfix(), MyAgentGroup, NodeToDo.Name);
								if (!String.IsNullOrEmpty(PreCondition))
								{
									NestArgs = NestArgs + ", precondition => " + PreCondition;
								}
								NestArgs = NestArgs + "});";
								StepList.Add(NestArgs);
							}
							{
								string NestArgs = String.Format("$batch->createJobStep({{parentPath => '{0}/jobSteps[{1}]', jobStepName => '{2}_GetAgent', subprocedure => 'GUBP{3}_AgentShare_GetAgent', parallel => '1', exclusiveMode => 'call', resourceName => '{4}', actualParameter => [{{actualParameterName => 'AgentSharingGroup', value => '{5}'}}, {{actualParameterName => 'NodeName', value=> '{6}'}}]",
									ParentPath, MyAgentGroup, MyAgentGroup, NodeToDo.Node.ECProcedureInfix(),
									String.Format("$[/myJob/jobSteps[{0}]/ResourcePool]", MyAgentGroup),
									MyAgentGroup, NodeToDo.Name);
								{
									NestArgs = NestArgs + ", precondition  => ";
									NestArgs = NestArgs + "\"\\$\" . \"[/javascript if(";
									NestArgs = NestArgs + "getProperty('" + PreconditionParentPath + "/jobSteps[" + MyAgentGroup + "]/jobSteps[" + MyAgentGroup + "_GetPool]/status') == 'completed'";
									NestArgs = NestArgs + ") true;]\"";
								}
								NestArgs = NestArgs + "});";
								StepList.Add(NestArgs);
							}
							{
								PreCondition = "\"\\$\" . \"[/javascript if(";
								PreCondition = PreCondition + "getProperty('" + PreconditionParentPath + "/jobSteps[" + MyAgentGroup + "]/jobSteps[" + MyAgentGroup + "_GetAgent]/status') == 'completed'";
								PreCondition = PreCondition + ") true;]\"";
							}
						}
						Args = Args.Replace(String.Format("parentPath => '{0}'", ParentPath), String.Format("parentPath => '{0}'", NodeParentPath));
						Args = Args.Replace("UAT_Node_Parallel_AgentShare", "UAT_Node_Parallel_AgentShare3");
					}

					if (!String.IsNullOrEmpty(PreCondition))
					{
						Args = Args + ", precondition => " + PreCondition;
					}
					if (!String.IsNullOrEmpty(RunCondition))
					{
						Args = Args + ", condition => " + RunCondition;
					}
	#if false
						// this doesn't work because it includes precondition time
						if (GUBPNodes[NodeToDo].TimeoutInMinutes() > 0)
						{
							Args = Args + String.Format(" --timeLimitUnits minutes --timeLimit {0}", GUBPNodes[NodeToDo].TimeoutInMinutes());
						}
	#endif
					if (Sticky && NodeToDo == LastSticky)
					{
						Args = Args + ", releaseMode => 'release'";
					}
					Args = Args + "});";
					StepList.Add(Args);
					if (bFakeEC &&
						!UnfinishedTriggers.Contains(NodeToDo) &&
						(NodeToDo.Node.ECProcedure().StartsWith("GUBP_UAT_Node") || NodeToDo.Node.ECProcedure().StartsWith("GUBP_Mac_UAT_Node")) // other things we really can't test
						) // unfinished triggers are never run directly by EC, rather it does another job setup
					{
						string Arg = String.Format("gubp -Node={0} -FakeEC {1} {2} {3} {4} {5}",
							NodeToDo,
							bFake ? "-Fake" : "",
							ParseParam("AllPlatforms") ? "-AllPlatforms" : "",
							ParseParam("UnfinishedTriggersFirst") ? "-UnfinishedTriggersFirst" : "",
							ParseParam("UnfinishedTriggersParallel") ? "-UnfinishedTriggersParallel" : "",
							ParseParam("WithMac") ? "-WithMac" : ""
							);

						string Node = ParseParamValue("-Node");
						if (!String.IsNullOrEmpty(Node))
						{
							Arg = Arg + " -Node=" + Node;
						}
						if (!String.IsNullOrEmpty(FakeFail))
						{
							Arg = Arg + " -FakeFail=" + FakeFail;
						}
						FakeECArgs.Add(Arg);
					}

					if (MyAgentGroup != "" && !bDoNestedJobstep)
					{
						List<NodeInfo> MyChain = AgentGroupChains[MyAgentGroup];
						int MyIndex = MyChain.IndexOf(NodeToDo);
						if (MyIndex == MyChain.Count - 1)
						{
							string RelPreCondition = "\"\\$\" . \"[/javascript if(";
							// this runs "parallel", but we a precondition to serialize it
							RelPreCondition = RelPreCondition + "getProperty('" + PreconditionParentPath + "/jobSteps[" + NodeToDo.Name + "]/status') == 'completed'";
							RelPreCondition = RelPreCondition + ") true;]\"";
							// we need to release the resource
							string RelArgs = String.Format("{0}, subprocedure => 'GUBP_Release_AgentShare', parallel => '1', jobStepName => 'Release_{1}', actualParameter => [{{actualParameterName => 'AgentSharingGroup', valued => '{2}'}}], releaseMode => 'release', precondition => '{3}'",
								BaseArgs, MyAgentGroup, MyAgentGroup, RelPreCondition);
							StepList.Add(RelArgs);
						}
					}
				}
			}
			WriteECPerl(StepList);
		}
		bool bHasTests = OrdereredToDo.Any(x => x.Node.IsTest());
		RunECTool(String.Format("setProperty \"/myWorkflow/HasTests\" \"{0}\"", bHasTests));
		{
			ECProps.Add("GUBP_LoadedProps=1");
			string BranchDefFile = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "BranchDef.properties");
			CommandUtils.WriteAllLines(BranchDefFile, ECProps.ToArray());
			RunECTool(String.Format("setProperty \"/myWorkflow/BranchDefFile\" \"{0}\"", BranchDefFile.Replace("\\", "\\\\")));
		}
		{
			ECProps.Add("GUBP_LoadedJobProps=1");
			string BranchJobDefFile = CommandUtils.CombinePaths(CommandUtils.CmdEnv.LogFolder, "BranchJobDef.properties");
			CommandUtils.WriteAllLines(BranchJobDefFile, ECProps.ToArray());
			RunECTool(String.Format("setProperty \"/myJob/BranchJobDefFile\" \"{0}\"", BranchJobDefFile.Replace("\\", "\\\\")));
		}
		if (bFakeEC)
		{
			foreach (string Args in FakeECArgs)
			{
				RunUAT(CmdEnv, Args);
			}
		}
	}

	void ExecuteNodes(List<NodeInfo> OrdereredToDo, bool bFake, bool bFakeEC, bool bSaveSharedTempStorage, Dictionary<string, NodeHistory> GUBPNodesHistory, string CLString, string StoreName, string FakeFail)
	{
        Dictionary<string, NodeInfo> BuildProductToNodeMap = new Dictionary<string, NodeInfo>();
		foreach (NodeInfo NodeToDo in OrdereredToDo)
        {
            if (NodeToDo.Node.BuildProducts != null || NodeToDo.Node.AllDependencyBuildProducts != null)
            {
                throw new AutomationException("topological sort error");
            }

            NodeToDo.Node.AllDependencyBuildProducts = new List<string>();
            NodeToDo.Node.AllDependencies = new List<string>();
            foreach (NodeInfo Dep in NodeToDo.Dependencies)
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

            string NodeStoreName = StoreName + "-" + NodeToDo.Node.GetFullName();
            
            string GameNameIfAny = NodeToDo.Node.GameNameIfAnyForTempStorage();
            string StorageRootIfAny = NodeToDo.Node.RootIfAnyForTempStorage();
					
            if (bFake)
            {
                StorageRootIfAny = ""; // we don't rebase fake runs since those are entirely "records of success", which are always in the logs folder
            }

            // this is kinda complicated
            bool SaveSuccessRecords = (IsBuildMachine || bFakeEC) && // no real reason to make these locally except for fakeEC tests
                (!NodeToDo.Node.TriggerNode() || NodeToDo.Node.IsSticky()) // trigger nodes are run twice, one to start the new workflow and once when it is actually triggered, we will save reconds for the latter
                && (NodeToDo.Node.RunInEC() || !NodeToDo.Node.IsAggregate()); //aggregates not in EC can be "run" multiple times, so we can't track those

            Log("***** Running GUBP Node {0} -> {1} : {2}", NodeToDo.Node.GetFullName(), GameNameIfAny, NodeStoreName);
            if (NodeToDo.IsComplete)
            {
                if (NodeToDo.Name == VersionFilesNode.StaticGetFullName() && !IsBuildMachine)
                {
                    Log("***** NOT ****** Retrieving GUBP Node {0} from {1}; it is the version files.", NodeToDo.Node.GetFullName(), NodeStoreName);
                    NodeToDo.Node.BuildProducts = new List<string>();

                }
                else
                {
                    Log("***** Retrieving GUBP Node {0} from {1}", NodeToDo.Node.GetFullName(), NodeStoreName);
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
                    SaveStatus(NodeToDo, StartedTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
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
                    if (!NodeToDo.Node.IsAggregate())
                    {
						double StoreDuration = 0.0;
						DateTime StartTime = DateTime.UtcNow;
						using(TelemetryStopwatch StoreBuildProductsStopwatch = new TelemetryStopwatch("StoreBuildProducts"))
						{
							TempStorage.StoreToTempStorage(CmdEnv, NodeStoreName, NodeToDo.Node.BuildProducts, !bSaveSharedTempStorage, GameNameIfAny, StorageRootIfAny);
						}
						StoreDuration = (DateTime.UtcNow - StartTime).TotalMilliseconds / 1000;
						Log("Took {0} seconds to store build products", StoreDuration);
						if (IsBuildMachine)
						{
							RunECTool(String.Format("setProperty \"/myJobStep/StoreDuration\" \"{0}\"", StoreDuration.ToString()));
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
                }
                catch (Exception Ex)
                {
                    if (SaveSuccessRecords)
                    {
						using(TelemetryStopwatch UpdateNodeHistoryStopwatch = new TelemetryStopwatch("UpdateNodeHistory"))
						{
							UpdateNodeHistory(GUBPNodesHistory, NodeToDo, CLString, StoreName);
						}
						using(TelemetryStopwatch SaveNodeStatusStopwatch = new TelemetryStopwatch("SaveNodeStatus"))
						{
							SaveStatus(NodeToDo, FailedTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny, ParseParamValue("MyJobStepId"));
						}
						using(TelemetryStopwatch UpdateECPropsStopwatch = new TelemetryStopwatch("UpdateECProps"))
						{
							UpdateECProps(NodeToDo, CLString);
						}
                        
						if (IsBuildMachine)
						{
							using(TelemetryStopwatch GetFailEmailsStopwatch = new TelemetryStopwatch("GetFailEmails"))
							{
								GetFailureEmails(GUBPNodesHistory, NodeToDo, CLString, StoreName);
							}
						}
						UpdateECBuildTime(NodeToDo, BuildDuration);
                    }

                    Log("{0}", ExceptionToString(Ex));


                    if (GUBPNodesHistory.ContainsKey(NodeToDo.Name))
                    {
                        NodeHistory History = GUBPNodesHistory[NodeToDo.Name];
                        Log("Changes since last green *********************************");
                        Log("");
                        Log("");
                        Log("");
                        PrintDetailedChanges(History);
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
					using(TelemetryStopwatch UpdateNodeHistoryStopwatch = new TelemetryStopwatch("UpdateNodeHistory"))
					{
						UpdateNodeHistory(GUBPNodesHistory, NodeToDo, CLString, StoreName);
					}
					using(TelemetryStopwatch SaveNodeStatusStopwatch = new TelemetryStopwatch("SaveNodeStatus"))
					{
						SaveStatus(NodeToDo, SucceededTempStorageSuffix, NodeStoreName, bSaveSharedTempStorage, GameNameIfAny);
					}
					using(TelemetryStopwatch UpdateECPropsStopwatch = new TelemetryStopwatch("UpdateECProps"))
					{
						UpdateECProps(NodeToDo, CLString);
					}
                    
					if (IsBuildMachine)
					{
						using(TelemetryStopwatch GetFailEmailsStopwatch = new TelemetryStopwatch("GetFailEmails"))
						{
							GetFailureEmails(GUBPNodesHistory, NodeToDo, CLString, StoreName);
						}
					}
					UpdateECBuildTime(NodeToDo, BuildDuration);
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
    
	/// <summary>
	/// Sorts a list of nodes to display in EC. The default order is based on execution order and agent groups, whereas this function arranges nodes by
	/// frequency then execution order, while trying to group nodes on parallel paths (eg. Mac/Windows editor nodes) together.
	/// </summary>
	static Dictionary<string, int> GetDisplayOrder(List<string> NodeNames, Dictionary<string, string> InitialNodeDependencyNames, Dictionary<string, NodeInfo> GUBPNodes)
	{
		// Split the nodes into separate lists for each frequency
		SortedDictionary<int, List<string>> NodesByFrequency = new SortedDictionary<int,List<string>>();
		foreach(string NodeName in NodeNames)
		{
			List<string> NodesByThisFrequency;
			if(!NodesByFrequency.TryGetValue(GUBPNodes[NodeName].FrequencyShift, out NodesByThisFrequency))
			{
				NodesByThisFrequency = new List<string>();
				NodesByFrequency.Add(GUBPNodes[NodeName].FrequencyShift, NodesByThisFrequency);
			}
			NodesByThisFrequency.Add(NodeName);
		}

		// Build the output list by scanning each frequency in order
		HashSet<string> VisitedNodes = new HashSet<string>();
		Dictionary<string, int> SortedNodes = new Dictionary<string,int>();
		foreach(List<string> NodesByThisFrequency in NodesByFrequency.Values)
		{
			// Find a list of nodes in each display group. If the group name matches the node name, put that node at the front of the list.
			Dictionary<string, string> DisplayGroups = new Dictionary<string,string>();
			foreach(string NodeName in NodesByThisFrequency)
			{
				string GroupName = GUBPNodes[NodeName].Node.GetDisplayGroupName();
				if(!DisplayGroups.ContainsKey(GroupName))
				{
					DisplayGroups.Add(GroupName, NodeName);
				}
				else if(GroupName == NodeName)
				{
					DisplayGroups[GroupName] = NodeName + " " + DisplayGroups[GroupName];
				}
				else
				{
					DisplayGroups[GroupName] = DisplayGroups[GroupName] + " " + NodeName;
				}
			}

			// Build a list of ordering dependencies, putting all Mac nodes after Windows nodes with the same names.
			Dictionary<string, string> NodeDependencyNames = new Dictionary<string,string>(InitialNodeDependencyNames);
			foreach(KeyValuePair<string, string> DisplayGroup in DisplayGroups)
			{
				string[] GroupNodes = DisplayGroup.Value.Split(' ');
				for(int Idx = 1; Idx < GroupNodes.Length; Idx++)
				{
					NodeDependencyNames[GroupNodes[Idx]] += " " + GroupNodes[0];
				}
			}

			// Add nodes for each frequency into the master list, trying to match up different groups along the way
			foreach(string FirstNodeName in NodesByThisFrequency)
			{
				string[] GroupNodeNames = DisplayGroups[GUBPNodes[FirstNodeName].Node.GetDisplayGroupName()].Split(' ');
				foreach(string GroupNodeName in GroupNodeNames)
				{
					AddNodeAndDependencies(GroupNodeName, NodeDependencyNames, VisitedNodes, SortedNodes);
				}
			}
		}
		return SortedNodes;
	}

	static void AddNodeAndDependencies(string NodeName, Dictionary<string, string> NodeDependencyNames, HashSet<string> VisitedNodes, Dictionary<string, int> SortedNodes)
	{
		if(!VisitedNodes.Contains(NodeName))
		{
			VisitedNodes.Add(NodeName);
			foreach(string NodeDependencyName in NodeDependencyNames[NodeName].Split(new char[]{ ' ' }, StringSplitOptions.RemoveEmptyEntries))
			{
				AddNodeAndDependencies(NodeDependencyName, NodeDependencyNames, VisitedNodes, SortedNodes);
			}
			SortedNodes.Add(NodeName, SortedNodes.Count);
		}
	}
}
