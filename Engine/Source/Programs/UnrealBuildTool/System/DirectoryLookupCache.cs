// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using System.Collections.Generic;

namespace UnrealBuildTool
{
	class DirectoryCache
	{
		public DirectoryCache( DirectoryReference BaseDirectory )
		{
			if( BaseDirectory.Exists() )
			{
				Files = new HashSet<FileReference>(BaseDirectory.EnumerateFileReferences());
			}
			else
			{
				Files = new HashSet<FileReference>();
			}
		}

		public bool HasFile( FileReference File )
		{
			return Files.Contains(File);
		}

		HashSet<FileReference> Files;
	}


	public static class DirectoryLookupCache
	{
		static public bool FileExists( FileReference FileFullPath )
		{
			DirectoryReference FileDirectory = FileFullPath.Directory;

			DirectoryCache FoundDirectoryCache;
			if( !Directories.TryGetValue( FileDirectory, out FoundDirectoryCache ) )
			{
				// First time we've seen this directory.  Gather information about files.
				FoundDirectoryCache = new DirectoryCache( FileDirectory );
				Directories.Add( FileDirectory, FoundDirectoryCache );
			}

			// Try to find the file in this directory
			return FoundDirectoryCache.HasFile( FileFullPath );
		}

		static Dictionary<DirectoryReference, DirectoryCache> Directories = new Dictionary<DirectoryReference, DirectoryCache>();
	}
}
