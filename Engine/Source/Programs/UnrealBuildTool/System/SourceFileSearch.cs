// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Linq;

namespace UnrealBuildTool
{
	public static class SourceFileSearch
	{
		private delegate void FileSearchDelegate(IEnumerable<string> FileList);


		/// Finds mouse source files
		public static List<FileReference> FindModuleSourceFiles( FileReference ModuleRulesFile, bool SearchSubdirectories = true )
		{
			// The module's "base directory" is simply the directory where its xxx.Build.cs file is stored.  We'll always
			// harvest source files for this module in this base directory directory and all of its sub-directories.
			var ModuleBaseDirectory = ModuleRulesFile.Directory;

			// Find all of the source files (and other files) and add them to the project
			var DirectoriesToSearch = new List<DirectoryReference>();
			DirectoriesToSearch.Add( ModuleBaseDirectory );
			var FoundFiles = SourceFileSearch.FindFiles( DirectoriesToSearch, SubdirectoryNamesToExclude:null, SearchSubdirectories:SearchSubdirectories );

			return FoundFiles;
		}


		/// <summary>
		/// Fills in a project file with source files discovered in the specified list of paths
		/// </summary>
		/// <param name="SourceFilePaths">List of paths to look for source files</param>
		/// <param name="SubdirectoryNamesToExclude">Directory base names to ignore when searching subdirectories.  Can be null.</param>
		/// <param name="SearchSubdirectories">True to include subdirectories, otherwise we only search the list of base directories</param>
		/// <param name="IncludePrivateSourceCode">True if source files in the 'Private' directory should be included</param>
		public static List<FileReference> FindFiles( List<DirectoryReference> DirectoriesToSearch, List<string> SubdirectoryNamesToExclude = null, bool SearchSubdirectories = true )
		{
			// Certain file types should never be added to project files
			var ExcludedFileTypes = new string[] 
						{
							".vcxproj",				// Visual Studio project files
							".vcxproj.filters",		// Visual Studio filter file
							".vcxproj.user",		// Visual Studio project user file
							".vcxproj.vspscc",		// Visual Studio source control state
							".sln",					// Visual Studio solution file
							".sdf",					// Visual Studio embedded symbol database file
							".suo",					// Visual Studio solution option files
							".sln.docstates.suo",	// Visual Studio temp file
							".tmp",					// Temp files used by P4 diffing (e.g. t6884t87698.tmp)
							".csproj",				// Visual Studio C# project files
							".userprefs",			// MonoDevelop project settings
							".DS_Store",			// Mac Desktop Services Store hidden files
						};
			var ExcludedFileNames = new string[] 
						{
							"DO_NOT_DELETE.txt",	// Placeholder .txt file in P4
						};

			HashSet<FileReference> FoundFiles = new HashSet<FileReference>();
			foreach( var SearchDirectory in DirectoriesToSearch )
			{
				var FoundFileUnderIntermediateDirectory = string.Empty;

				// Look for files to add to the project within this module's directory
				// NOTE: Due to issue with current Mono release (2.10.x), Directory.EnumerateFiles does not
				// work correctly. Therefore, the logic here is encapsulated inside a delegate, and called
				// with either the result of EnumerateFiles or GetFiles, depending on platform.
				FileSearchDelegate DoSearch = delegate(IEnumerable<string> FileList) 
				{
					foreach(var CurSourceFilePath in FileList)
					{
						// Get the part of the path that is relative to the module folder
						var RelativeSourceFilePath = new FileReference(CurSourceFilePath).MakeRelativeTo(SearchDirectory);

						bool IncludeThisFile = true;

						// Mask out blacklisted folders
						if( IncludeThisFile && SubdirectoryNamesToExclude != null )
						{
							foreach( var BlacklistedSubdirectoryName in SubdirectoryNamesToExclude )
							{
								if( RelativeSourceFilePath.StartsWith( BlacklistedSubdirectoryName + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase ) ||
									RelativeSourceFilePath.IndexOf( Path.DirectorySeparatorChar + BlacklistedSubdirectoryName + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase ) != -1 )
								{
									IncludeThisFile = false;
									break;
								}
							}
						}

						// Mask out blacklisted file extensions
						if( IncludeThisFile )
						{
							foreach( var BlacklistedFileExtension in ExcludedFileTypes )
							{
								if( RelativeSourceFilePath.EndsWith( BlacklistedFileExtension, StringComparison.InvariantCultureIgnoreCase ) )
								{
									IncludeThisFile = false;
									break;
								}
							}

							if( IncludeThisFile )
							{
								foreach( var BlacklistedFileName in ExcludedFileNames )
								{
									if( Path.GetFileName( RelativeSourceFilePath ).Equals( BlacklistedFileName, StringComparison.InvariantCultureIgnoreCase ) )
									{
										IncludeThisFile = false;
										break;
									}
								}
							}
						}

						// Watch out for "Intermediate" folders.  Module folders should not usually have an Intermediate subfolder, but we'll check for this in case
						// there is something stale or broken; Our module is not going to compile properly if it tries to pull in existing intermediates as source!
						if( IncludeThisFile )
						{ 
							if( RelativeSourceFilePath.StartsWith("Intermediate" + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase) ||
								RelativeSourceFilePath.IndexOf(Path.DirectorySeparatorChar + "Intermediate" + Path.DirectorySeparatorChar, StringComparison.InvariantCultureIgnoreCase) != -1)
							{
								if( !String.IsNullOrEmpty( FoundFileUnderIntermediateDirectory ) )
								{ 
									// Save off at least one example of a file that was bad, so that we can print a warning later
									FoundFileUnderIntermediateDirectory = CurSourceFilePath;
								}
								IncludeThisFile = false;
							}
						}

						if( IncludeThisFile )
						{
							// Don't add duplicates
							if( !FoundFiles.Contains( new FileReference(CurSourceFilePath) ) )
							{
								FoundFiles.Add( new FileReference(CurSourceFilePath) );
							}
						}
					}
				};

				// call the delegate to do the work, based on platform (see note above)
				if(!Utils.IsRunningOnMono)
				{
					DoSearch(Directory.EnumerateFiles(SearchDirectory.FullName, "*.*", SearchSubdirectories ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly ));
				}
				else
				{
					DoSearch(Directory.GetFiles(SearchDirectory.FullName, "*.*", SearchSubdirectories ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly).AsEnumerable());
				}

				if( !string.IsNullOrEmpty( FoundFileUnderIntermediateDirectory ) )
				{
					Console.WriteLine("WARNING: UnrealBuildTool skipped at least one file in an Intermediate folder while gathering module files in directory '{0}' (file: '{1}').  This is unexpected, please remove this Intermediate directory.", SearchDirectory, FoundFileUnderIntermediateDirectory );
				}
			}

			return FoundFiles.ToList();
		}

	}
}
