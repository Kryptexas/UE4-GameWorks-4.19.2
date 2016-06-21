using EpicGames.MCP.Automation;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using UnrealBuildTool;

namespace AutomationTool.Tasks
{
	/// <summary>
	/// Parameters for a task which merges two chunked builds together
	/// </summary>
	public class MergeTaskParameters
	{
		/// <summary>
		/// The application name
		/// </summary>
		[TaskParameter]
		public string AppName;

		/// <summary>
		/// The initial build version
		/// </summary>
		[TaskParameter]
		public string BaseVersion;

		/// <summary>
		/// The additional files to merge in
		/// </summary>
		[TaskParameter]
		public string PatchVersion;

		/// <summary>
		/// The new build version
		/// </summary>
		[TaskParameter]
		public string FinalVersion;

		/// <summary>
		/// The platform to build for
		/// </summary>
		[TaskParameter]
		public MCPPlatform Platform;

        /// <summary>
        /// Full path to the CloudDir where chunks and manifests should be staged.
        /// </summary>
		[TaskParameter]
        public string CloudDir;
	}

	/// <summary>
	/// Implements a task which merges two chunked builds together
	/// </summary>
	[TaskElement("Merge", typeof(MergeTaskParameters))]
	public class MergeTask : CustomTask
	{
		/// <summary>
		/// Parameters for this task
		/// </summary>
		MergeTaskParameters Parameters;

		/// <summary>
		/// Construct a new CommandTask.
		/// </summary>
		/// <param name="InParameters">Parameters for this task</param>
		public MergeTask(MergeTaskParameters InParameters)
		{
			Parameters = InParameters;
		}

		/// <summary>
		/// Execute the task.
		/// </summary>
		/// <param name="Job">Information about the current job</param>
		/// <param name="BuildProducts">Set of build products produced by this node.</param>
		/// <param name="TagNameToFileSet">Mapping from tag names to the set of files they include</param>
		/// <returns>True if the task succeeded</returns>
		public override bool Execute(JobContext Job, HashSet<FileReference> BuildProducts, Dictionary<string, HashSet<FileReference>> TagNameToFileSet)
		{
			// Get the cloud directory
			DirectoryReference CloudDir = ResolveDirectory(Parameters.CloudDir);

			// Set the patch generation options
			BuildPatchToolBase.ManifestMergeOptions Options = new BuildPatchToolBase.ManifestMergeOptions();
			Options.ManifestA = FileReference.Combine(CloudDir, String.Format("{0}{1}-{2}.manifest", Parameters.AppName, Parameters.BaseVersion, Parameters.Platform)).FullName;
			Options.ManifestB = FileReference.Combine(CloudDir, String.Format("{0}{1}-{2}.manifest", Parameters.AppName, Parameters.PatchVersion, Parameters.Platform)).FullName;
			Options.ManifestC = FileReference.Combine(CloudDir, String.Format("{0}{1}-{2}.manifest", Parameters.AppName, Parameters.FinalVersion, Parameters.Platform)).FullName;
			Options.BuildVersion = String.Format("{0}-{1}", Parameters.FinalVersion, Parameters.Platform);

			// Run the chunking
			BuildPatchToolBase.Get().Execute(Options);
			return true;
		}

		/// <summary>
		/// Output this task out to an XML writer.
		/// </summary>
		public override void Write(XmlWriter Writer)
		{
			Write(Writer, Parameters);
		}

		/// <summary>
		/// Find all the tags which are used as inputs to this task
		/// </summary>
		/// <returns>The tag names which are read by this task</returns>
		public override IEnumerable<string> FindConsumedTagNames()
		{
			yield break;
		}

		/// <summary>
		/// Find all the tags which are modified by this task
		/// </summary>
		/// <returns>The tag names which are modified by this task</returns>
		public override IEnumerable<string> FindProducedTagNames()
		{
			yield break;
		}
	}
}
