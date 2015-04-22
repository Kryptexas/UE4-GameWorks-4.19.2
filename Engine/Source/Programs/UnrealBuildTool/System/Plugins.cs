// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Linq;

namespace UnrealBuildTool
{
	public class PluginInfo
	{
		public enum LoadedFromType
		{
			// Plugin is built-in to the engine
			Engine,

			// Project-specific plugin, stored within a game project directory
			GameProject
		};
		
		// Plugin name
		public string Name;

		// Path to the plugin's root directory
		public string Directory;

		// The plugin descriptor
		public PluginDescriptor Descriptor;

		// Where does this plugin live?
		public LoadedFromType LoadedFrom;

		public override string ToString()
		{
			return Path.Combine(Directory, Name + ".uplugin");
		}
	}

	public class Plugins
	{
		/// <summary>
		/// Loads a plugin descriptor file and fills out a new PluginInfo structure.  Throws an exception on failure.
		/// </summary>
		/// <param name="PluginFile">The path to the plugin file to load</param>
		/// <param name="LoadedFrom">Where the plugin was loaded from</param>
		/// <returns>New PluginInfo for the loaded descriptor.</returns>
		public static PluginInfo ReadPlugin(string PluginFileName, PluginInfo.LoadedFromType LoadedFrom)
		{
			// Create the plugin info object
			PluginInfo Info = new PluginInfo();
			Info.LoadedFrom = LoadedFrom;
			Info.Directory = Path.GetDirectoryName(Path.GetFullPath(PluginFileName));
			Info.Name = Path.GetFileName(Info.Directory);
			Info.Descriptor = PluginDescriptor.FromFile(PluginFileName);
			return Info;
		}

		/// <summary>
		/// Read all the plugins available to a given project
		/// </summary>
		/// <param name="ProjectFileName">Path to the project file (or null)</param>
		/// <returns>Sequence of PluginInfo objects, one for each discovered plugin</returns>
		public static List<PluginInfo> ReadAvailablePlugins(string ProjectFileName)
		{
			List<PluginInfo> Plugins = new List<PluginInfo>();

			// Read all the engine plugins
			string EnginePluginsDir = Path.Combine(BuildConfiguration.RelativeEnginePath, "Plugins");
			foreach(string PluginFileName in EnumeratePlugins(EnginePluginsDir))
			{
				PluginInfo Plugin = ReadPlugin(PluginFileName, PluginInfo.LoadedFromType.Engine);
				Plugins.Add(Plugin);
			}

			// Read all the project plugins
			if(!String.IsNullOrEmpty(ProjectFileName))
			{
				string ProjectPluginsDir = Path.Combine(Path.GetDirectoryName(ProjectFileName), "Plugins");
				foreach(string PluginFileName in EnumeratePlugins(ProjectPluginsDir))
				{
					PluginInfo Plugin = ReadPlugin(PluginFileName, PluginInfo.LoadedFromType.GameProject);
					Plugins.Add(Plugin);
				}
			}

			return Plugins;
		}

		/// <summary>
		/// Find paths to all the plugins under a given parent directory (recursively)
		/// </summary>
		/// <param name="ParentDirectory">Parent directory to look in. Plugins will be found in any *subfolders* of this directory.</param>
		/// <param name="FileNames">List of filenames. Will have all the discovered .uplugin files appended to it.</param>
		public static IEnumerable<string> EnumeratePlugins(string ParentDirectory)
		{
			List<string> FileNames = new List<string>();
			if(Directory.Exists(ParentDirectory))
			{
				EnumeratePluginsInternal(new DirectoryInfo(ParentDirectory), FileNames);
			}
			return FileNames;
		}

		/// <summary>
		/// Find paths to all the plugins under a given parent directory (recursively)
		/// </summary>
		/// <param name="ParentDirectory">Parent directory to look in. Plugins will be found in any *subfolders* of this directory.</param>
		/// <param name="FileNames">List of filenames. Will have all the discovered .uplugin files appended to it.</param>
		static void EnumeratePluginsInternal(DirectoryInfo ParentDirectory, List<string> FileNames)
		{
			foreach(DirectoryInfo ChildDirectory in ParentDirectory.EnumerateDirectories())
			{
				int InitialFileNamesCount = FileNames.Count;
				foreach(FileInfo PluginFileInfo in ChildDirectory.EnumerateFiles("*.uplugin", SearchOption.TopDirectoryOnly))
				{
					FileNames.Add(PluginFileInfo.FullName);
				}
				if(FileNames.Count == InitialFileNamesCount)
				{
					EnumeratePluginsInternal(ChildDirectory, FileNames);
				}
			}
		}
	}
}
