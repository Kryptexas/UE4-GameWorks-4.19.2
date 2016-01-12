// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;
using System.Linq;
using System.Net.Mail;
using AutomationTool;
using UnrealBuildTool;

/// <summary>
/// Helper command used for rebuilding a projects light maps.
/// </summary>
/// <remarks>
/// Command line parameters used by this command:
/// -project					- Absolute path to a .uproject file
/// -MapsToRebuildLightMaps		- A list of '+' delimited maps we wish to build lightmaps for.
/// -CommandletTargetName		- The Target used in running the commandlet
/// -StakeholdersEmailAddresses	- Users to notify of completion
/// 
/// </remarks>
namespace AutomationScripts.Automation
{
	[RequireP4]
	public class RebuildLightMaps : BuildCommand
	{
		// The rebuild lighting process
		#region RebuildLightMaps Command

		public override void ExecuteBuild()
		{
			Log("********** REBUILD LIGHT MAPS COMMAND STARTED **********");

			try
			{
				var Params = new ProjectParams
				(
					Command: this,
					// Shared
					RawProjectPath: ProjectPath
				);

				// Sync and build our targets required for the commandlet to run correctly.
				P4.Sync(String.Format("{0}/...#head", P4Env.BuildRootP4));

				BuildNecessaryTargets();
				CheckOutMaps(Params);
				RunRebuildLightmapsCommandlet(Params);
				SubmitRebuiltMaps();
			}
			catch (Exception ProcessEx)
			{
				Log("********** REBUILD LIGHT MAPS COMMAND FAILED **********");
				HandleFailure(ProcessEx.Message);
				throw ProcessEx;
			}

			// The processes steps have completed successfully.
			HandleSuccess();

			Log("********** REBUILD LIGHT MAPS COMMAND COMPLETED **********");
		}

		#endregion

		// Broken down steps used to run the process.
		#region RebuildLightMaps Process Steps

		private void BuildNecessaryTargets()
		{
			Log("Running Step:- RebuildLightMaps::BuildNecessaryTargets");
			UE4Build.BuildAgenda Agenda = new UE4Build.BuildAgenda();
			Agenda.AddTarget("ShaderCompileWorker", UnrealBuildTool.UnrealTargetPlatform.Win64, UnrealBuildTool.UnrealTargetConfiguration.Development);
			Agenda.AddTarget("UnrealLightmass", UnrealBuildTool.UnrealTargetPlatform.Win64, UnrealBuildTool.UnrealTargetConfiguration.Development);
			Agenda.AddTarget(CommandletTargetName, UnrealBuildTool.UnrealTargetPlatform.Win64, UnrealBuildTool.UnrealTargetConfiguration.Development);

			try
			{
				UE4Build Builder = new UE4Build(this);
				Builder.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true, InForceNoXGE: false);
				UE4Build.CheckBuildProducts(Builder.BuildProductFiles);
			}
			catch (AutomationException Ex)
			{
				LogError("Rebuild Light Maps has failed.");
				throw Ex;
			}
		}

		private void CheckOutMaps(ProjectParams Params)
		{
			Log("Running Step:- RebuildLightMaps::CheckOutMaps");
			// Setup a P4 Cl we will use to submit the new lightmaps
			WorkingCL = P4.CreateChange(P4Env.Client, String.Format("{0} rebuilding lightmaps from changelist {1}", Params.ShortProjectName, P4Env.Changelist));
			Log("Working in {0}", WorkingCL);

			string AllMapsWildcardCmdline = String.Format("{0}\\...\\*.umap", Params.RawProjectPath.Directory);
			string Output = "";
			P4.P4Output(out Output, String.Format("edit -c {0} {1}", WorkingCL, AllMapsWildcardCmdline));

			// We need to ensure that any error in the output log is observed.
			// P4 is still successful if it manages to run the operation.
			if (FoundCheckOutErrorInP4Output(Output) == true)
			{
				LogError("Failed to check out every one of the project maps.");
				throw new AutomationException("Failed to check out every one of the project maps.");
			}
		}

		private void RunRebuildLightmapsCommandlet(ProjectParams Params)
		{
			Log("Running Step:- RebuildLightMaps::RunRebuildLightmapsCommandlet");

			// Find the commandlet binary
			string UE4EditorExe = HostPlatform.Current.GetUE4ExePath(Params.UE4Exe);
			if (!FileExists(UE4EditorExe))
			{
				LogError("Missing " + UE4EditorExe + " executable. Needs to be built first.");
				throw new AutomationException("Missing " + UE4EditorExe + " executable. Needs to be built first.");
			}

			// Now let's rebuild lightmaps for the project
			try
			{
				var CommandletParams = IsBuildMachine ? "-unattended -buildmachine -fileopenlog" : "-fileopenlog";
				RebuildLightMapsCommandlet(Params.RawProjectPath, Params.UE4Exe, Params.MapsToRebuildLightMaps.ToArray(), CommandletParams);
			}
			catch (Exception Ex)
			{
				// Something went wrong with the commandlet. Abandon this run, don't check in any updated files, etc.
				LogError("Rebuild Light Maps has failed.");
				throw new AutomationException(ExitCode.Error_Unknown, Ex, "RebuildLightMaps failed.");
			}
		}

		private void SubmitRebuiltMaps()
		{
			Log("Running Step:- RebuildLightMaps::SubmitRebuiltMaps");

			// Check everything in!
			if (WorkingCL != -1)
			{
				int SubmittedCL;
				P4.Submit(WorkingCL, out SubmittedCL, true, true);
				Log("New lightmaps have been submitted in changelist {0}", SubmittedCL);
			}
		}

		#endregion

		// Helper functions and procedure steps necessary for running the commandlet successfully
		#region RebuildLightMaps Helper Functions

		/**
		 * Parse the P4 output for any errors that we really care about.
		 * e.g. umaps and assets are exclusive checkout files, if we cant check out a map for this reason
		 *		then we need to stop.
		 */
		private bool FoundCheckOutErrorInP4Output(string Output)
		{
			bool bHadAnError = false;

			var Lines = Output.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
			foreach (string Line in Lines)
			{
				// Check for log spew that matches exclusive checkout failure
				// http://answers.perforce.com/articles/KB/3114
				if (Line.Contains("can't edit exclusive file already opened"))
				{
					bHadAnError = true;
					break;
				}
			}

			return bHadAnError;
		}

		/**
		 * Cleanup anything this build may leave behind and inform the user
		 */
		private void HandleFailure(String FailureMessage)
		{
			try
			{
				if (WorkingCL != -1)
				{
					P4.RevertAll(WorkingCL);
					P4.DeleteChange(WorkingCL);
				}
				SendCompletionMessage(false, FailureMessage);
			}
			catch (P4Exception P4Ex)
			{
				LogError("Failed to clean up P4 changelist: " + P4Ex.Message);
			}
			catch (Exception SendMailEx)
			{
				LogError("Failed to notify that build succeeded: " + SendMailEx.Message);
			}
		}

		/**
		 * Perform any post completion steps needed. I.e. Notify stakeholders etc.
		 */
		private void HandleSuccess()
		{
			try
			{
				SendCompletionMessage(true, "Successfully rebuilt lightmaps.");
			}
			catch (Exception SendMailEx)
			{
				LogError("Failed to notify that build succeeded: " + SendMailEx.Message);
			}
		}

		/**
		 * Notify stakeholders of the commandlet results
		 */
		void SendCompletionMessage(bool bWasSuccessful, String MessageBody)
		{
			MailMessage Message = new System.Net.Mail.MailMessage();
			Message.Priority = MailPriority.High;
			Message.From = new MailAddress("unrealbot@epicgames.com");
			foreach (String NextStakeHolder in StakeholdersEmailAddresses)
			{
				Message.To.Add(new MailAddress(NextStakeHolder));
			}

			Message.CC.Add(new MailAddress("Terence.Burns@epicgames.com"));
			Message.Subject = String.Format("Nightly lightmap rebuild ", bWasSuccessful ? "[SUCCESS]" : "[FAILED]");
			Message.Body = MessageBody;

			try
			{
				SmtpClient MailClient = new SmtpClient("smtp.epicgames.net");
				MailClient.Send(Message);
			}
			catch (Exception Ex)
			{
				LogError("Failed to send notify email to {0} ({1})", String.Join(", ", StakeholdersEmailAddresses.ToArray()), Ex.Message);
			}
		}

		#endregion

		// Member vars used in multiple steps.
		#region RebuildLightMaps Property Set-up

		// Users to notify if the process fails or succeeds.
		List<String> StakeholdersEmailAddresses
		{
			get
			{
				String UnprocessedEmailList = ParseParamValue("StakeholdersEmailAddresses");
				if (String.IsNullOrEmpty(UnprocessedEmailList) == false)
				{
					return UnprocessedEmailList.Split('+').ToList();
				}
				else
				{
					return null;
				}
			}
		}

		// The Changelist used when doing the work.
		private int WorkingCL = -1;

		// The target name of the commandlet binary we wish to build and run.
		private String CommandletTargetName
		{
			get
			{
				return ParseParamValue("CommandletTargetName", "");
			}
		}

		// Process command-line and find a project file. This is necessary for the commandlet to run successfully
		private FileReference ProjectFullPath;
		public virtual FileReference ProjectPath
		{
			get
			{
				if (ProjectFullPath == null)
				{
					var OriginalProjectName = ParseParamValue("project", "");
					var ProjectName = OriginalProjectName;
					ProjectName = ProjectName.Trim(new char[] { '\"' });
					if (ProjectName.IndexOfAny(new char[] { '\\', '/' }) < 0)
					{
						ProjectName = CombinePaths(CmdEnv.LocalRoot, ProjectName, ProjectName + ".uproject");
					}
					else if (!FileExists_NoExceptions(ProjectName))
					{
						ProjectName = CombinePaths(CmdEnv.LocalRoot, ProjectName);
					}
					if (FileExists_NoExceptions(ProjectName))
					{
						ProjectFullPath = new FileReference(ProjectName);
					}
					else
					{
						var Branch = new BranchInfo(new List<UnrealTargetPlatform> { UnrealBuildTool.BuildHostPlatform.Current.Platform });
						var GameProj = Branch.FindGame(OriginalProjectName);
						if (GameProj != null)
						{
							ProjectFullPath = GameProj.FilePath;
						}
						if (!FileExists_NoExceptions(ProjectFullPath.FullName))
						{
							throw new AutomationException("Could not find a project file {0}.", ProjectName);
						}
					}
				}
				return ProjectFullPath;
			}
		}

		#endregion
	}
}