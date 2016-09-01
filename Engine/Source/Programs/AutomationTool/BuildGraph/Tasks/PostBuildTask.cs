using AutomationTool;
using EpicGames.MCP.Automation;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using UnrealBuildTool;

namespace BuildGraph.Tasks
{
	/// <summary>
	/// Parameters to posts build information to the MCP backend
	/// </summary>
	public class PostBuildTaskParameters
	{
		/// <summary>
		/// The application name
		/// </summary>
		[TaskParameter]
		public string AppName;

        /// <summary>
        /// Unique build version of the app.
        /// </summary>
		[TaskParameter]
        public string BuildVersion;

        /// <summary>
        /// Platform we are posting info for.
        /// </summary>
		[TaskParameter]
        public MCPPlatform Platform;

        /// <summary>
        /// Path to the directory containing chunks and manifests
        /// </summary>
		[TaskParameter]
        public string CloudDir;

		/// <summary>
		/// The MCP configuration name.
		/// </summary>
		[TaskParameter]
		public string McpConfig;
	}

	/// <summary>
	/// Implements a task which posts build information to the MCP backend
	/// </summary>
	[TaskElement("PostBuild", typeof(PostBuildTaskParameters))]
	public class PostBuildTask : CustomTask
	{
		/// <summary>
		/// Parameters for this task
		/// </summary>
		PostBuildTaskParameters Parameters;

		/// <summary>
		/// Construct a new PostBuildTask.
		/// </summary>
		/// <param name="InParameters">Parameters for this task</param>
		public PostBuildTask(PostBuildTaskParameters InParameters)
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
			DirectoryReference CloudDir = new DirectoryReference(Parameters.CloudDir);
			BuildPatchToolStagingInfo StagingInfo = new BuildPatchToolStagingInfo(Job.OwnerCommand, Parameters.AppName, 1, Parameters.BuildVersion, Parameters.Platform, null, CloudDir);
			BuildInfoPublisherBase.Get().PostBuildInfo(StagingInfo, Parameters.McpConfig);
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
