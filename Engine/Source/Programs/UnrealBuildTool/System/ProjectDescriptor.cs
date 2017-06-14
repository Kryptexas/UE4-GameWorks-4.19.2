// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace UnrealBuildTool
{
	/// <summary>
	/// The version format for .uproject files. This rarely changes now; project descriptors should maintain backwards compatibility automatically.
	/// </summary>
	enum ProjectDescriptorVersion
	{
		/// <summary>
		/// Invalid
		/// </summary>
		Invalid = 0,

		/// <summary>
		/// Initial version
		/// </summary>
		Initial = 1,

		/// <summary>
		/// Adding SampleNameHash
		/// </summary>
		NameHash = 2,

		/// <summary>
		/// Unifying plugin/project files (since abandoned, but backwards compatibility maintained)
		/// </summary>
		ProjectPluginUnification = 3,

		/// <summary>
        /// This needs to be the last line, so we can calculate the value of Latest below
		/// </summary>
        LatestPlusOne,

		/// <summary>
		/// The latest plugin descriptor version
		/// </summary>
		Latest = LatestPlusOne - 1
	}

	/// <summary>
	/// In-memory representation of a .uproject file
	/// </summary>
	public class ProjectDescriptor
	{
		/// <summary>
		/// Descriptor version number.
		/// </summary>
		public int FileVersion;

		/// <summary>
		/// The engine to open this project with.
		/// </summary>
		public string EngineAssociation;

		/// <summary>
		/// Category to show under the project browser
		/// </summary>
		public string Category;

		/// <summary>
		/// Description to show in the project browser
		/// </summary>
		public string Description;

		/// <summary>
		/// List of all modules associated with this project
		/// </summary>
		public ModuleDescriptor[] Modules;

		/// <summary>
		/// List of plugins for this project (may be enabled/disabled)
		/// </summary>
		public PluginReferenceDescriptor[] Plugins;

		/// <summary>
        /// List of additional plugin directories to scan for available plugins
		/// </summary>
        public List<DirectoryReference> AdditionalPluginDirectories;

		/// <summary>
		/// Array of platforms that this project is targeting
		/// </summary>
		public string[] TargetPlatforms;

		/// <summary>
		/// A hash that is used to determine if the project was forked from a sample
		/// </summary>
		public uint EpicSampleNameHash;

		/// <summary>
		/// Steps to execute before building targets in this project
		/// </summary>
		public CustomBuildSteps PreBuildSteps;

		/// <summary>
		/// Steps to execute before building targets in this project
		/// </summary>
		public CustomBuildSteps PostBuildSteps;

		/// <summary>
		/// Indicates if this project is an Enterprise project
		/// </summary>
		public bool IsEnterpriseProject;

		/// <summary>
		/// Constructor.
		/// </summary>
		public ProjectDescriptor()
		{
			FileVersion = (int)ProjectDescriptorVersion.Latest;
			IsEnterpriseProject = false;
		}

		/// <summary>
		/// Creates a plugin descriptor from a file on disk
		/// </summary>
		/// <param name="FileName">The filename to read</param>
		/// <returns>New plugin descriptor</returns>
		public static ProjectDescriptor FromFile(string FileName)
		{
			JsonObject RawObject = JsonObject.Read(FileName);
			try
			{
				ProjectDescriptor Descriptor = new ProjectDescriptor();

				// Read the version
				if (!RawObject.TryGetIntegerField("FileVersion", out Descriptor.FileVersion))
				{
					if (!RawObject.TryGetIntegerField("ProjectFileVersion", out Descriptor.FileVersion))
					{
						throw new BuildException("Project descriptor '{0}' does not contain a valid FileVersion entry", FileName);
					}
				}

				// Check it's not newer than the latest version we can parse
				if (Descriptor.FileVersion > (int)PluginDescriptorVersion.Latest)
				{
					throw new BuildException("Project descriptor '{0}' appears to be in a newer version ({1}) of the file format that we can load (max version: {2}).", FileName, Descriptor.FileVersion, (int)ProjectDescriptorVersion.Latest);
				}

				// Read simple fields
				RawObject.TryGetStringField("EngineAssociation", out Descriptor.EngineAssociation);
				RawObject.TryGetStringField("Category", out Descriptor.Category);
				RawObject.TryGetStringField("Description", out Descriptor.Description);
				RawObject.TryGetBoolField("Enterprise", out Descriptor.IsEnterpriseProject);

				// Read the modules
				JsonObject[] ModulesArray;
				if (RawObject.TryGetObjectArrayField("Modules", out ModulesArray))
				{
					Descriptor.Modules = Array.ConvertAll(ModulesArray, x => ModuleDescriptor.FromJsonObject(x));
				}

				// Read the plugins
				JsonObject[] PluginsArray;
				if (RawObject.TryGetObjectArrayField("Plugins", out PluginsArray))
				{
					Descriptor.Plugins = Array.ConvertAll(PluginsArray, x => PluginReferenceDescriptor.FromJsonObject(x));
				}

                string[] Dirs;
                Descriptor.AdditionalPluginDirectories = new List<DirectoryReference>();
                // Read the additional plugin directories
                if (RawObject.TryGetStringArrayField("AdditionalPluginDirectories", out Dirs))
                {
                    for (int Index = 0; Index < Dirs.Length; Index++)
                    {
                        if (Path.IsPathRooted(Dirs[Index]))
                        {
                            // Absolute path so create in place
                            Descriptor.AdditionalPluginDirectories.Add(new DirectoryReference(Dirs[Index]));
                            Log.TraceVerbose("Project ({0}) : Added additional absolute plugin directory ({1})", FileName, Dirs[Index]);
                        }
                        else
                        {
                            // This path is relative to the project path so build that out
                            string RelativePath = Path.Combine(Path.GetDirectoryName(FileName), Dirs[Index]);
                            Descriptor.AdditionalPluginDirectories.Add(new DirectoryReference(RelativePath));
                            Log.TraceVerbose("Project ({0}) : Added additional relative plugin directory ({1})", FileName, Dirs[Index]);
                        }
                    }
                }

                // Read the target platforms
                RawObject.TryGetStringArrayField("TargetPlatforms", out Descriptor.TargetPlatforms);

                // Get the sample name hash
                RawObject.TryGetUnsignedIntegerField("EpicSampleNameHash", out Descriptor.EpicSampleNameHash);

				// Read the pre and post-build steps
				CustomBuildSteps.TryRead(RawObject, "PreBuildSteps", out Descriptor.PreBuildSteps);
				CustomBuildSteps.TryRead(RawObject, "PostBuildSteps", out Descriptor.PostBuildSteps);

				return Descriptor;
			}
			catch (JsonParseException ParseException)
			{
				throw new JsonParseException("{0} (in {1})", ParseException.Message, FileName);
			}
		}
	}
}
