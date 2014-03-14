// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;

class Localise : BuildCommand
{
    public override void ExecuteBuild()
    {
        var EditorExe = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/UE4Editor-Cmd.exe");

        if (P4Enabled)
        {
            Log("Sync necessary content to head revision");
            Sync(P4Env.BuildRootP4 + "/Engine/Config/...");
            Sync(P4Env.BuildRootP4 + "/Engine/Content/...");
            Sync(P4Env.BuildRootP4 + "/Engine/Source/...");
            Log("Localize from label {0}", P4Env.LabelToSync);            
        }

        // Setup editor arguments for SCC.
        string EditorArguments = String.Empty;
        if (P4Enabled)
        {
            EditorArguments = String.Format("-SCCProvider={0} -P4Port={1} -P4User={2} -P4Client={3} -P4Passwd={4}", "Perforce", P4Env.P4Port, P4Env.User, P4Env.Client, GetAuthenticationToken());
        }
        else
        {
            EditorArguments = String.Format("-SCCProvider={0}", "None");
        }

        // Setup commandlet arguments for SCC.
        string CommandletSCCArguments = String.Empty;
        if (P4Enabled) { CommandletSCCArguments += (string.IsNullOrEmpty(CommandletSCCArguments) ? "" : " ") + "-EnableSCC"; }
        if (!AllowSubmit) { CommandletSCCArguments += (string.IsNullOrEmpty(CommandletSCCArguments) ? "" : " ") + "-DisableSCCSubmit"; }

        // Setup commandlet arguments with configurations.
        var CommandletArgumentSets = new string[] 
				{
                    String.Format("-config={0}", @"../../../Engine/Config/Localization/Engine.ini") + (string.IsNullOrEmpty(CommandletSCCArguments) ? "" : " " + CommandletSCCArguments),
                    String.Format("-config={0}", @"../../../Engine/Config/Localization/Editor.ini") + (string.IsNullOrEmpty(CommandletSCCArguments) ? "" : " " + CommandletSCCArguments),
                    String.Format("-config={0}", @"../../../Engine/Config/Localization/WordCount.ini"),
				};

        // Execute commandlet for each set of arguments.
        foreach (var CommandletArguments in CommandletArgumentSets)
        {
            Log("Localization for {0} {1}", EditorArguments, CommandletArguments);

            Log("Running UE4Editor to generate Localisation data");

            string Arguments = String.Format("-run=GatherText {0} {1}", EditorArguments, CommandletArguments);
            var RunResult = Run(EditorExe, Arguments);

            if (RunResult.ExitCode != 0)
            {
                throw new AutomationException("Error while executing localization commandlet '{0}'", Arguments);
            }
        }

        // Localisation statistics estimator.
        if (P4Enabled)
        {
            // Available only for P4
            var EstimatorExePath = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/DotNET/TranslatedWordsCountEstimator.exe");
            var StatisticsFilePath = @"\\epicgames.net\root\UE3\Localization\WordCounts\udn.csv";

            var Arguments = string.Format(
                "{0} {1} {2} {3} {4}",
                StatisticsFilePath,
                P4Env.P4Port,
                P4Env.User,
                P4Env.Client,
                Environment.GetEnvironmentVariable("P4PASSWD"));

            var RunResult = Run(EstimatorExePath, Arguments);

            if (RunResult.ExitCode != 0)
            {
                throw new AutomationException("Error while executing TranslatedWordsCountEstimator with arguments '{0}'", Arguments);
            }
        }
    }
}

