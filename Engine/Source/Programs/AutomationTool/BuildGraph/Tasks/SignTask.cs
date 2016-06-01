using AutomationTool;
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
	/// Parameters for a task that strips symbols from a set of files
	/// </summary>
	public class SignTaskParameters
	{
		/// <summary>
		/// List of file specifications separated by semicolons (eg. *.cpp;Engine/.../*.bat), or the name of a tag set
		/// </summary>
		[TaskParameter]
		public string Files;

		/// <summary>
		/// Tag to be applied to build products of this task
		/// </summary>
		[TaskParameter(Optional = true, ValidationType = TaskParameterValidationType.Tag)]
		public string Tag;
	}

	/// <summary>
	/// Task which strips symbols from a set of files
	/// </summary>
	[TaskElement("Sign", typeof(SignTaskParameters))]
	public class SignTask : CustomTask
	{
		/// <summary>
		/// Parameters for this task
		/// </summary>
		SignTaskParameters Parameters;

		/// <summary>
		/// Construct a spawn task
		/// </summary>
		/// <param name="Parameters">Parameters for the task</param>
		public SignTask(SignTaskParameters InParameters)
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
			// Find the matching files
			FileReference[] Files = ResolveFilespec(CommandUtils.RootDirectory, Parameters.Files, TagNameToFileSet).OrderBy(x => x.FullName).ToArray();

			// Sign all the files
			CodeSign.SignMultipleFilesIfEXEOrDLL(Files.Select(x => x.FullName).ToList(), true);

			// Apply the optional tag to the build products
			if(!String.IsNullOrEmpty(Parameters.Tag))
			{
				FindOrAddTagSet(TagNameToFileSet, Parameters.Tag).UnionWith(Files);
			}

			// Add them to the list of build products
			BuildProducts.UnionWith(Files);
			return true;
		}

		/// <summary>
		/// Output this task out to an XML writer.
		/// </summary>
		public override void Write(XmlWriter Writer)
		{
			Write(Writer, Parameters);
		}
	}
}
