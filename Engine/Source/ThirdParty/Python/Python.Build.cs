// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
using System.IO;
using System.Collections.Generic;
using UnrealBuildTool;

public class Python : ModuleRules
{
	public Python(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.External;

		string PythonRoot = null;
		string PythonIncludePath = null;
		string PythonLibPath = null;
		string PythonLibName = null;
		
		if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux)
		{
			// Check for an explicit version before using the auto-detection logic
			PythonRoot = System.Environment.GetEnvironmentVariable("UE_PYTHON_DIR");
		}

		// Perform auto-detection to try and find the Python root
		if (PythonRoot == null)
		{
			var KnownPaths = new List<string>();

			// todo: This isn't correct for cross-compilation, we need to consider the host platform too
			if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
			{
				KnownPaths.AddRange(
					new string[] {
						//"C:/Program Files/Python36",
						"C:/Python27",
					}
				);
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				KnownPaths.AddRange(
					new string[] {
						"/Library/Frameworks/Python.framework/Versions/2.7",
						//"/System/Library/Frameworks/Python.framework/Versions/2.7",
					}
				);
			}

			foreach (var KnownPath in KnownPaths)
			{
				if (Directory.Exists(KnownPath))
				{
					PythonRoot = KnownPath;
					break;
				}
			}
		}

		// Work out the include path
		if (PythonRoot != null)
		{
			PythonIncludePath = Path.Combine(PythonRoot, (Target.Platform == UnrealTargetPlatform.Mac) ? "Headers" : "include");
			if (!Directory.Exists(PythonIncludePath))
			{
				PythonRoot = null;
			}
		}

		// Work out the lib path
		if (PythonRoot != null)
		{
			string LibFolder = null;
			string LibNamePattern = null;
			switch (Target.Platform)
			{
				case UnrealTargetPlatform.Win32:
				case UnrealTargetPlatform.Win64:
					LibFolder = "libs";
					LibNamePattern = "python*.lib";
					break;

				case UnrealTargetPlatform.Mac:
					LibFolder = "lib";
					LibNamePattern = "libpython*.dylib";
					break;

				case UnrealTargetPlatform.Linux:
					LibFolder = "lib";
					LibNamePattern = "libpython*.so";
					break;

				default:
					break;
			}

			if (LibFolder != null && LibNamePattern != null)
			{
				PythonLibPath = Path.Combine(PythonRoot, LibFolder);

				if (Directory.Exists(PythonLibPath))
				{
					string[] MatchingLibFiles = Directory.GetFiles(PythonLibPath, LibNamePattern);
					if (MatchingLibFiles.Length > 0)
					{
						PythonLibName = Path.GetFileName(MatchingLibFiles[0]);
					}
				}
			}

			if (PythonLibPath == null || PythonLibName == null)
			{
				PythonRoot = null;
			}
		}
		
		if (PythonRoot == null)
		{
			Definitions.Add("WITH_PYTHON=0");
		}
		else
		{
			Definitions.Add("WITH_PYTHON=1");
			Definitions.Add(string.Format("UE_PYTHON_DIR=\"{0}\"", PythonRoot.Replace('\\', '/')));

			PublicSystemIncludePaths.Add(PythonIncludePath);
			if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				// Mac doesn't understand PublicLibraryPaths
				PublicAdditionalLibraries.Add(Path.Combine(PythonLibPath, PythonLibName));
			}
			else
			{
				PublicLibraryPaths.Add(PythonLibPath);
				PublicAdditionalLibraries.Add(PythonLibName);
			}
		}
	}
}
