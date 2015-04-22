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
		/// File extension of plugin descriptor files.  NOTE: This constant exists in UnrealBuildTool code as well.
		/// NOTE: This constant exists in PluginManager C++ code as well.
		private static string PluginDescriptorFileExtension = ".uplugin";



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

		/// <summary>
		/// Recursively locates all plugins in the specified folder, appending to the incoming list
		/// </summary>
		/// <param name="PluginsDirectory">Directory to search</param>
		/// <param name="LoadedFrom">Where we're loading these plugins from</param>
		/// <param name="Plugins">List of plugins found so far</param>
		private static void FindPluginsRecursively(string PluginsDirectory, PluginInfo.LoadedFromType LoadedFrom, ref List<PluginInfo> Plugins)
		{
			// NOTE: The logic in this function generally matches that of the C++ code for FindPluginsRecursively
			//       in the core engine code.  These routines should be kept in sync.

			// Each sub-directory is possibly a plugin.  If we find that it contains a plugin, we won't recurse any
			// further -- you can't have plugins within plugins.  If we didn't find a plugin, we'll keep recursing.

			var PluginsDirectoryInfo = new DirectoryInfo(PluginsDirectory);
			foreach( var PossiblePluginDirectory in PluginsDirectoryInfo.EnumerateDirectories() )
			{
				// Do we have a plugin descriptor in this directory?
				bool bFoundPlugin = false;
				foreach (var PluginDescriptorFileName in Directory.GetFiles(PossiblePluginDirectory.FullName, "*" + PluginDescriptorFileExtension))
				{
					// Found a plugin directory!  No need to recurse any further, but make sure it's unique.
					if (!Plugins.Any(x => x.Directory == PossiblePluginDirectory.FullName))
					{
						// Load the plugin info and keep track of it
						var PluginDescriptorFile = new FileInfo(PluginDescriptorFileName);
						var PluginInfo = ReadPlugin(PluginDescriptorFile.FullName, LoadedFrom);

						Plugins.Add(PluginInfo);
						bFoundPlugin = true;
						Log.TraceVerbose("Found plugin in: " + PluginInfo.Directory);
					}

					// No need to search for more plugins
					break;
				}

				if (!bFoundPlugin)
				{
					// Didn't find a plugin in this directory.  Continue to look in subfolders.
					FindPluginsRecursively(PossiblePluginDirectory.FullName, LoadedFrom, ref Plugins);
				}
			}
		}


		/// <summary>
		/// Finds all plugins in the specified base directory
		/// </summary>
		/// <param name="PluginsDirectory">Base directory to search.  All subdirectories will be searched, except directories within other plugins.</param>
		/// <param name="LoadedFrom">Where we're loading these plugins from</param>
		/// <param name="Plugins">List of all of the plugins we found</param>
		public static void FindPluginsIn(string PluginsDirectory, PluginInfo.LoadedFromType LoadedFrom, ref List<PluginInfo> Plugins)
		{
			if (Directory.Exists(PluginsDirectory))
			{
				FindPluginsRecursively(PluginsDirectory, LoadedFrom, ref Plugins);
			}
		}


		/// <summary>
		/// Discovers all plugins
		/// </summary>
		private static void DiscoverAllPlugins()
		{
			if( AllPluginsVar == null )	// Only do this search once per session
			{
				AllPluginsVar = new List< PluginInfo >();

				// Engine plugins
				var EnginePluginsDirectory = Path.Combine( ProjectFileGenerator.EngineRelativePath, "Plugins" );
				Plugins.FindPluginsIn( EnginePluginsDirectory, PluginInfo.LoadedFromType.Engine, ref AllPluginsVar );

				// Game plugins
				if (RulesCompiler.AllGameFolders != null)
				{
					foreach (var GameProjectFolder in RulesCompiler.AllGameFolders)
					{
						var GamePluginsDirectory = Path.Combine(GameProjectFolder, "Plugins");
						Plugins.FindPluginsIn(GamePluginsDirectory, PluginInfo.LoadedFromType.GameProject, ref AllPluginsVar);
					}
				}
			}
		}

		/// Access the list of all plugins.  We'll scan for plugins when this is called the first time.
		public static List< PluginInfo > AllPlugins
		{
			get
			{
				DiscoverAllPlugins();
				return AllPluginsVar;
			}
		}

		/// List of all plugins we've found so far in this session
		private static List< PluginInfo > AllPluginsVar = null;
	}
}
