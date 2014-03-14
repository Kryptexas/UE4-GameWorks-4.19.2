// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;

[Help("Generates a Derived Data Cache for QAGame OrionGame FortniteGame PlatformerGame ShooterGame StrategyGame")]
[Help("DCC", "Sets additional params for the run command")]
class FillDerivedDataCache : BuildCommand
{
	#region Fields
	#endregion
    bool FailureOccurred;
    List<string> FailedProjects = new List<string>();
	private void FillDDC(string ProjectName)
	{
		Log("Running UE4Editor to generate DDC data for project {0}", ProjectName);
		var EditorExe = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/UE4Editor-Cmd.exe");
        string DDCArg = ParseParamValue("DDC", "");
        if (!String.IsNullOrEmpty(DDCArg))
        {
            DDCArg = "-DDC=" + DDCArg;
        }
		var RunResult = Run(EditorExe, String.Format( "{0} -run=DerivedDataCache -TargetPlatform=Windows+WindowsNoEditor -fill -unattended -buildmachine -forcelogflush {1}", ProjectName, DDCArg) );

		// Save off the log file.  We expect the file to exist.
        if (FileExists(CombinePaths(CmdEnv.LocalRoot, ProjectName, @"Saved\Logs", ProjectName + ".log")))
        {
            CopyFile(CombinePaths(CmdEnv.LocalRoot, ProjectName, @"Saved\Logs", ProjectName + ".log"), CombinePaths(CmdEnv.LogFolder, ProjectName + "_GenerateDDC.log"));
        }
        else
        {
            CopyFile(CombinePaths(CmdEnv.LocalRoot, @"Samples\SampleGames", ProjectName, @"Saved\Logs", ProjectName + ".log"), CombinePaths(CmdEnv.LogFolder, ProjectName + "_GenerateDDC.log"));
        }

		if (RunResult.ExitCode != 0)
		{
            FailureOccurred = true;
            FailedProjects.Add(ProjectName);
		}
	}


	public override void ExecuteBuild()
	{
		Log("************************* Fill DDCs");

		if (P4Enabled)
		{
			Log("Fill DDCs from label {0}", P4Env.LabelToSync);
		}

        var Game = ParseParamValue("Game");
        if (String.IsNullOrEmpty(Game))
        {
            var GameTargets = new string[] 
				{
					"QAGame",
                    "FortniteGame",
					"OrionGame",					
					"PlatformerGame",
					"ShooterGame",
					"StrategyGame",					
				};

            foreach (var Target in GameTargets)
            {
                Log("Fill DDCs for {0}", Target);
                FillDDC(Target);
            }
        }
        else
        {
            List<string> Games = new List<string>(Game.Split('+'));

            foreach (var Target in Games)
            {
                Log("Fill DDCs for {0}", Game);
                FillDDC(Game);
            }
        }

        if (FailureOccurred == true)
        {
            foreach (string ProjectName in FailedProjects)
            {
                Log("BUILD FAILED: Failed while generating DDC data for {0}", ProjectName);                
            }
            throw new AutomationException("BUILD FAILED: Failed generating DDC data");
        }
	}
}
