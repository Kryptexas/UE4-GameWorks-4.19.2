// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using System.Reflection;

namespace Tools.DotNETCommon
{
	public static class AssemblyUtils
	{
        /// <summary>
        /// Gets the original location (path and filename) of an assembly.
        /// This method is using Assembly.CodeBase property to properly resolve original
        /// assembly path in case shadow copying is enabled.
        /// </summary>
        /// <returns>Absolute path and filename to the assembly.</returns>
        public static string GetOriginalLocation(this Assembly ThisAssembly)
        {
            return new Uri(ThisAssembly.CodeBase).LocalPath;
        }
	}
}
