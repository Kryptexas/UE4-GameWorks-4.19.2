using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace UnrealBuildTool
{
	public static class EngineConfiguration
	{
		/// <summary>
		/// Reads an array from the config files, matching a given section and key.
		/// </summary>
		public static void ReadArray(string ProjectDirectory, UnrealTargetPlatform Platform, string BaseIniName, string FindSection, string FindKey, List<string> Values)
		{
			foreach (string IniFileName in EnumerateIniFileNames(ProjectDirectory, Platform, BaseIniName))
			{
				if (File.Exists(IniFileName))
				{
					bool bInCorrectSection = false;
					foreach(string Line in File.ReadAllLines(IniFileName))
					{
						// Skip the whitespace at the start of the line
						int LineMin = 0;
						while(LineMin < Line.Length && Char.IsWhiteSpace(Line[LineMin])) LineMin++;

						// Skip the whitespace at the end of the line
						int LineMax = Line.Length;
						while (LineMax > LineMin && Char.IsWhiteSpace(Line[LineMax - 1])) LineMax--;

						// Check it's not a comment
						if (LineMax > LineMin && Line[LineMin] != ';')
						{
							if (Line[LineMin] == '[' && Line[LineMax - 1] == ']')
							{
								// This is a section heading. Check if it's the section we're looking for.
								bInCorrectSection = (FindSection.Length == LineMax - LineMin - 2 && String.Compare(Line, LineMin + 1, FindSection, 0, LineMax - LineMin - 2) == 0);
							}
							else if (bInCorrectSection)
							{
								// Split the line into key/value.
								TryParseArrayKeyValue(Line, LineMin, LineMax, FindKey, Values);
							}
						}
					}
				}
			}
		}

		/// <summary>
		/// Returns a list of INI filenames for the given project
		/// </summary>
		private static IEnumerable<string> EnumerateIniFileNames(string ProjectDirectory, UnrealTargetPlatform Platform, string BaseIniName)
		{
			string PlatformName = GetIniPlatformName(Platform);

			// Engine/Config/Base.ini (included in every ini type, required)
			yield return Path.Combine(BuildConfiguration.RelativeEnginePath, "Config", "Base.ini");

			// Engine/Config/Base* ini
			yield return Path.Combine(BuildConfiguration.RelativeEnginePath, "Config", "Base" +  BaseIniName + ".ini");

			// Game/Config/Default* ini
			yield return Path.Combine(ProjectDirectory, "Config", "Default" + BaseIniName + ".ini");

			// Engine/Config/Platform/Platform* ini
			yield return Path.Combine(BuildConfiguration.RelativeEnginePath, "Config", PlatformName, PlatformName + BaseIniName + ".ini");
	
			// Game/Config/Platform/Platform* ini
			yield return Path.Combine(ProjectDirectory, "Config", PlatformName, PlatformName + BaseIniName + ".ini");
		}

		/// <summary>
		/// Returns the platform name to use as part of platform-specific config files
		/// </summary>
		private static string GetIniPlatformName(UnrealTargetPlatform TargetPlatform)
		{
			if(TargetPlatform == UnrealTargetPlatform.Win32 || TargetPlatform == UnrealTargetPlatform.Win64)
			{
				return "Windows";
			}
			else
			{
				return Enum.GetName(typeof(UnrealTargetPlatform), TargetPlatform);
			}
		}

		/// <summary>
		/// Try to match a config file key, and adjust an array of values to reflect it.
		/// </summary>
		private static void TryParseArrayKeyValue(string Line, int LineMin, int LineMax, string FindKey, List<string> Values)
		{
			int EqualsIdx = Line.IndexOf('=', LineMin, LineMax - LineMin);
			if (EqualsIdx != -1)
			{
				// Skip the + or - at the start of the key name
				int KeyMin = LineMin;
				if (Line[KeyMin] == '+' || Line[KeyMin] == '-')
				{
					KeyMin++;
				}

				// Skip the whitespace between the key name and equals sign
				int KeyMax = EqualsIdx;
				while (KeyMax > KeyMin && Char.IsWhiteSpace(Line[KeyMax - 1]))
				{
					KeyMax--;
				}

				// Check if it matches the key we're looking for
				int KeyLength = KeyMax - KeyMin;
				if (FindKey.Length == KeyLength && String.Compare(Line, KeyMin, FindKey, 0, KeyLength, true) == 0)
				{
					// Get the start and end of the value
					int ValueMin = EqualsIdx + 1;
					while (ValueMin < LineMax && Char.IsWhiteSpace(Line[ValueMin]))
					{
						ValueMin++;
					}
					int ValueMax = LineMax;
					if (ValueMax > ValueMin + 1 && Line[ValueMin] == '\"' && Line[ValueMax - 1] == '\"')
					{
						ValueMin++;
						ValueMax--;
					}

					// Add or remove it from the list
					string Value = Line.Substring(ValueMin, ValueMax - ValueMin);
					if (Line.StartsWith("-"))
					{
						Values.RemoveAll(x => String.Compare(x, Value, true) == 0);
					}
					else
					{
						Values.Add(Value);
					}
				}
			}
		}
	}
}
