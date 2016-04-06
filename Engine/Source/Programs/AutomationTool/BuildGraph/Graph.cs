// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

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

namespace AutomationTool
{
	/// <summary>
	/// Options for how the graph should be printed
	/// </summary>
	enum GraphPrintOptions
	{
		/// <summary>
		/// Includes the list of dependencies for each node
		/// </summary>
		ShowDependencies = 0x1,

		/// <summary>
		/// Includes the list of notifiers for each node
		/// </summary>
		ShowNotifications = 0x2,
	}

	/// <summary>
	/// Diagnostic message from the graph script. These messages are parsed at startup, then culled along with the rest of the graph nodes before output. Doing so
	/// allows errors and warnings which are only output if a node is part of the graph being executed.
	/// </summary>
	class GraphDiagnostic
	{
		/// <summary>
		/// The diagnostic event type
		/// </summary>
		public LogEventType EventType;

		/// <summary>
		/// The message to display
		/// </summary>
		public string Message;

		/// <summary>
		/// The node which this diagnostic is declared in. If the node is culled from the graph, the message will not be displayed.
		/// </summary>
		public Node EnclosingNode;

		/// <summary>
		/// The agent group that this diagnostic is declared in. If the entire group is culled from the graph, the message will not be displayed.
		/// </summary>
		public AgentGroup EnclosingGroup;

		/// <summary>
		/// The trigger that this diagnostic is declared in. If this trigger is not being run, the message will not be displayed.
		/// </summary>
		public ManualTrigger EnclosingTrigger;
	}

	/// <summary>
	/// Definition of a graph.
	/// </summary>
	class Graph
	{
		/// <summary>
		/// List of nodes grouped by the agent they will be executed on
		/// </summary>
		public List<AgentGroup> Groups = new List<AgentGroup>();

		/// <summary>
		/// All manual triggers that are part of this graph
		/// </summary>
		public Dictionary<string, ManualTrigger> NameToTrigger = new Dictionary<string, ManualTrigger>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// Mapping of names to the corresponding node.
		/// </summary>
		public Dictionary<string, Node> NameToNode = new Dictionary<string,Node>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// Mapping of names to their corresponding node output.
		/// </summary>
		public Dictionary<string, NodeOutput> NameToNodeOutput = new Dictionary<string,NodeOutput>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// Mapping of aggregate names to their respective nodes
		/// </summary>
		public Dictionary<string, Node[]> AggregateNameToNodes = new Dictionary<string,Node[]>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// Diagnostic messages for this graph
		/// </summary>
		public List<GraphDiagnostic> Diagnostics = new List<GraphDiagnostic>();

		/// <summary>
		/// Default constructor
		/// </summary>
		public Graph()
		{
		}

		/// <summary>
		/// Checks whether a given name (or tag name) already exists
		/// </summary>
		/// <param name="Name">The name to check. May begin with a '#' character.</param>
		/// <returns>True if the name exists, false otherwise.</returns>
		public bool ContainsName(string Name)
		{
			// Strip off the tag character, if present
			string TrimName = Name.StartsWith("#")? Name.Substring(1) : Name;

			// Check it doesn't match anything in the graph
			return NameToNode.ContainsKey(TrimName) || NameToNodeOutput.ContainsKey(TrimName) || AggregateNameToNodes.ContainsKey(TrimName);
		}

		/// <summary>
		/// Tries to resolve the given name to one or more nodes. Checks for aggregates, and actual nodes.
		/// </summary>
		/// <param name="Name">The name to search for</param>
		/// <param name="OutNodes">If the name is a match, receives an array of nodes and their output names</param>
		/// <returns>True if the name was found, false otherwise.</returns>
		public bool TryResolveReference(string Name, out Node[] OutNodes)
		{
			// Check if it's a tag reference or node reference
			if(Name.StartsWith("#"))
			{
				// Check if it's a regular node or output name
				NodeOutput Output;
				if(NameToNodeOutput.TryGetValue(Name.Substring(1), out Output))
				{
					OutNodes = new Node[]{ Output.ProducingNode };
					return true;
				}
			}
			else
			{
				// Check if it's a regular node or output name
				Node Node;
				if(NameToNode.TryGetValue(Name, out Node))
				{
					OutNodes = new Node[]{ Node };
					return true;
				}

				// Check if it's an aggregate name
				Node[] Nodes;
				if(AggregateNameToNodes.TryGetValue(Name, out Nodes))
				{
					OutNodes = Nodes;
					return true;
				}
			}

			// Otherwise fail
			OutNodes = null;
			return false;
		}

		/// <summary>
		/// Tries to resolve the given name to one or more node outputs. Checks for aggregates, and actual nodes.
		/// </summary>
		/// <param name="Name">The name to search for</param>
		/// <param name="OutOutputs">If the name is a match, receives an array of nodes and their output names</param>
		/// <returns>True if the name was found, false otherwise.</returns>
		public bool TryResolveInputReference(string Name, out NodeOutput[] OutOutputs)
		{
			// Check if it's a tag reference or node reference
			if(Name.StartsWith("#"))
			{
				// Check if it's a regular node or output name
				NodeOutput Output;
				if(NameToNodeOutput.TryGetValue(Name.Substring(1), out Output))
				{
					OutOutputs = new NodeOutput[]{ Output };
					return true;
				}
			}
			else
			{
				// Check if it's a regular node or output name
				Node Node;
				if(NameToNode.TryGetValue(Name, out Node))
				{
					OutOutputs = Node.Outputs.Union(Node.Inputs).ToArray();
					return true;
				}

				// Check if it's an aggregate name
				Node[] Nodes;
				if(AggregateNameToNodes.TryGetValue(Name, out Nodes))
				{
					OutOutputs = Nodes.SelectMany(x => x.Outputs.Union(x.Inputs)).Distinct().ToArray();
					return true;
				}
			}

			// Otherwise fail
			OutOutputs = null;
			return false;
		}

		/// <summary>
		/// Cull the graph to only include the given nodes and their dependencies
		/// </summary>
		/// <param name="TargetNodes">A set of target nodes to build</param>
		public void Select(IEnumerable<Node> TargetNodes)
		{
			// Find this node and all its dependencies
			HashSet<Node> RetainNodes = new HashSet<Node>(TargetNodes);
			foreach(Node TargetNode in TargetNodes)
			{
				RetainNodes.UnionWith(TargetNode.InputDependencies);
			}

			// Remove all the nodes which are not marked to be kept
			foreach(AgentGroup Group in Groups)
			{
				Group.Nodes = Group.Nodes.Where(x => RetainNodes.Contains(x)).ToList();
			}

			// Remove all the empty groups
			Groups.RemoveAll(x => x.Nodes.Count == 0);

			// Remove all the order dependencies which are no longer part of the graph. Since we don't need to build them, we don't need to wait for them
			foreach(Node Node in RetainNodes)
			{
				Node.OrderDependencies = Node.OrderDependencies.Where(x => RetainNodes.Contains(x)).ToArray();
			}

			// Create a new list of triggers from all the nodes which are left
			NameToTrigger = RetainNodes.Where(x => x.ControllingTrigger != null).Select(x => x.ControllingTrigger).Distinct().ToDictionary(x => x.Name, x => x, StringComparer.InvariantCultureIgnoreCase);

			// Create a new list of aggregates for everything that's left
			AggregateNameToNodes = AggregateNameToNodes.Where(x => x.Value.All(y => RetainNodes.Contains(y))).ToDictionary(Pair => Pair.Key, Pair => Pair.Value);

			// Remove any diagnostics which are no longer part of the graph
			Diagnostics.RemoveAll(x => (x.EnclosingNode != null && !RetainNodes.Contains(x.EnclosingNode)) || (x.EnclosingGroup != null && !Groups.Contains(x.EnclosingGroup)));
		}

		/// <summary>
		/// Export the build graph to a Json file, for parallel execution by the build system
		/// </summary>
		/// <param name="File">Output file to write</param>
		/// <param name="ActivatedTriggers">Set of triggers which have been activated</param>
		/// <param name="CompletedNodes">Set of nodes which have been completed</param>
		public void Export(FileReference File, HashSet<ManualTrigger> ActivatedTriggers, HashSet<Node> CompletedNodes)
		{
			// Find all the nodes which we're actually going to execute. We'll use this to filter the graph.
			HashSet<Node> NodesToExecute = new HashSet<Node>();
			foreach(Node Node in Groups.SelectMany(x => x.Nodes))
			{
				if(!CompletedNodes.Contains(Node))
				{
					if(Node.ControllingTrigger == null || ActivatedTriggers.Contains(Node.ControllingTrigger))
					{
						NodesToExecute.Add(Node);
					}
				}
			}

			// Open the output file
			using(JsonWriter JsonWriter = new JsonWriter(File.FullName))
			{
				JsonWriter.WriteObjectStart();

				// Write all the agent groups
				JsonWriter.WriteArrayStart("Groups");
				foreach(AgentGroup Group in Groups)
				{
					Node[] Nodes = Group.Nodes.Where(x => NodesToExecute.Contains(x)).ToArray();
					if(Nodes.Length > 0)
					{
						JsonWriter.WriteObjectStart();
						JsonWriter.WriteValue("Name", Group.Name);
						JsonWriter.WriteArrayStart("Agent Types");
						foreach(string AgentType in Group.PossibleTypes)
						{
							JsonWriter.WriteValue(AgentType);
						}
						JsonWriter.WriteArrayEnd();
						JsonWriter.WriteArrayStart("Nodes");
						foreach(Node Node in Nodes)
						{
							JsonWriter.WriteObjectStart();
							JsonWriter.WriteValue("Name", Node.Name);
							JsonWriter.WriteValue("DependsOn", String.Join(";", Node.GetDirectOrderDependencies().Where(x => NodesToExecute.Contains(x))));
							JsonWriter.WriteObjectStart("Notify");
							JsonWriter.WriteValue("Default", String.Join(";", Node.NotifyUsers));
							JsonWriter.WriteValue("Submitters", String.Join(";", Node.NotifySubmitters));
							JsonWriter.WriteValue("Warnings", Node.bNotifyOnWarnings);
							JsonWriter.WriteObjectEnd();
							JsonWriter.WriteObjectEnd();
						}
						JsonWriter.WriteArrayEnd();
						JsonWriter.WriteObjectEnd();
					}
				}
				JsonWriter.WriteArrayEnd();

				// Write all the triggers
				JsonWriter.WriteArrayStart("Triggers");
				foreach (ManualTrigger Trigger in NameToTrigger.Values)
				{
					if(!ActivatedTriggers.Contains(Trigger) && NodesToExecute.Any(x => x.ControllingTrigger == Trigger.Parent))
					{
						// Find all the nodes that this trigger is dependent on
						HashSet<Node> Dependencies = new HashSet<Node>();
						foreach(Node Node in Groups.SelectMany(x => x.Nodes))
						{
							for(ManualTrigger ControllingTrigger = Node.ControllingTrigger; ControllingTrigger != null; ControllingTrigger = ControllingTrigger.Parent)
							{
								if(ControllingTrigger == Trigger)
								{
									Dependencies.UnionWith(Node.OrderDependencies.Where(x => x.ControllingTrigger != Trigger && NodesToExecute.Contains(x)));
									break;
								}
							}
						}

						// Reduce that list to the smallest subset of direct dependencies
						HashSet<Node> DirectDependencies = new HashSet<Node>(Dependencies);
						foreach(Node Dependency in Dependencies)
						{
							DirectDependencies.ExceptWith(Dependency.OrderDependencies);
						}

						// Write out the object
						JsonWriter.WriteObjectStart();
						JsonWriter.WriteValue("Name", Trigger.Name);
						JsonWriter.WriteValue("AllDependencies", String.Join(";", Groups.SelectMany(x => x.Nodes).Where(x => Dependencies.Contains(x)).Select(x => x.Name)));
						JsonWriter.WriteValue("DirectDependencies", String.Join(";", Dependencies.Where(x => DirectDependencies.Contains(x)).Select(x => x.Name)));
						JsonWriter.WriteValue("Notify", String.Join(";", Trigger.NotifyUsers));
						JsonWriter.WriteObjectEnd();
					}
				}
				JsonWriter.WriteArrayEnd();
				JsonWriter.WriteObjectEnd();
			}
		}

		/// <summary>
		/// Print the contents of the graph
		/// </summary>
		/// <param name="NodeToState">Mapping of node to its current state</param>
		/// <param name="Options">Options for how to print the graph</param>
		public void Print(HashSet<Node> CompletedNodes, GraphPrintOptions Options)
		{
			// Get a list of all the triggers, including the null global one
			List<ManualTrigger> AllTriggers = new List<ManualTrigger>();
			AllTriggers.Add(null);
			AllTriggers.AddRange(NameToTrigger.Values.OrderBy(x => x.QualifiedName));

			// Output all the triggers in order
			CommandUtils.Log("");
			CommandUtils.Log("Graph:");
			foreach(ManualTrigger Trigger in AllTriggers)
			{
				// Filter everything by this trigger
				Dictionary<AgentGroup, Node[]> FilteredGroupToNodes = new Dictionary<AgentGroup,Node[]>();
				foreach(AgentGroup Group in Groups)
				{
					Node[] Nodes = Group.Nodes.Where(x => x.ControllingTrigger == Trigger).ToArray();
					if(Nodes.Length > 0)
					{
						FilteredGroupToNodes[Group] = Nodes;
					}
				}

				// Skip this trigger if there's nothing to display
				if(FilteredGroupToNodes.Count == 0)
				{
					continue;
				}

				// Print the trigger name
				CommandUtils.Log("    Trigger: {0}", (Trigger == null)? "None" : Trigger.QualifiedName);
				if(Trigger != null && Options.HasFlag(GraphPrintOptions.ShowNotifications))
				{
					foreach(string User in Trigger.NotifyUsers)
					{
						CommandUtils.Log("            notify> {0}", User);
					}
				}

				// Output all the groups for this trigger
				foreach(AgentGroup Group in Groups)
				{
					Node[] Nodes;
					if(FilteredGroupToNodes.TryGetValue(Group, out Nodes))
					{
						CommandUtils.Log("        Group: {0} ({1})", Group.Name, String.Join(";", Group.PossibleTypes));
						foreach(Node Node in Nodes)
						{
							CommandUtils.Log("            Node: {0}{1}", Node.Name, CompletedNodes.Contains(Node)? " (completed)" : "");
							if(Options.HasFlag(GraphPrintOptions.ShowDependencies))
							{
								HashSet<Node> InputDependencies = new HashSet<Node>(Node.GetDirectInputDependencies());
								foreach(Node InputDependency in InputDependencies)
								{
									CommandUtils.Log("                input> {0}", InputDependency.Name);
								}
								HashSet<Node> OrderDependencies = new HashSet<Node>(Node.GetDirectOrderDependencies());
								foreach(Node OrderDependency in OrderDependencies.Except(InputDependencies))
								{
									CommandUtils.Log("                after> {0}", OrderDependency.Name);
								}
							}
							if(Options.HasFlag(GraphPrintOptions.ShowNotifications))
							{
								string Label = Node.bNotifyOnWarnings? "warnings" : "errors";
								foreach(string User in Node.NotifyUsers)
								{
									CommandUtils.Log("                {0}> {1}", Label, User);
								}
								foreach(string Submitter in Node.NotifySubmitters)
								{
									CommandUtils.Log("                {0}> submitters to {1}", Label, Submitter);
								}
							}
						}
					}
				}
			}
			CommandUtils.Log("");

			// Print out all the aggregates
			string[] AggregateNames = AggregateNameToNodes.Keys.OrderBy(x => x).ToArray();
			if(AggregateNames.Length > 0)
			{
				CommandUtils.Log("Aggregates:");
				foreach(string AggregateName in AggregateNames)
				{
					CommandUtils.Log("    {0}", AggregateName);
				}
				CommandUtils.Log("");
			}
		}
	}
}
