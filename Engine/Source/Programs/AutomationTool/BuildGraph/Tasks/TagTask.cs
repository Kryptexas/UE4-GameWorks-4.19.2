using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnrealBuildTool;

namespace AutomationTool.Tasks
{
	/// <summary>
	/// Parameters for the Tag task.
	/// </summary>
	public class TagTaskParameters
	{
		/// <summary>
		/// Set the base directory for patterns to be matched against
		/// </summary>
		[TaskParameter(Optional = true)]
		public string BaseDirectory;

		/// <summary>
		/// List of tags to use as the source files. If not specified, files will be enumerated from disk (under BaseDirectory) to match the patterns.
		/// </summary>
		[TaskParameter(Optional = true, ValidationType = TaskParameterValidationType.Tag)]
		public string From;

		/// <summary>
		/// Patterns to filter the list of files by. May include tag names or patterns that apply to the base directory. Defaults to all files if not specified.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string Files;

		/// <summary>
		/// Set of patterns to exclude from the matched list. May include tag names of patterns that apply to the base directory.
		/// </summary>
		[TaskParameter(Optional = true)]
		public string Except;

		/// <summary>
		/// Name of the tag to apply
		/// </summary>
		[TaskParameter(ValidationType = TaskParameterValidationType.Tag)]
		public string With;
	}

	/// <summary>
	/// Task which applies a tag to a given set of files. Filtering is performed using the following method:
	/// * A set of files is enumerated from the tags and file specifications given by the "Files" parameter
	/// * Any files not matched by the "Filter" parameter are removed.
	/// * Any files matched by the "Except" parameter are removed.
	/// </summary>
	[TaskElement("Tag", typeof(TagTaskParameters))]
	class TagTask : CustomTask
	{
		/// <summary>
		/// Parameters to this task
		/// </summary>
		TagTaskParameters Parameters;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InParameters">Parameters to select which files to match</param>
		/// <param name="Type"></param>
		public TagTask(TagTaskParameters InParameters)
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
			// Find the base directory
			DirectoryReference BaseDirectory;
			if(Parameters.BaseDirectory == null)
			{
				BaseDirectory = new DirectoryReference(CommandUtils.CmdEnv.LocalRoot);
			}
			else if(Path.IsPathRooted(Parameters.BaseDirectory))
			{
				BaseDirectory = new DirectoryReference(Parameters.BaseDirectory);
			}
			else
			{
				BaseDirectory = new DirectoryReference(Path.Combine(CommandUtils.CmdEnv.LocalRoot, Parameters.BaseDirectory));
			}

			// Build a filter to select the files
			FileFilter Filter = new FileFilter(FileFilterType.Exclude);
			if(Parameters.Files == null)
			{
				Filter.AddRule("...", FileFilterType.Include);
			}
			else
			{
				Filter.AddRules(SplitDelimitedList(Parameters.Files), FileFilterType.Include);
			}

			// Add the exclude rules
			if(Parameters.Except != null)
			{
				Filter.AddRules(SplitDelimitedList(Parameters.Except), FileFilterType.Exclude);
			}

			// Find the input files
			HashSet<FileReference> InputFiles = null;
			if(Parameters.From != null)
			{
				InputFiles = new HashSet<FileReference>();
				foreach(string TagName in SplitDelimitedList(Parameters.From))
				{
					InputFiles.UnionWith(FindOrAddTagSet(TagNameToFileSet, TagName));
				}
			}

			// Match the files and add them to the tag set
			HashSet<FileReference> Files = FindOrAddTagSet(TagNameToFileSet, Parameters.With);
			if(InputFiles == null)
			{
				Files.UnionWith(Filter.ApplyToDirectory(BaseDirectory, true));
			}
			else
			{
				Files.UnionWith(Filter.ApplyTo(BaseDirectory, InputFiles));
			}
			return true;
		}
	}
}
