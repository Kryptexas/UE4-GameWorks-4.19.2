using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using UnrealBuildTool;

namespace AutomationTool
{
	/// <summary>
	/// Specifies validation that should be performed on a task parameter.
	/// </summary>
	public enum TaskParameterValidationType
	{
		/// <summary>
		/// Allow any valid values for the field type.
		/// </summary>
		Default,

		/// <summary>
		/// A standard name; alphanumeric characters, plus underscore and space. Spaces at the start or end, or more than one in a row are prohibited.
		/// </summary>
		Name,

		/// <summary>
		/// A list of names separated by semicolons
		/// </summary>
		NameList,

		/// <summary>
		/// A tag name (a regular name with an optional '#' prefix)
		/// </summary>
		Tag,

		/// <summary>
		/// A list of tag names separated by semicolons
		/// </summary>
		TagList,
	}

	/// <summary>
	/// Attribute to mark parameters to a task, which should be read as XML attributes from the script file.
	/// </summary>
	[AttributeUsage(AttributeTargets.Field | AttributeTargets.Property)]
	public class TaskParameterAttribute : Attribute
	{
		/// <summary>
		/// Whether the parameter can be omitted
		/// </summary>
		public bool Optional
		{
			get;
			set;
		}

		/// <summary>
		/// Sets additional restrictions on how this field is validated in the schema. Default is to allow any valid field type.
		/// </summary>
		public TaskParameterValidationType ValidationType
		{
			get;
			set;
		}
	}

	/// <summary>
	/// Attribute used to associate an XML element name with a parameter block that can be used to construct tasks
	/// </summary>
	[AttributeUsage(AttributeTargets.Class)]
	public class TaskElementAttribute : Attribute
	{
		/// <summary>
		/// Name of the XML element that can be used to denote this class
		/// </summary>
		public string Name;

		/// <summary>
		/// Type to be constructed from the deserialized element
		/// </summary>
		public Type ParametersType;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="InName">Name of the XML element used to denote this object</param>
		/// <param name="InParametersType">Type to be constructed from this object</param>
		public TaskElementAttribute(string InName, Type InParametersType)
		{
			Name = InName;
			ParametersType =  InParametersType;
		}
	}

	/// <summary>
	/// Base class for all custom build tasks
	/// </summary>
	public abstract class CustomTask
	{
		/// <summary>
		/// Allow this task to merge with other tasks within the same node if it can. This can be useful to allow tasks to execute in parallel.
		/// </summary>
		/// <param name="OtherTasks">Other tasks that this task can merge with. If a merge takes place, the other tasks should be removed from the list.</param>
		public virtual void Merge(List<CustomTask> OtherTasks)
		{
		}

		/// <summary>
		/// Execute this node.
		/// </summary>
		/// <param name="Job">Information about the current job</param>
		/// <param name="BuildProducts">Set of build products produced by this node.</param>
		/// <param name="TagNameToFileSet">Mapping from tag names to the set of files they include</param>
		/// <returns>Whether the task succeeded or not. Exiting with an exception will be caught and treated as a failure.</returns>
		public abstract bool Execute(JobContext Job, HashSet<FileReference> BuildProducts, Dictionary<string, HashSet<FileReference>> TagNameToFileSet);

		/// <summary>
		/// Resolves a single name to a file reference, resolving relative paths to the root of the current path.
		/// </summary>
		/// <param name="Name">Name of the file</param>
		/// <returns>Fully qualified file reference</returns>
		public static FileReference ResolveFile(string Name)
		{
			if(Path.IsPathRooted(Name))
			{
				return new FileReference(Name);
			}
			else
			{
				return new FileReference(Path.Combine(CommandUtils.CmdEnv.LocalRoot, Name));
			}
		}

		/// <summary>
		/// Resolves a directory reference from the given string. Assumes the root directory is the root of the current branch.
		/// </summary>
		/// <param name="Name">Name of the directory. May be null or empty.</param>
		/// <returns>The resolved directory</returns>
		public static DirectoryReference ResolveDirectory(string Name)
		{
			if(String.IsNullOrEmpty(Name))
			{
				return new DirectoryReference(CommandUtils.CmdEnv.LocalRoot);
			}
			else if(Path.IsPathRooted(Name))
			{
				return new DirectoryReference(Name);
			}
			else
			{
				return new DirectoryReference(Path.Combine(CommandUtils.CmdEnv.LocalRoot, Name));
			}
		}

		/// <summary>
		/// Finds or adds a set containing files with the given tag
		/// </summary>
		/// <param name="Name">The tag name to return a set for. An leading '#' character is optional.</param>
		/// <returns>Set of files</returns>
		public static HashSet<FileReference> FindOrAddTagSet(Dictionary<string, HashSet<FileReference>> TagNameToFileSet, string Name)
		{
			// Get the clean tag name, without the leading '#' character
			string TagName = Name.StartsWith("#")? Name.Substring(1) : Name;

			// Find the files which match this tag
			HashSet<FileReference> Files;
			if(!TagNameToFileSet.TryGetValue(TagName, out Files))
			{
				Files = new HashSet<FileReference>();
				TagNameToFileSet.Add(TagName, Files);
			}

			// If we got a null reference, it's because the tag is not listed as an input for this node (see RunGraph.BuildSingleNode). Fill it in, but only with an error.
			if(Files == null)
			{
				CommandUtils.LogError("Attempt to reference tag '{0}', which is not listed as a dependency of this node.", Name);
				Files = new HashSet<FileReference>();
				TagNameToFileSet.Add(TagName, Files);
			}
			return Files;
		}

		/// <summary>
		/// Resolve a list of file specifications separated by semicolons, each of which may be:
		///   a) A p4-style wildcard as acceped by FileFilter (eg. *.cpp;Engine/.../*.bat)
		///   b) The name of a tag set (eg. #CompiledBinaries)
		/// Only rules which include files are supported - files cannot be excluded using "-Pattern" syntax (because wildcards are not run on the tag set).
		/// </summary>
		/// <param name="BaseDirectory">The base directory to match files</param>
		/// <param name="IncludePatternList">The list of patterns which select files to include in the match. See above.</param>
		/// <param name="IncludePatternList">Patterns of files to exclude from the match.</param>
		/// <param name="TagNameToFileSet">Mapping of tag name to fileset, as passed to the Execute() method</param>
		/// <returns>Set of matching files.</returns>
		public static HashSet<FileReference> ResolveFilespec(DirectoryReference BaseDirectory, string[] Patterns, Dictionary<string, HashSet<FileReference>> TagNameToFileSet)
		{
			HashSet<FileReference> Files = new HashSet<FileReference>();

			// Add all the tagged files directly into the output set, and add any patterns into a filter that gets applied to the directory
			FileFilter Filter = new FileFilter(FileFilterType.Exclude);
			foreach(string Pattern in Patterns)
			{
				if(Pattern.StartsWith("#"))
				{
					Files.UnionWith(FindOrAddTagSet(TagNameToFileSet, Pattern.Substring(1)));
				}
				else
				{
					Filter.AddRule(Pattern, FileFilterType.Include);
				}
			}

			// Include the files from disk, and return the full set
			Files.UnionWith(Filter.ApplyToDirectory(BaseDirectory, true));
			return Files;
		}

		/// <summary>
		/// Splits a string separated by semicolons into a list, removing empty entries
		/// </summary>
		/// <param name="Text">The input string</param>
		/// <returns>Array of the parsed items</returns>
		public static string[] SplitDelimitedList(string Text)
		{
			return Text.Split(';').Select(x => x.Trim()).Where(x => x.Length > 0).ToArray();
		}
	}
}
