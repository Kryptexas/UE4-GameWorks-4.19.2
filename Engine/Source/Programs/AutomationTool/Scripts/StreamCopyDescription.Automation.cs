// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Reflection;
using System.Linq;
using AutomationTool;
using UnrealBuildTool;

namespace AutomationTool
{
	/// <summary>
	/// Generates a changelist description for copying a stream to its parent
	/// </summary>
	[RequireP4]
	public class StreamCopyDescription : BuildCommand
	{
		/// <summary>
		/// Hunts down the list of confidential platforms, based on ConfidentialPlatform.ini in the Engine/Config directory (this way we don't 
		/// need to hardcode the platform names!)
		/// </summary>
		/// <returns></returns>
		private string[] GetPlatformNames()
		{
			List<string> Platforms = new List<string>();
			
			// two special platform names (for non-special, and combined CLs that affect multiple confidential platforms)
			Platforms.Add("");
			Platforms.Add("Multi");

			foreach (string Dir in Directory.EnumerateDirectories(Path.Combine(EngineDirectory.FullName, "Config")))
			{
				if (File.Exists(Path.Combine(Dir, "ConfidentialPlatform.ini")))
				{
					Platforms.Add(Path.GetFileName(Dir));
				}
			}

			return Platforms.ToArray();
		}

		/// <summary>
		/// Execute the command
		/// </summary>
		public override void ExecuteBuild()
		{
			// Get the source and target streams
			string Stream = ParseParamValue("Stream", P4Env.BuildRootP4);
			string Changes = ParseParamValue("Changes", null);
			string Lockdown = ParseParamValue("Lockdown", "Nick.Penwarden");
			bool bPlatformSplit = !ParseParam("CombinePlatforms");

			// Get changes which haven't been copied up from the current stream
			string AllDescriptions;
			string SourceStream;
			string VerbosityFlag = bPlatformSplit ? "" : "-l";
			int LastCl;

			string[] Platforms = GetPlatformNames();
			StringBuilder[] DescriptionBuilders = new StringBuilder[Platforms.Length];

			for (int Index = 0; Index < Platforms.Length; Index++)
			{
				DescriptionBuilders[Index] = new StringBuilder();
			}

			if (Changes == null)
			{
				IProcessResult Result = P4.P4(String.Format("interchanges {0} -S {1}", VerbosityFlag, Stream), AllowSpew: false);
				AllDescriptions = Result.Output;
				SourceStream = Stream;

				// Get the last submitted change in the source stream
				List<P4Connection.ChangeRecord> ChangeRecords;
				if (!P4.Changes(out ChangeRecords, String.Format("-m1 {0}/...", SourceStream), AllowSpew: false))
				{
					throw new AutomationException("Couldn't get changes for this branch");
				}
				LastCl = ChangeRecords[0].CL;
			}
			else
			{
				IProcessResult Result = P4.P4(String.Format("changes {0} {1}", VerbosityFlag, Changes), AllowSpew: false);
				AllDescriptions = Result.Output;
				SourceStream = Regex.Replace(Changes, @"(\/(?:\/[^\/]*){2}).*", "$1");
				LastCl = Int32.Parse(Regex.Replace(Changes, ".*,", ""));
			}

			if (bPlatformSplit)
			{
				string[] Lines = AllDescriptions.Split("\n".ToCharArray());
				foreach (string Line in Lines)
				{
					// @todo replace with regexes!!
					string[] Tokens = Line.Split(" ".ToCharArray());
					if (Tokens.Length > 2 && Tokens[0] == "Change")
					{
						IProcessResult Result = P4.P4(String.Format("describe -s {0}", Tokens[1]), AllowSpew: false);

						// Affected files ... is the splitting point
						int AffectedFilesPos = Result.Output.IndexOf("Affected files ...");
						string Description = Result.Output.Substring(0, AffectedFilesPos);
						string Files = Result.Output.Substring(AffectedFilesPos);

						// look for the NDA platforms in the list of files (skipping over the "no" and "multi" platforms
						int WhichPlatform = 0;
						for (int Index = 2; Index < Platforms.Length; Index++)
						{
							// we search by directory in the form of /Platform/
							if (Files.Contains("/" + Platforms[Index] + "/"))
							{
								// if we contained multiple files, then we put into the Multi file, and someone will have to manually deal with it!!
								if (WhichPlatform == 0)
								{
									WhichPlatform = Index;
								}
								else
								{
									WhichPlatform = 1;
								}
							}
						}

						// add this description to the proper platform
						DescriptionBuilders[WhichPlatform].AppendLine(Description);
					}
				}
			}
			else
			{
				DescriptionBuilders[0].Append(AllDescriptions);
			}


			for (int PlatformIndex = 0; PlatformIndex < Platforms.Length; PlatformIndex++)
			{
				string Desc = DescriptionBuilders[PlatformIndex].ToString().Replace("\r\n", "\n");

				// Clean any workspace names that may reveal internal information
				Desc = Regex.Replace(Desc, "(Change[^@]*)@.*", "$1", RegexOptions.Multiline);

				// Remove changes by the build machine
				Desc = Regex.Replace(Desc, "[^\n]*buildmachine\n(\n|\t[^\n]*\n)*", "");

				// Remove all the tags we don't care about
				Desc = Regex.Replace(Desc, "^[ \t]*#(rb|fyi|codereview|lockdown)\\s.*$", "", RegexOptions.Multiline);

				// Empty out lines which just contain whitespace
				Desc = Regex.Replace(Desc, "^[ \t]+$", "", RegexOptions.Multiline);

				// Remove multiple consecutive blank lines
				Desc = Regex.Replace(Desc, "\n\n+", "\n\n");

				// Only include one newline at the end of each description
				Desc = Regex.Replace(Desc, "\n+Change", "\n\nChange");

				// Remove merge-only changelists
				Desc = Regex.Replace(Desc, "(?<=(^|\\n))Change .*\\s*Merging .* to .*\\s*\\n(?=(Change|$))", "");

				if (string.IsNullOrEmpty(Desc))
				{
					continue;
				}

				// Figure out the target stream
				IProcessResult StreamResult = P4.P4(String.Format("stream -o {0}", Stream), AllowSpew: false);
				if (StreamResult.ExitCode != 0)
				{
					throw new AutomationException("Couldn't get stream description for {0}", Stream);
				}
				string Target = P4Spec.FromString(StreamResult.Output).GetField("Parent");
				if (Target == null)
				{
					throw new AutomationException("Couldn't get parent stream for {0}", Stream);
				}

				// Write the output file
				string OutputDirName = Path.Combine(CommandUtils.CmdEnv.LocalRoot, "Engine", "Intermediate");
				CommandUtils.CreateDirectory(OutputDirName);
				string OutputFileName = Path.Combine(OutputDirName, string.Format("Changes{0}.txt", Platforms[PlatformIndex]));
				using (StreamWriter Writer = new StreamWriter(OutputFileName))
				{
					if (PlatformIndex == 1)
					{
						Writer.WriteLine("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
						Writer.WriteLine("CHANGES WITH MULTIPLE PLATFORMS!!! YOU MUST COPY THESE INTO THE OTHER ONES AS MAKES SENSE!!");
						Writer.WriteLine("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
						Writer.WriteLine();
						Writer.WriteLine();
					}
					else
					{
						Writer.WriteLine("Copying {0} to {1} (Source: {2} @ {3})", Stream, Target.Trim(), SourceStream, LastCl);
						Writer.WriteLine("#lockdown {0}", Lockdown);
						Writer.WriteLine();
						Writer.WriteLine("=====================================");
						Writer.WriteLine("{0} MAJOR FEATURES + CHANGES", Platforms[PlatformIndex]);
						Writer.WriteLine("=====================================");
						Writer.WriteLine();
					}

					foreach (string Line in Desc.Split('\n'))
					{
						Writer.WriteLine(Line);
					}

					Writer.WriteLine("DONE!");

				}
				Log("Written {0}.", OutputFileName);

				// Open it with the default text editor
				Process.Start(OutputFileName);
			}
		}
	}
}
