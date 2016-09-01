using EpicGames.MCP.Automation;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using UnrealBuildTool;

namespace AutomationTool.Tasks
{
	/// <summary>
	/// Parameters for a task which splits a build into chunks
	/// </summary>
	public class ChunkTaskParameters
	{
		/// <summary>
		/// The application name
		/// </summary>
		[TaskParameter]
		public string AppName;

		/// <summary>
		/// The application id
		/// </summary>
		[TaskParameter(Optional = true)]
		public int AppID = 1;

		/// <summary>
		/// Platform we are staging for.
		/// </summary>
		[TaskParameter]
		public MCPPlatform Platform;

		/// <summary>
		/// BuildVersion of the App we are staging.
		/// </summary>
		[TaskParameter]
		public string BuildVersion;

		/// <summary>
		/// Directory that build data will be copied from.
		/// </summary>
		[TaskParameter]
		public string InputDir;

		/// <summary>
		/// Optional list of files that should be considered
		/// </summary>
		[TaskParameter(Optional = true)]
		public string Files;

		/// <summary>
		/// The executable to run to launch this application.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string Launch;

		/// <summary>
		/// Parameters that the application should be launched with.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string LaunchArgs;

		/// <summary>
		/// Full path to the CloudDir where chunks and manifests should be staged.
		/// </summary>
		[TaskParameter]
		public string CloudDir;

		/// <summary>
		/// Determines the version of BuildPatchTool to use, for example, to allow us to use a pre-release version.
		/// </summary>
		[TaskParameter(Optional = true)]
		public BuildPatchToolBase.ToolVersion ToolVersion = BuildPatchToolBase.ToolVersion.Live;

		/// <summary>
		/// Location of a file listing attributes to apply to chunked files.
		/// Should contain quoted InputDir relative files followed by optional attribute keywords readonly compressed executable, separated by \\r\\n line endings.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string AttributesFileName;

		/// <summary>
		/// The prerequisites installer to launch on successful product install, must be relative to, and inside of InputDir.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string PrereqPath;

		/// <summary>
		/// The commandline to send to prerequisites installer on launch.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string PrereqArgs;
	}

	/// <summary>
	/// Implements a task which splits a build into chunks
	/// </summary>
	[TaskElement("Chunk", typeof(ChunkTaskParameters))]
	public class ChunkTask : CustomTask
	{
		/// <summary>
		/// Parameters for this task
		/// </summary>
		ChunkTaskParameters Parameters;

		/// <summary>
		/// Construct a new ChunkTask.
		/// </summary>
		/// <param name="InParameters">Parameters for this task</param>
		public ChunkTask(ChunkTaskParameters InParameters)
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
			// Get the build directory
			DirectoryReference InputDir = ResolveDirectory(Parameters.InputDir);

			// If there's a set of files specified, generate a temporary ignore list.
			FileReference IgnoreList = null;
			if (Parameters.Files != null)
			{
				// Find the files which are to be included
				HashSet<FileReference> IncludeFiles = ResolveFilespec(InputDir, Parameters.Files, TagNameToFileSet);

				// Create a file to store the ignored file list
				IgnoreList = new FileReference(LogUtils.GetUniqueLogName(Path.Combine(CommandUtils.CmdEnv.LogFolder, Parameters.AppName + "-Ignore")));
				using (StreamWriter Writer = new StreamWriter(IgnoreList.FullName))
				{
					DirectoryInfo InputDirInfo = new DirectoryInfo(InputDir.FullName);
					foreach (FileInfo File in InputDirInfo.EnumerateFiles("*", SearchOption.AllDirectories))
					{
						FileReference FileRef = new FileReference(File);
						if (!IncludeFiles.Contains(FileRef))
						{
							string RelativePath = FileRef.MakeRelativeTo(InputDir);
							const string Iso8601DateTimeFormat = "yyyy'-'MM'-'dd'T'HH':'mm':'ss'.'fffZ";
							Writer.WriteLine("{0}\t{1}", (RelativePath.IndexOf(' ') == -1) ? RelativePath : "\"" + RelativePath + "\"", File.LastWriteTimeUtc.ToString(Iso8601DateTimeFormat));
						}
					}
				}
			}

			// Create the staging info
			BuildPatchToolStagingInfo StagingInfo = new BuildPatchToolStagingInfo(Job.OwnerCommand, Parameters.AppName, Parameters.AppID, Parameters.BuildVersion, Parameters.Platform, Parameters.CloudDir);

			// Set the patch generation options
			BuildPatchToolBase.PatchGenerationOptions Options = new BuildPatchToolBase.PatchGenerationOptions();
			Options.StagingInfo = StagingInfo;
			Options.BuildRoot = ResolveDirectory(Parameters.InputDir).FullName;
			Options.FileIgnoreList = (IgnoreList != null) ? IgnoreList.FullName : null;
			Options.FileAttributeList = Parameters.AttributesFileName ?? "";
			Options.AppLaunchCmd = Parameters.Launch ?? "";
			Options.AppLaunchCmdArgs = Parameters.LaunchArgs ?? "";
			Options.AppChunkType = BuildPatchToolBase.ChunkType.Chunk;
			Options.Platform = Parameters.Platform;
			Options.PrereqPath = Parameters.PrereqPath ?? "";
			Options.PrereqArgs = Parameters.PrereqArgs ?? "";

			// Run the chunking
			BuildPatchToolBase.Get().Execute(Options, Parameters.ToolVersion);
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
			return FindTagNamesFromFilespec(Parameters.Files);
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
