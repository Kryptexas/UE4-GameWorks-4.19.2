using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnrealBuildTool;
using AutomationTool;
using System.Xml;

namespace AutomationTool
{
	/// <summary>
	/// Reference to a node's output
	/// </summary>
	class NodeOutput
	{
		/// <summary>
		/// The node which produces the given output
		/// </summary>
		public Node ProducingNode;

		/// <summary>
		/// Name of the output
		/// </summary>
		public string Name;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InProducingNode">Node which produces the given output</param>
		/// <param name="InName">Name of the output</param>
		public NodeOutput(Node InProducingNode, string InName)
		{
			ProducingNode = InProducingNode;
			Name = InName;
		}

		/// <summary>
		/// Returns a string representation of this output for debugging purposes
		/// </summary>
		/// <returns>The name of this output</returns>
		public override string ToString()
		{
			return (ProducingNode.Name == Name)? Name : String.Format("{0}: {1}", ProducingNode.Name, Name);
		}
	}

	/// <summary>
	/// Defines a node, a container for tasks and the smallest unit of execution that can be run as part of a build graph.
	/// </summary>
	class Node
	{
		/// <summary>
		/// The node's name
		/// </summary>
		public string Name;

		/// <summary>
		/// Array of inputs which this node requires to run
		/// </summary>
		public NodeOutput[] Inputs;

		/// <summary>
		/// Array of output names produced by this node
		/// </summary>
		public NodeOutput[] Outputs;

		/// <summary>
		/// Nodes which this node has input dependencies on
		/// </summary>
		public Node[] InputDependencies;

		/// <summary>
		/// Nodes which this node needs to run after
		/// </summary>
		public Node[] OrderDependencies;

		/// <summary>
		/// The trigger which controls whether this node will be executed
		/// </summary>
		public ManualTrigger ControllingTrigger;

		/// <summary>
		/// List of tasks to execute
		/// </summary>
		public List<CustomTask> Tasks = new List<CustomTask>();

		/// <summary>
		/// List of email addresses to notify if this node fails.
		/// </summary>
		public HashSet<string> NotifyUsers = new HashSet<string>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// If set, anyone that has submitted to one of the given paths will be notified on failure of this node
		/// </summary>
		public HashSet<string> NotifySubmitters = new HashSet<string>(StringComparer.InvariantCultureIgnoreCase);

		/// <summary>
		/// Whether to ignore warnings produced by this node
		/// </summary>
		public bool bNotifyOnWarnings = true;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InName">The name of this node</param>
		/// <param name="InInputs">Inputs that this node depends on</param>
		/// <param name="InOutputNames">Names of the outputs that this node produces</param>
		/// <param name="InInputDependencies">Nodes which this node is dependent on for its inputs</param>
		/// <param name="InOrderDependencies">Nodes which this node needs to run after. Should include all input dependencies.</param>
		/// <param name="InControllingTrigger">The trigger which this node is behind</param>
		public Node(string InName, NodeOutput[] InInputs, string[] InOutputNames, Node[] InInputDependencies, Node[] InOrderDependencies, ManualTrigger InControllingTrigger)
		{
			Name = InName;
			Inputs = InInputs;
			Outputs = InOutputNames.Select(x => new NodeOutput(this, x)).ToArray();
			InputDependencies = InInputDependencies;
			OrderDependencies = InOrderDependencies;
			ControllingTrigger = InControllingTrigger;
		}

		/// <summary>
		/// Build all the tasks for this node
		/// </summary>
		/// <param name="Job">Information about the current job</param>
		/// <param name="BuildProducts">Set of build products produced by this node.</param>
		/// <param name="TagNameToFileSet">Mapping from tag names to the set of files they include. Should be set to contain the node inputs on entry.</param>
		/// <returns>Whether the task succeeded or not. Exiting with an exception will be caught and treated as a failure.</returns>
		public bool Build(JobContext Job, Dictionary<string, HashSet<FileReference>> TagNameToFileSet)
		{
			bool bResult = true;

			// Allow tasks to merge together
			MergeTasks();

			// Build everything
			HashSet<FileReference> BuildProducts = new HashSet<FileReference>();
			foreach(CustomTask Task in Tasks)
			{
				if(!Task.Execute(Job, BuildProducts, TagNameToFileSet))
				{
					CommandUtils.LogError("Failed to execute task.");
					return false;
				}
			}

			// Build a mapping of build product to the outputs it belongs to, using the filesets created by the tasks.
			Dictionary<FileReference, NodeOutput> FileToOutput = new Dictionary<FileReference,NodeOutput>();
			foreach(NodeOutput Output in Outputs)
			{
				HashSet<FileReference> FileSet = TagNameToFileSet[Output.Name];
				foreach(FileReference File in FileSet)
				{
					NodeOutput ExistingOutput;
					if(FileToOutput.TryGetValue(File, out ExistingOutput))
					{
						CommandUtils.LogError("Build product is added to multiple outputs; {0} added to {1} and {2}", File.MakeRelativeTo(new DirectoryReference(CommandUtils.CmdEnv.LocalRoot)), ExistingOutput.Name, Output.Name);
						bResult = false;
						continue;
					}
					FileToOutput.Add(File, Output);
				}
			}

			// Add any remaining valid build products into the output channel for this node. Since it's a catch-all output whose build products were not explicitly specified by the user, we can remove 
			// those which are outside the root directory or which no longer exist (they may have been removed by downstream tasks).
			HashSet<FileReference> DefaultOutputs = TagNameToFileSet[Name];
			foreach(FileReference BuildProduct in BuildProducts)
			{
				if(!FileToOutput.ContainsKey(BuildProduct) && BuildProduct.IsUnderDirectory(CommandUtils.RootDirectory) && BuildProduct.Exists())
				{
					DefaultOutputs.Add(BuildProduct);
				}
			}

			return bResult;
		}

		/// <summary>
		/// Merge tasks which can be combined together
		/// </summary>
		void MergeTasks()
		{
			List<CustomTask> MergedTasks = new List<CustomTask>();
			while(Tasks.Count > 0)
			{
				CustomTask NextTask = Tasks[0];
				Tasks.RemoveAt(0);
				NextTask.Merge(Tasks);
				MergedTasks.Add(NextTask);
			}
			Tasks = MergedTasks;
		}

		/// <summary>
		/// Determines the minimal set of direct input dependencies for this node to run
		/// </summary>
		/// <returns>Sequence of nodes that are direct inputs to this node</returns>
		public IEnumerable<Node> GetDirectInputDependencies()
		{
			HashSet<Node> DirectDependencies = new HashSet<Node>(InputDependencies);
			foreach(Node InputDependency in InputDependencies)
			{
				DirectDependencies.ExceptWith(InputDependency.InputDependencies);
			}
			return DirectDependencies;
		}

		/// <summary>
		/// Determines the minimal set of direct order dependencies for this node to run
		/// </summary>
		/// <returns>Sequence of nodes that are direct order dependencies of this node</returns>
		public IEnumerable<Node> GetDirectOrderDependencies()
		{
			HashSet<Node> DirectDependencies = new HashSet<Node>(OrderDependencies);
			foreach(Node OrderDependency in OrderDependencies)
			{
				DirectDependencies.ExceptWith(OrderDependency.OrderDependencies);
			}
			return DirectDependencies;
		}

		/// <summary>
		/// Checks whether this node is downstream of the given trigger
		/// </summary>
		/// <param name="Trigger">The trigger to check</param>
		/// <returns>True if the node is downstream of the trigger, false otherwise</returns>
		public bool IsControlledBy(ManualTrigger Trigger)
		{
			return Trigger == null || ControllingTrigger == Trigger || (ControllingTrigger != null && ControllingTrigger.IsDownstreamFrom(Trigger));
		}

		/// <summary>
		/// Write this node to an XML writer
		/// </summary>
		/// <param name="Writer">The writer to output the node to</param>
		public void Write(XmlWriter Writer)
		{
			Writer.WriteStartElement("Node");
			Writer.WriteAttributeString("Name", Name);

			Node[] RequireNodes = Inputs.Where(x => x.Name == x.ProducingNode.Name).Select(x => x.ProducingNode).ToArray();
			string[] RequireNames = RequireNodes.Select(x => x.Name).Union(Inputs.Where(x => !RequireNodes.Contains(x.ProducingNode)).Select(x => "#" + x.Name)).ToArray();
			if (RequireNames.Length > 0)
			{
				Writer.WriteAttributeString("Requires", String.Join(";", RequireNames));
			}

			string[] ProducesNames = Outputs.Where(x => x.Name != Name).Select(x => "#" + x.Name).ToArray();
			if (ProducesNames.Length > 0)
			{
				Writer.WriteAttributeString("Produces", String.Join(";", ProducesNames));
			}

			string[] AfterNames = GetDirectOrderDependencies().Except(InputDependencies).Select(x => x.Name).ToArray();
			if (AfterNames.Length > 0)
			{
				Writer.WriteAttributeString("After", String.Join(";", AfterNames));
			}

			if (!bNotifyOnWarnings)
			{
				Writer.WriteAttributeString("NotifyOnWarnings", bNotifyOnWarnings.ToString());
			}

			foreach (CustomTask Task in Tasks)
			{
				Task.Write(Writer);
			}
			Writer.WriteEndElement();
		}

		/// <summary>
		/// Returns the name of this node
		/// </summary>
		/// <returns>The name of this node</returns>
		public override string ToString()
		{
			return Name;
		}
	}
}
