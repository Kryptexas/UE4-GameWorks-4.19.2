using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	/// <summary>
	/// Static file methods. Contains similar methods to System.IO.File, but takes FileReference and DirectoryReference parameters.
	/// </summary>
	public static class FileExtensions
	{
		/// <summary>
		/// Checks whether a file exists
		/// </summary>
		/// <param name="Location">File to check</param>
		/// <returns>True if the file exists, false otherwise</returns>
		public static bool Exists(FileReference Location)
		{
			return System.IO.File.Exists(Location.FullName);
		}

		/// <summary>
		/// Deletes the given file
		/// </summary>
		/// <param name="Location">File to delete</param>
		public static void Delete(FileReference Location)
		{
			File.Delete(Location.FullName);
		}

		/// <summary>
		/// Reads the text from a given file
		/// </summary>
		/// <param name="Location">File to read</param>
		/// <returns>Text from the file</returns>
		public static string ReadAllText(FileReference Location)
		{
			return File.ReadAllText(Location.FullName);
		}
	}
}
