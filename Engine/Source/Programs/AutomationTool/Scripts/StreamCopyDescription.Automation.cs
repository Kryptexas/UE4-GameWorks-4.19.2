// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
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
		/// Execute the command
		/// </summary>
		public override void ExecuteBuild()
		{
			// Get the source and target streams
			string Stream = ParseParamValue("Stream", P4Env.BuildRootP4);
			string Changes = ParseParamValue("Changes", null);

			// Get changes which haven't been copied up from the current stream
			string Descriptions;
			if (Changes == null)
			{
				ProcessResult Result = P4.P4(String.Format("interchanges -l -S {0}", Stream), AllowSpew: false);
				Descriptions = Result.Output.Replace("\r\n", "\n");
			}
			else
			{
				ProcessResult Result = P4.P4(String.Format("changes -l {0}", Changes), AllowSpew: false);
				Descriptions = Result.Output.Replace("\r\n", "\n");
			}

			// Clean any workspace names that may reveal internal information
			Descriptions = Regex.Replace(Descriptions, "(Change[^@]*)@.*", "$1", RegexOptions.Multiline);

			// Remove changes by the build machine
			Descriptions = Regex.Replace(Descriptions, "[^\n]*buildmachine\n(\n|\t[^\n]*\n)*", "");

			// Get the last submitted change in the source stream
			List<P4Connection.ChangeRecord> ChangeRecords;
			if (!P4.Changes(out ChangeRecords, String.Format("-m1 {0}/...", Stream), AllowSpew: false))
			{
				throw new AutomationException("Couldn't get changes for this branch");
			}

			// Figure out the target stream
			ProcessResult StreamResult = P4.P4(String.Format("stream -o {0}", Stream), AllowSpew: false);
			if (StreamResult.ExitCode != 0)
			{
				throw new AutomationException("Couldn't get stream description for {0}", Stream);
			}
			string Target = P4Spec.FromString(StreamResult.Output).GetField("Parent");
			if(Target == null)
			{
				throw new AutomationException("Couldn't get parent stream for {0}", Stream);
			}

			// Write the output file
			string OutputDirName = Path.Combine(CommandUtils.CmdEnv.LocalRoot, "Engine", "Intermediate");
			CommandUtils.CreateDirectory(OutputDirName);
			string OutputFileName = Path.Combine(OutputDirName, "Changes.txt");
			using (StreamWriter Writer = new StreamWriter(OutputFileName))
			{
				Writer.WriteLine("Copying {0} to {1} (Source: {0} @ {2})", Stream, Target.Trim(), ChangeRecords[0].CL);
				Writer.WriteLine();
				Writer.WriteLine("==========================");
				Writer.WriteLine("MAJOR FEATURES + CHANGES");
				Writer.WriteLine("==========================");
				Writer.WriteLine();

				foreach (string Line in Descriptions.Split('\n'))
				{
					string TrimLine = Line.TrimStart();
					if(!TrimLine.StartsWith("#codereview") && !TrimLine.StartsWith("#rb") && !TrimLine.StartsWith("#lockdown"))
					{
						Writer.WriteLine(Line);
					}
				}
			}
			Log("Written {0}.", OutputFileName);

			// Open it with the default text editor
			Process.Start(OutputFileName);
		}
	}
}
