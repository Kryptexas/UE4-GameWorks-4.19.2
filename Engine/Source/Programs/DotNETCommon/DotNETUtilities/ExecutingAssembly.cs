// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;

namespace Tools.DotNETCommon.ExecutingAssembly
{
	public static class ExecutingAssembly
	{
		/// <summary>
		/// Gets the executing assembly path (including filename).
		/// This method is using Assembly.CodeBase property to properly resolve original
		/// assembly path in case shadow copying is enabled.
		/// </summary>
		/// <returns>Absolute path to the executing assembly including the assembly filename.</returns>
		public static string GetFilename()
		{
			return new Uri(System.Reflection.Assembly.GetExecutingAssembly().CodeBase).LocalPath;
		}

		/// <summary>
		/// Gets the executing assembly directory.
		/// This method is using Assembly.CodeBase property to properly resolve original
		/// assembly directory in case shadow copying is enabled.
		/// </summary>
		/// <returns>Absolute path to the directory containing the executing assembly.</returns>
		public static string GetDirectory()
		{
			return Path.GetDirectoryName(GetFilename());
		}
	}
}
