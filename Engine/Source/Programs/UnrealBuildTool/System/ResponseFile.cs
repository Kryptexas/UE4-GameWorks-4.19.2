// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace UnrealBuildTool
{
	public class ResponseFile
	{
		/// <summary>
		/// Creates a file from a list of strings; each string is placed on a line in the file.
		/// </summary>
		/// <param name="TempFileName">Name of response file</param>
		/// <param name="Lines">List of lines to write to the response file</param>
		public static string Create(string TempFileName, List<string> Lines)
		{
			FileInfo TempFileInfo = new FileInfo( TempFileName );
			DirectoryInfo TempFolderInfo = new DirectoryInfo( TempFileInfo.DirectoryName );

			// Delete the existing file if it exists
			if( TempFileInfo.Exists )
			{
				TempFileInfo.IsReadOnly = false;
				TempFileInfo.Delete();
				TempFileInfo.Refresh();
			}

			// Create the folder if it doesn't exist
			if( !TempFolderInfo.Exists )
			{
				// Create the 
				TempFolderInfo.Create();
				TempFolderInfo.Refresh();
			}

			using( FileStream Writer = TempFileInfo.OpenWrite() )
			{
				using( StreamWriter TextWriter = new StreamWriter( Writer ) )
				{
					Lines.ForEach( x => TextWriter.WriteLine( x ) );
				}
			}

			return TempFileName;
		}
	}
}
