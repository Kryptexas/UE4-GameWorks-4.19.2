using AutomationTool;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnrealBuildTool;

namespace BuildGraph.Tasks
{
	/// <summary>
	/// Parameters for the log task
	/// </summary>
	public class LogTaskParameters
	{
		/// <summary>
		/// Message to print out
		/// </summary>
		[TaskParameter]
		public string Message;

		/// <summary>
		/// If specified, causes the matching list of files to be printed after the given message.
		/// </summary>
		[TaskParameter(Optional = true, ValidationType = TaskParameterValidationType.Tag)]
		public string Tag;

		/// <summary>
		/// If specified, causes the contents of the given file to be printed after the given message.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string File;
	}

	/// <summary>
	/// Print a message (and other optional diagnostic information) to the output log
	/// </summary>
	[TaskElement("Log", typeof(LogTaskParameters))]
	public class LogTask : CustomTask
	{
		/// <summary>
		/// Parameters for the task
		/// </summary>
		LogTaskParameters Parameters;

		/// <summary>
		/// Constructor.
		/// </summary>
		/// <param name="InParameters">Parameters for the task</param>
		public LogTask(LogTaskParameters InParameters)
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
			// Print the message
			CommandUtils.Log(Parameters.Message);

			// Print the contents of the given tag, if specified
			if(!String.IsNullOrEmpty(Parameters.Tag))
			{
				HashSet<FileReference> Files = FindOrAddTagSet(TagNameToFileSet, Parameters.Tag);
				foreach(FileReference File in Files.OrderBy(x => x.FullName))
				{
					CommandUtils.Log("  {0}", File.FullName);
				}
			}

			// Print the contents of the given file, if specified
			if(!String.IsNullOrEmpty(Parameters.File))
			{
				FileReference FileToRead = new FileReference(Path.Combine(CommandUtils.CmdEnv.LocalRoot, Parameters.File));
				if(FileToRead.Exists())
				{
					foreach(string Line in File.ReadAllLines(FileToRead.FullName))
					{
						CommandUtils.Log("  {0}", Line);
					}
				}
				else
				{
					CommandUtils.Log("  No file found named - {0}", FileToRead.FullName);
				}
			}

			return true;
		}
	}
}
