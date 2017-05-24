using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnrealBuildTool
{
	/// <summary>
	/// Static directory methods. Contains a superset of functionality to the System.IO.Directory namespace, but takes FileReference and DirectoryReference parameters.
	/// </summary>
	public static class DirectoryExtensions
	{
		/// <summary>
		/// Creates the directory
		/// </summary>
		public static void CreateDirectory(DirectoryReference Location)
		{
			Directory.CreateDirectory(Location.FullName);
		}

        /// <summary>
        /// Removes the given directory
        /// </summary>
        public static void Delete(DirectoryReference Location)
        {
            Directory.Delete(Location.FullName);
        }

        /// <summary>
        /// Removes the given directory
        /// </summary>
        public static void Delete(DirectoryReference Location, bool bRecursive)
        {
            Directory.Delete(Location.FullName, bRecursive);
        }

		/// <summary>
		/// Checks whether a directory exists
		/// </summary>
		/// <param name="Location">Directory to check</param>
		/// <returns>True if the directory exists, false otherwise</returns>
		public static bool Exists(DirectoryReference Location)
		{
			return Directory.Exists(Location.FullName);
		}

		/// <summary>
		/// Enumerate files from a given directory
		/// </summary>
		/// <returns>Sequence of file references</returns>
		public static IEnumerable<FileReference> EnumerateFiles(DirectoryReference Location)
		{
			foreach (string FileName in Directory.EnumerateFiles(Location.FullName))
			{
				yield return FileReference.MakeFromNormalizedFullPath(FileName);
			}
		}

		/// <summary>
		/// Enumerate files from a given directory
		/// </summary>
		/// <returns>Sequence of file references</returns>
		public static IEnumerable<FileReference> EnumerateFiles(DirectoryReference Location, string Pattern)
		{
			foreach (string FileName in Directory.EnumerateFiles(Location.FullName, Pattern))
			{
				yield return FileReference.MakeFromNormalizedFullPath(FileName);
			}
		}

		/// <summary>
		/// Enumerate files from a given directory
		/// </summary>
		/// <returns>Sequence of file references</returns>
		public static IEnumerable<FileReference> EnumerateFiles(DirectoryReference Location, string Pattern, SearchOption Option)
		{
			foreach (string FileName in Directory.EnumerateFiles(Location.FullName, Pattern, Option))
			{
				yield return FileReference.MakeFromNormalizedFullPath(FileName);
			}
		}

		/// <summary>
		/// Enumerate subdirectories in a given directory
		/// </summary>
		/// <returns>Sequence of directory references</returns>
		public static IEnumerable<DirectoryReference> EnumerateDirectories(DirectoryReference Location)
		{
			foreach (string DirectoryName in Directory.EnumerateDirectories(Location.FullName))
			{
				yield return DirectoryReference.MakeFromNormalizedFullPath(DirectoryName);
			}
		}

		/// <summary>
		/// Enumerate subdirectories in a given directory
		/// </summary>
		/// <returns>Sequence of directory references</returns>
		public static IEnumerable<DirectoryReference> EnumerateDirectories(DirectoryReference Location, string Pattern)
		{
			foreach (string DirectoryName in Directory.EnumerateDirectories(Location.FullName, Pattern))
			{
				yield return DirectoryReference.MakeFromNormalizedFullPath(DirectoryName);
			}
		}

		/// <summary>
		/// Enumerate subdirectories in a given directory
		/// </summary>
		/// <returns>Sequence of directory references</returns>
		public static IEnumerable<DirectoryReference> EnumerateDirectories(DirectoryReference Location, string Pattern, SearchOption Option)
		{
			foreach (string DirectoryName in Directory.EnumerateDirectories(Location.FullName, Pattern, Option))
			{
				yield return DirectoryReference.MakeFromNormalizedFullPath(DirectoryName);
			}
		}
	}
}
