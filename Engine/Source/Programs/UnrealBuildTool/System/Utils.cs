// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Serialization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Linq;
using System.Management;
using System.Web.Script.Serialization;

namespace UnrealBuildTool
{
	/// <summary>
	/// Utility functions
	/// </summary>
	public static class Utils
	{
		/// <summary>
		/// Whether we are currently running on Mono platform.  We cache this statically because it is a bit slow to check.
		/// </summary>
		public static readonly bool IsRunningOnMono = Type.GetType("Mono.Runtime") != null;

		/// <summary>
		/// Searches for a flag in a set of command-line arguments.
		/// </summary>
		public static bool ParseCommandLineFlag(string[] Arguments, string FlagName, out int ArgumentIndex)
		{
			// Find an argument with the given name.
			for (ArgumentIndex = 0; ArgumentIndex < Arguments.Length; ArgumentIndex++)
			{
				string Argument = Arguments[ArgumentIndex].ToUpperInvariant();
				if (Argument == FlagName.ToUpperInvariant())
				{
					return true;
				}
			}
			return false;
		}

		/// <summary>
		/// Regular expression to match $(ENV) and/ or %ENV% environment variables.
		/// </summary>
		static Regex EnvironmentVariableRegex = new Regex(@"\$\((.*?)\)|\%(.*?)\%", RegexOptions.None);

		/// <summary>
		/// Resolves $(ENV) and/ or %ENV% to the value of the environment variable in the passed in string.
		/// </summary>
		/// <param name="InString">String to resolve environment variable in.</param>
		/// <returns>String with environment variable expanded/ resolved.</returns>
		public static string ResolveEnvironmentVariable(string InString)
		{
			string Result = InString;

			// Try to find $(ENV) substring.
			Match M = EnvironmentVariableRegex.Match(InString);

			// Iterate over all matches, resolving the match to an environment variable.
			while (M.Success)
			{
				// Convoluted way of stripping first and last character and '(' in the case of $(ENV) to get to ENV
				string EnvironmentVariable = M.ToString();
				if (EnvironmentVariable.StartsWith("$") && EnvironmentVariable.EndsWith(")"))
				{
					EnvironmentVariable = EnvironmentVariable.Substring(1, EnvironmentVariable.Length - 2).Replace("(", "");
				}

				if (EnvironmentVariable.StartsWith("%") && EnvironmentVariable.EndsWith("%"))
				{
					EnvironmentVariable = EnvironmentVariable.Substring(1, EnvironmentVariable.Length - 2);
				}

				// Resolve environment variable.				
				Result = Result.Replace(M.ToString(), Environment.GetEnvironmentVariable(EnvironmentVariable));

				// Move on to next match. Multiple environment variables are handled correctly by regexp.
				M = M.NextMatch();
			}

			return Result;
		}

		/// <summary>
		/// Expands variables in $(VarName) format in the given string. Variables are retrieved from the given dictionary, or through the environment of the current process.
		/// Any unknown variables are ignored.
		/// </summary>
		/// <param name="InputString">String to search for variable names</param>
		/// <param name="AdditionalVariables">Lookup of variable names to values</param>
		/// <returns>String with all variables replaced</returns>
		public static string ExpandVariables(string InputString, Dictionary<string, string> AdditionalVariables = null)
		{
			string Result = InputString;
			for (int Idx = Result.IndexOf("$("); Idx != -1; Idx = Result.IndexOf("$(", Idx))
			{
				// Find the end of the variable name
				int EndIdx = Result.IndexOf(')', Idx + 2);
				if (EndIdx == -1)
				{
					break;
				}

				// Extract the variable name from the string
				string Name = Result.Substring(Idx + 2, EndIdx - (Idx + 2));

				// Find the value for it, either from the dictionary or the environment block
				string Value;
				if (AdditionalVariables == null || !AdditionalVariables.TryGetValue(Name, out Value))
				{
					Value = Environment.GetEnvironmentVariable(Name);
					if (Value == null)
					{
						Idx = EndIdx + 1;
						continue;
					}
				}

				// Replace the variable, or skip past it
				Result = Result.Substring(0, Idx) + Value + Result.Substring(EndIdx + 1);
			}
			return Result;
		}

		/// <summary>
		/// This is a faster replacement of File.ReadAllText. Code snippet based on code
		/// and analysis by Sam Allen
		/// http://dotnetperls.com/Content/File-Handling.aspx
		/// </summary>
		/// <param name="SourceFile"> Source file to fully read and convert to string</param>
		/// <returns>Textual representation of file.</returns>
		public static string ReadAllText(string SourceFile)
		{
			using (StreamReader Reader = new StreamReader(SourceFile, System.Text.Encoding.UTF8))
			{
				return Reader.ReadToEnd();
			}
		}

		/// <summary>
		/// Reads the specified environment variable
		/// </summary>
		/// <param name="VarName"> the environment variable to read</param>
		/// <param name="bDefault">the default value to use if missing</param>
		/// <returns>the value of the environment variable if found and the default value if missing</returns>
		public static bool GetEnvironmentVariable(string VarName, bool bDefault)
		{
			string Value = Environment.GetEnvironmentVariable(VarName);
			if (Value != null)
			{
				// Convert the string to its boolean value
				return Convert.ToBoolean(Value);
			}
			return bDefault;
		}

		/// <summary>
		/// Reads the specified environment variable
		/// </summary>
		/// <param name="VarName"> the environment variable to read</param>
		/// <param name="Default">the default value to use if missing</param>
		/// <returns>the value of the environment variable if found and the default value if missing</returns>
		public static string GetStringEnvironmentVariable(string VarName, string Default)
		{
			string Value = Environment.GetEnvironmentVariable(VarName);
			if (Value != null)
			{
				return Value;
			}
			return Default;
		}

		/// <summary>
		/// Reads the specified environment variable
		/// </summary>
		/// <param name="VarName"> the environment variable to read</param>
		/// <param name="Default">the default value to use if missing</param>
		/// <returns>the value of the environment variable if found and the default value if missing</returns>
		public static double GetEnvironmentVariable(string VarName, double Default)
		{
			string Value = Environment.GetEnvironmentVariable(VarName);
			if (Value != null)
			{
				return Convert.ToDouble(Value);
			}
			return Default;
		}

		/// <summary>
		/// Reads the specified environment variable
		/// </summary>
		/// <param name="VarName"> the environment variable to read</param>
		/// <param name="Default">the default value to use if missing</param>
		/// <returns>the value of the environment variable if found and the default value if missing</returns>
		public static string GetEnvironmentVariable(string VarName, string Default)
		{
			string Value = Environment.GetEnvironmentVariable(VarName);
			if (Value != null)
			{
				return Value;
			}
			return Default;
		}

		/// <summary>
		/// Try to launch a local process, and produce a friendly error message if it fails.
		/// </summary>
		public static int RunLocalProcess(Process LocalProcess)
		{
			int ExitCode = -1;

			// release all process resources
			using (LocalProcess)
			{
				LocalProcess.StartInfo.UseShellExecute = false;
				LocalProcess.StartInfo.RedirectStandardOutput = true;
				LocalProcess.StartInfo.RedirectStandardError = true;

				try
				{
					// Start the process up and then wait for it to finish
					LocalProcess.Start();
					LocalProcess.BeginOutputReadLine();
					LocalProcess.BeginErrorReadLine();
					LocalProcess.WaitForExit();
					ExitCode = LocalProcess.ExitCode;
				}
				catch (Exception ex)
				{
					throw new BuildException(ex, "Failed to start local process for action (\"{0}\"): {1} {2}", ex.Message, LocalProcess.StartInfo.FileName, LocalProcess.StartInfo.Arguments);
				}
			}

			return ExitCode;
		}

		/// <summary>
		/// Runs a local process and pipes the output to the log
		/// </summary>
		public static int RunLocalProcessAndLogOutput(ProcessStartInfo StartInfo)
		{
			Process LocalProcess = new Process();
			LocalProcess.StartInfo = StartInfo;
			LocalProcess.OutputDataReceived += (Sender, Line) => { if (Line != null && Line.Data != null) Log.TraceInformation(Line.Data); };
			LocalProcess.ErrorDataReceived += (Sender, Line) => { if (Line != null && Line.Data != null) Log.TraceError(Line.Data); };
			return RunLocalProcess(LocalProcess);
		}

		/// <summary>
		/// Runs a command line process, and returns simple StdOut output. This doesn't handle errors or return codes
		/// </summary>
		/// <returns>The entire StdOut generated from the process as a single trimmed string</returns>
		/// <param name="Command">Command to run</param>
		/// <param name="Args">Arguments to Command</param>
		public static string RunLocalProcessAndReturnStdOut(string Command, string Args)
		{
			ProcessStartInfo StartInfo = new ProcessStartInfo(Command, Args);
			StartInfo.UseShellExecute = false;
			StartInfo.RedirectStandardOutput = true;
			StartInfo.CreateNoWindow = true;

			string FullOutput = "";
			using (Process LocalProcess = Process.Start(StartInfo))
			{
				StreamReader OutputReader = LocalProcess.StandardOutput;
				// trim off any extraneous new lines, helpful for those one-line outputs
				FullOutput = OutputReader.ReadToEnd().Trim();
			}

			return FullOutput;
		}

		/// <summary>
		/// Find all the platforms in a given class
		/// </summary>
		/// <param name="Class">Class of platforms to return</param>
		/// <returns>Array of platforms in the given class</returns>
		public static UnrealTargetPlatform[] GetPlatformsInClass(UnrealPlatformClass Class)
		{
			switch (Class)
			{
				case UnrealPlatformClass.All:
					return ((UnrealTargetPlatform[])Enum.GetValues(typeof(UnrealTargetPlatform))).Where(x => x != UnrealTargetPlatform.Unknown).ToArray();
				case UnrealPlatformClass.Desktop:
					return new UnrealTargetPlatform[] { UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64, UnrealTargetPlatform.Linux, UnrealTargetPlatform.Mac };
				case UnrealPlatformClass.Editor:
					return new UnrealTargetPlatform[] { UnrealTargetPlatform.Win64, UnrealTargetPlatform.Linux, UnrealTargetPlatform.Mac };
				case UnrealPlatformClass.Server:
					return new UnrealTargetPlatform[] { UnrealTargetPlatform.Win32, UnrealTargetPlatform.Win64, UnrealTargetPlatform.Linux, UnrealTargetPlatform.Mac };
			}
			throw new ArgumentException(String.Format("'{0}' is not a valid value for UnrealPlatformClass", (int)Class));
		}

		/// <summary>
		/// Given a list of supported platforms, returns a list of names of platforms that should not be supported
		/// </summary>
		/// <param name="SupportedPlatforms">List of supported platforms</param>
		/// <returns>List of unsupported platforms in string format</returns>
		public static List<string> MakeListOfUnsupportedPlatforms(List<UnrealTargetPlatform> SupportedPlatforms)
		{
			// Make a list of all platform name strings that we're *not* currently compiling, to speed
			// up file path comparisons later on
			List<string> OtherPlatformNameStrings = new List<string>();
			{
				// look at each group to see if any supported platforms are in it
				List<UnrealPlatformGroup> SupportedGroups = new List<UnrealPlatformGroup>();
				foreach (UnrealPlatformGroup Group in Enum.GetValues(typeof(UnrealPlatformGroup)))
				{
					// get the list of platforms registered to this group, if any
					List<UnrealTargetPlatform> Platforms = UEBuildPlatform.GetPlatformsInGroup(Group);
					if (Platforms != null)
					{
						// loop over each one
						foreach (UnrealTargetPlatform Platform in Platforms)
						{
							// if it's a compiled platform, then add this group to be supported
							if (SupportedPlatforms.Contains(Platform))
							{
								SupportedGroups.Add(Group);
							}
						}
					}
				}

				// loop over groups one more time, anything NOT in SupportedGroups is now unsuppored, and should be added to the output list
				foreach (UnrealPlatformGroup Group in Enum.GetValues(typeof(UnrealPlatformGroup)))
				{
					if (SupportedGroups.Contains(Group) == false)
					{
						OtherPlatformNameStrings.Add(Group.ToString());
					}
				}

				foreach (UnrealTargetPlatform CurPlatform in Enum.GetValues(typeof(UnrealTargetPlatform)))
				{
					if (CurPlatform != UnrealTargetPlatform.Unknown)
					{
						bool ShouldConsider = true;

						// If we have a platform and a group with the same name, don't add the platform
						// to the other list if the same-named group is supported.  This is a lot of
						// lines because we need to do the comparisons as strings.
						string CurPlatformString = CurPlatform.ToString();
						foreach (UnrealPlatformGroup Group in Enum.GetValues(typeof(UnrealPlatformGroup)))
						{
							if (Group.ToString().Equals(CurPlatformString))
							{
								ShouldConsider = false;
								break;
							}
						}

						// Don't add our current platform to the list of platform sub-directory names that
						// we'll skip source files for
						if (ShouldConsider && !SupportedPlatforms.Contains(CurPlatform))
						{
							OtherPlatformNameStrings.Add(CurPlatform.ToString());
						}
					}
				}

				return OtherPlatformNameStrings;
			}
		}


		/// <summary>
		/// Takes a path string and makes all of the path separator characters consistent. Also removes unnecessary multiple separators.
		/// </summary>
		/// <param name="FilePath">File path with potentially inconsistent slashes</param>
		/// <param name="UseDirectorySeparatorChar">The directory separator to use</param>
		/// <returns>File path with consistent separators</returns>
		public static string CleanDirectorySeparators(string FilePath, char UseDirectorySeparatorChar = '\0')
		{
			StringBuilder CleanPath = null;
			if (UseDirectorySeparatorChar == '\0')
			{
				UseDirectorySeparatorChar = Environment.OSVersion.Platform == PlatformID.Unix ? '/' : '\\';
			}
			char PrevC = '\0';
			// Don't check for double separators until we run across a valid dir name. Paths that start with '//' or '\\' can still be valid.			
			bool bCanCheckDoubleSeparators = false;
			for (int Index = 0; Index < FilePath.Length; ++Index)
			{
				char C = FilePath[Index];
				if (C == '/' || C == '\\')
				{
					if (C != UseDirectorySeparatorChar)
					{
						C = UseDirectorySeparatorChar;
						if (CleanPath == null)
						{
							CleanPath = new StringBuilder(FilePath.Substring(0, Index), FilePath.Length);
						}
					}

					if (bCanCheckDoubleSeparators && C == PrevC)
					{
						if (CleanPath == null)
						{
							CleanPath = new StringBuilder(FilePath.Substring(0, Index), FilePath.Length);
						}
						continue;
					}
				}
				else
				{
					// First non-separator character, safe to check double separators
					bCanCheckDoubleSeparators = true;
				}

				if (CleanPath != null)
				{
					CleanPath.Append(C);
				}
				PrevC = C;
			}
			return CleanPath != null ? CleanPath.ToString() : FilePath;
		}


		/// <summary>
		/// Given a file path and a directory, returns a file path that is relative to the specified directory
		/// </summary>
		/// <param name="SourcePath">File path to convert</param>
		/// <param name="RelativeToDirectory">The directory that the source file path should be converted to be relative to.  If this path is not rooted, it will be assumed to be relative to the current working directory.</param>
		/// <param name="AlwaysTreatSourceAsDirectory">True if we should treat the source path like a directory even if it doesn't end with a path separator</param>
		/// <returns>Converted relative path</returns>
		public static string MakePathRelativeTo(string SourcePath, string RelativeToDirectory, bool AlwaysTreatSourceAsDirectory = false)
		{
			if (String.IsNullOrEmpty(RelativeToDirectory))
			{
				// Assume CWD
				RelativeToDirectory = ".";
			}

			string AbsolutePath = SourcePath;
			if (!Path.IsPathRooted(AbsolutePath))
			{
				AbsolutePath = Path.GetFullPath(SourcePath);
			}
			bool SourcePathEndsWithDirectorySeparator = AbsolutePath.EndsWith(Path.DirectorySeparatorChar.ToString()) || AbsolutePath.EndsWith(Path.AltDirectorySeparatorChar.ToString());
			if (AlwaysTreatSourceAsDirectory && !SourcePathEndsWithDirectorySeparator)
			{
				AbsolutePath += Path.DirectorySeparatorChar;
			}

			Uri AbsolutePathUri = new Uri(AbsolutePath);

			string AbsoluteRelativeDirectory = RelativeToDirectory;
			if (!Path.IsPathRooted(AbsoluteRelativeDirectory))
			{
				AbsoluteRelativeDirectory = Path.GetFullPath(AbsoluteRelativeDirectory);
			}

			// Make sure the directory has a trailing directory separator so that the relative directory that
			// MakeRelativeUri creates doesn't include our directory -- only the directories beneath it!
			if (!AbsoluteRelativeDirectory.EndsWith(Path.DirectorySeparatorChar.ToString()) && !AbsoluteRelativeDirectory.EndsWith(Path.AltDirectorySeparatorChar.ToString()))
			{
				AbsoluteRelativeDirectory += Path.DirectorySeparatorChar;
			}

			// Convert to URI form which is where we can make the relative conversion happen
			Uri AbsoluteRelativeDirectoryUri = new Uri(AbsoluteRelativeDirectory);

			// Ask the URI system to convert to a nicely formed relative path, then convert it back to a regular path string
			Uri UriRelativePath = AbsoluteRelativeDirectoryUri.MakeRelativeUri(AbsolutePathUri);
			string RelativePath = Uri.UnescapeDataString(UriRelativePath.ToString()).Replace('/', Path.DirectorySeparatorChar);

			// If we added a directory separator character earlier on, remove it now
			if (!SourcePathEndsWithDirectorySeparator && AlwaysTreatSourceAsDirectory && RelativePath.EndsWith(Path.DirectorySeparatorChar.ToString()))
			{
				RelativePath = RelativePath.Substring(0, RelativePath.Length - 1);
			}

			// Uri.MakeRelativeUri is broken in Mono 2.x and sometimes returns broken path
			if (IsRunningOnMono)
			{
				// Check if result is correct
				string TestPath = Path.GetFullPath(Path.Combine(AbsoluteRelativeDirectory, RelativePath));
				if (TestPath != AbsolutePath)
				{
					TestPath += "/";
					if (TestPath != AbsolutePath)
					{
						// Fix the path. @todo Mac: replace this hack with something better
						RelativePath = "../" + RelativePath;
					}
				}
			}

			return RelativePath;
		}


		/// <summary>
		/// Backspaces the specified number of characters, then displays a progress percentage value to the console
		/// </summary>
		/// <param name="Numerator">Progress numerator</param>
		/// <param name="Denominator">Progress denominator</param>
		/// <param name="NumCharsToBackspaceOver">Number of characters to backspace before writing the text.  This value will be updated with the length of the new progress string.  The first time progress is displayed, you should pass 0 for this value.</param>
		public static void DisplayProgress(int Numerator, int Denominator, ref int NumCharsToBackspaceOver)
		{
			// Backspace over previous progress value
			while (NumCharsToBackspaceOver-- > 0)
			{
				Console.Write("\b");
			}

			// Display updated progress string and keep track of how long it was
			float ProgressValue = Denominator > 0 ? ((float)Numerator / (float)Denominator) : 1.0f;
			string ProgressString = String.Format("{0}%", Math.Round(ProgressValue * 100.0f));
			NumCharsToBackspaceOver = ProgressString.Length;
			Console.Write(ProgressString);
		}


		/*
		 * Read and write classes with xml specifiers
		 */
		static private void UnknownAttributeDelegate(object sender, XmlAttributeEventArgs e)
		{
		}

		static private void UnknownNodeDelegate(object sender, XmlNodeEventArgs e)
		{
		}

		/// <summary>
		/// Reads a class using XML serialization
		/// </summary>
		/// <typeparam name="T">The type to read</typeparam>
		/// <param name="FileName">The XML file to read from</param>
		/// <returns>New deserialized instance of type T</returns>
		static public T ReadClass<T>(string FileName) where T : new()
		{
			T Instance = new T();
			StreamReader XmlStream = null;
			try
			{
				// Get the XML data stream to read from
				XmlStream = new StreamReader(FileName);

				// Creates an instance of the XmlSerializer class so we can read the settings object
				XmlSerializer Serialiser = new XmlSerializer(typeof(T));
				// Add our callbacks for unknown nodes and attributes
				Serialiser.UnknownNode += new XmlNodeEventHandler(UnknownNodeDelegate);
				Serialiser.UnknownAttribute += new XmlAttributeEventHandler(UnknownAttributeDelegate);

				// Create an object graph from the XML data
				Instance = (T)Serialiser.Deserialize(XmlStream);
			}
			catch (Exception E)
			{
				Log.TraceInformation(E.Message);
			}
			finally
			{
				if (XmlStream != null)
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}

			return Instance;
		}

		/// <summary>
		/// Serialize an object to an XML file
		/// </summary>
		/// <typeparam name="T">Type of the object to serialize</typeparam>
		/// <param name="Data">Object to write</param>
		/// <param name="FileName">File to write to</param>
		/// <param name="DefaultNameSpace">Default namespace for the output elements</param>
		/// <returns>True if the file was written successfully</returns>
		static public bool WriteClass<T>(T Data, string FileName, string DefaultNameSpace)
		{
			bool bSuccess = true;
			StreamWriter XmlStream = null;
			try
			{
				FileInfo Info = new FileInfo(FileName);
				if (Info.Exists)
				{
					Info.IsReadOnly = false;
				}

				// Make sure the output directory exists
				Directory.CreateDirectory(Path.GetDirectoryName(FileName));

				XmlSerializerNamespaces EmptyNameSpace = new XmlSerializerNamespaces();
				EmptyNameSpace.Add("", DefaultNameSpace);

				XmlStream = new StreamWriter(FileName, false, Encoding.Unicode);
				XmlSerializer Serialiser = new XmlSerializer(typeof(T));

				// Add our callbacks for unknown nodes and attributes
				Serialiser.UnknownNode += new XmlNodeEventHandler(UnknownNodeDelegate);
				Serialiser.UnknownAttribute += new XmlAttributeEventHandler(UnknownAttributeDelegate);

				Serialiser.Serialize(XmlStream, Data, EmptyNameSpace);
			}
			catch (Exception E)
			{
				Log.TraceInformation(E.Message);
				bSuccess = false;
			}
			finally
			{
				if (XmlStream != null)
				{
					// Done with the file so close it
					XmlStream.Close();
				}
			}

			return (bSuccess);
		}

		/// <summary>
		/// Returns true if the specified Process has been created, started and remains valid (i.e. running).
		/// </summary>
		/// <param name="p">Process object to test</param>
		/// <returns>True if valid, false otherwise.</returns>
		public static bool IsValidProcess(Process p)
		{
			// null objects are always invalid
			if (p == null)
				return false;
			// due to multithreading on Windows, lock the object
			lock (p)
			{
				// Mono has a specific requirement if testing for an alive process
				if (IsRunningOnMono)
					return p.Handle != IntPtr.Zero; // native handle to the process
				// on Windows, simply test the process ID to be non-zero. 
				// note that this can fail and have a race condition in threads, but the framework throws an exception when this occurs.
				try
				{
					return p.Id != 0;
				}
				catch { } // all exceptions can be safely caught and ignored, meaning the process is not started or has stopped.
			}
			return false;
		}

		/// <summary>
		/// Removes multi-dot extensions from a filename (i.e. *.automation.csproj)
		/// </summary>
		/// <param name="Filename">Filename to remove the extensions from</param>
		/// <returns>Clean filename.</returns>
		public static string GetFilenameWithoutAnyExtensions(string Filename)
		{
			Filename = Path.GetFileName(Filename);

			int DotIndex = Filename.IndexOf('.');
			if (DotIndex == -1)
			{
				return Filename; // No need to copy string
			}
			else
			{
				return Filename.Substring(0, DotIndex);
			}
		}

		/// <summary>
		/// Returns Filename with path but without extension.
		/// </summary>
		/// <param name="Filename">Filename</param>
		/// <returns>Path to the file with its extension removed.</returns>
		public static string GetPathWithoutExtension(string Filename)
		{
			if (!String.IsNullOrEmpty(Path.GetExtension(Filename)))
			{
				return Path.Combine(Path.GetDirectoryName(Filename), Path.GetFileNameWithoutExtension(Filename));
			}
			else
			{
				return Filename;
			}
		}


		/// <summary>
		/// Returns true if the specified file's path is located under the specified directory, or any of that directory's sub-folders.  Does not care whether the file or directory exist or not.  This is a simple string-based check.
		/// </summary>
		/// <param name="FilePath">The path to the file</param>
		/// <param name="Directory">The directory to check to see if the file is located under (or any of this directory's subfolders)</param>
		/// <returns></returns>
		public static bool IsFileUnderDirectory(string FilePath, string Directory)
		{
			string DirectoryPathPlusSeparator = Path.GetFullPath(Directory);
			if (!DirectoryPathPlusSeparator.EndsWith(Path.DirectorySeparatorChar.ToString()))
			{
				DirectoryPathPlusSeparator += Path.DirectorySeparatorChar;
			}
			return Path.GetFullPath(FilePath).StartsWith(DirectoryPathPlusSeparator, StringComparison.InvariantCultureIgnoreCase);
		}

		/// <summary>
		/// Checks if given type implements given interface.
		/// </summary>
		/// <typeparam name="InterfaceType">Interface to check.</typeparam>
		/// <param name="TestType">Type to check.</param>
		/// <returns>True if TestType implements InterfaceType. False otherwise.</returns>
		public static bool ImplementsInterface<InterfaceType>(Type TestType)
		{
			return Array.IndexOf(TestType.GetInterfaces(), typeof(InterfaceType)) != -1;
		}

		/// <summary>
		/// Returns the User Settings Directory path. This matches FPlatformProcess::UserSettingsDir().
		/// NOTE: This function may return null. Some accounts (eg. the SYSTEM account on Windows) do not have a personal folder, and Jenkins
		/// runs using this account by default.
		/// </summary>
		public static DirectoryReference GetUserSettingDirectory()
		{
			if (BuildHostPlatform.Current.Platform == UnrealTargetPlatform.Mac)
			{
				return new DirectoryReference(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Personal), "Library", "Application Support", "Epic"));
			}
			else if (Environment.OSVersion.Platform == PlatformID.Unix)
			{
				return new DirectoryReference(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "Epic"));
			}
			else
			{
				// Not all user accounts have a local application data directory (eg. SYSTEM, used by Jenkins for builds).
				string DirectoryName = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
				if(String.IsNullOrEmpty(DirectoryName))
				{
					return null;
				}
				else
				{
					return new DirectoryReference(DirectoryName);
				}
			}
		}

		/// <summary>
		/// Gets the number of physical cores, excluding hyper threading.
		/// </summary>
		/// <returns>The number of physical cores, or -1 if it could not be obtained</returns>
		public static int GetPhysicalProcessorCount()
		{
			// Can't use WMI queries on Mono; just fail.
			if (Utils.IsRunningOnMono)
			{
				return -1;
			}

			// On some systems this requires a hot fix to work so catch any exceptions
			try
			{
				int NumCores = 0;
				using (ManagementObjectSearcher Mos = new System.Management.ManagementObjectSearcher("Select * from Win32_Processor"))
				{
					ManagementObjectCollection MosCollection = Mos.Get();
					foreach (ManagementBaseObject Item in MosCollection)
					{
						NumCores += int.Parse(Item["NumberOfCores"].ToString());
					}
				}
				return NumCores;
			}
			catch (Exception Ex)
			{
				Log.TraceWarning("Unable to get the number of Cores: {0}", Ex.ToString());
				Log.TraceWarning("Falling back to processor count.");
				return -1;
			}
		}
	}
}
