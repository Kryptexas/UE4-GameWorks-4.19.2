// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.IO;
using AutomationTool;
//using OneSky;

class LauncherLocalization : BuildCommand
{
    public override void ExecuteBuild()
    {
        var EditorExe = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/UE4Editor-Cmd.exe");

        if (P4Enabled)
        {
            Log("Sync necessary content to head revision");
			P4.Sync(P4Env.BuildRootP4 + "/Engine/Config/...");
			P4.Sync(P4Env.BuildRootP4 + "/Engine/Content/...");
			P4.Sync(P4Env.BuildRootP4 + "/Engine/Source/...");

            P4.Sync(P4Env.BuildRootP4 + "/Engine/Programs/NoRedist/UnrealEngineLauncher/Config/...");
            P4.Sync(P4Env.BuildRootP4 + "/Engine/Programs/NoRedist/UnrealEngineLauncher/Content/...");
            //P4.Sync(P4Env.BuildRootP4 + "/Engine/Source/..."); <- takes care of syncing Launcher source already

            Log("Localize from label {0}", P4Env.LabelToSync);            
        }

        //var oneSkyService = new OneSkyService("Reo5M8fnKcqVANHb4Zjc5nVUeXeehhGZ", "ByaWoHTjLM6clyxWLrrhhgdTY1btatdB");

        //// Export Launcher text from OneSky
        //{
        //    var launcherGroup = GetLauncherGroup(oneSkyService);
        //    var appProject = GetAppProject(oneSkyService);
        //    var appFile = appProject.UploadedFiles.FirstOrDefault(f => f.Filename == "App.po");

        //    //Export
        //    if (appFile != null)
        //    {
        //        ExportFileToDirectory(appFile, new DirectoryInfo(CmdEnv.LocalRoot + "/Engine/Programs/NoRedist/UnrealEngineLauncher/Content/Localization/App"), launcherGroup.EnabledCultures);
        //    }
        //}

        // Setup editor arguments for SCC.
        string EditorArguments = String.Empty;
        if (P4Enabled)
        {
            EditorArguments = String.Format("-SCCProvider={0} -P4Port={1} -P4User={2} -P4Client={3} -P4Passwd={4}", "Perforce", P4Env.P4Port, P4Env.User, P4Env.Client, P4.GetAuthenticationToken());
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
                    String.Format("-config={0}", @"../../../Engine/Programs/NoRedist/UnrealEngineLauncher/Config/Localization/App.ini") + (string.IsNullOrEmpty(CommandletSCCArguments) ? "" : " " + CommandletSCCArguments),
                    String.Format("-config={0}", @"../../../Engine/Programs/NoRedist/UnrealEngineLauncher/Config/Localization/WordCount.ini"),
                };

        // Execute commandlet for each set of arguments.
        foreach (var CommandletArguments in CommandletArgumentSets)
        {
            Log("Localization for {0} {1}", EditorArguments, CommandletArguments);

            Log("Running UE4Editor to generate Localization data");

            string Arguments = String.Format("-run=GatherText {0} {1}", EditorArguments, CommandletArguments);
            var RunResult = Run(EditorExe, Arguments);

            if (RunResult.ExitCode != 0)
            {
                throw new AutomationException("Error while executing localization commandlet '{0}'", Arguments);
            }
        }

        // Upload Launcher text to OneSky
        //UploadDirectoryToProject(GetAppProject(oneSkyService), new DirectoryInfo(CmdEnv.LocalRoot + "/Engine/Programs/NoRedist/UnrealEngineLauncher/Content/Localization/App"), "*.po");
    }

    //private static ProjectGroup GetLauncherGroup(OneSkyService oneSkyService)
    //{
    //    var launcherGroup = oneSkyService.ProjectGroups.FirstOrDefault(g => g.Name == "Launcher");

    //    if (launcherGroup == null)
    //    {
    //        launcherGroup = new ProjectGroup("Launcher", new CultureInfo("en"));
    //        oneSkyService.ProjectGroups.Add(launcherGroup);
    //    }

    //    return launcherGroup;
    //}

    //private static OneSky.Project GetAppProject(OneSkyService oneSkyService)
    //{
    //    var launcherGroup = GetLauncherGroup(oneSkyService);

    //    OneSky.Project appProject = launcherGroup.Projects.FirstOrDefault(p => p.Name == "App");

    //    if (appProject == null)
    //    {
    //        ProjectType projectType = oneSkyService.ProjectTypes.First(pt => pt.Code == "website");

    //        appProject = new OneSky.Project("App", "The core application text that ships with the Launcher", projectType);
    //        launcherGroup.Projects.Add(appProject);
    //    }

    //    return appProject;
    //}

    //private static void ExportFileToDirectory(UploadedFile file, DirectoryInfo destination, IEnumerable<CultureInfo> cultures)
    //{
    //    foreach (var culture in cultures)
    //    {
    //        var cultureDirectory = new DirectoryInfo(Path.Combine(destination.FullName, culture.Name));
    //        if (!cultureDirectory.Exists)
    //        {
    //            cultureDirectory.Create();
    //        }

    //        using (var memoryStream = new MemoryStream())
    //        {
    //            var exportFile = new FileInfo(Path.Combine(cultureDirectory.FullName, file.Filename));

    //            var exportTranslationState = file.ExportTranslation(culture, memoryStream).Result;
    //            if (exportTranslationState == UploadedFile.ExportTranslationState.Success)
    //            {
    //                memoryStream.Position = 0;
    //                using (Stream fileStream = File.OpenWrite(exportFile.FullName))
    //                {
    //                    memoryStream.CopyTo(fileStream);
    //                    Console.WriteLine("[SUCCESS] Exporting: " + exportFile.FullName + " Locale: " + culture.Name);
    //                }
    //            }
    //            else if (exportTranslationState == UploadedFile.ExportTranslationState.NoContent)
    //            {
    //                Console.WriteLine("[WARNING] Exporting: " + exportFile.FullName + " Locale: " + culture.Name + " has no translations!");
    //            }
    //            else
    //            {
    //                Console.WriteLine("[FAILED] Exporting: " + exportFile.FullName + " Locale: " + culture.Name);
    //            }
    //        }
    //    }
    //}

    //static void UploadDirectoryToProject(OneSky.Project project, DirectoryInfo directory, string fileExtension)
    //{
    //    foreach (var file in Directory.GetFiles(directory.FullName, fileExtension, SearchOption.AllDirectories))
    //    {
    //        DirectoryInfo parentDirectory = Directory.GetParent(file);
    //        string localeName = parentDirectory.Name;
    //        string currentFile = file;

    //        using (var fileStream = File.OpenRead(currentFile))
    //        {
    //            // Read the BOM
    //            var bom = new byte[3];
    //            fileStream.Read(bom, 0, 3);

    //            //We want to ignore the utf8 BOM
    //            if (bom[0] != 0xef || bom[1] != 0xbb || bom[2] != 0xbf)
    //            {
    //                fileStream.Position = 0;
    //            }

    //            Console.WriteLine("Uploading: " + currentFile + " Locale: " + localeName);
    //            var uploadedFile = project.Upload(Path.GetFileName(currentFile), fileStream, new CultureInfo(localeName)).Result;

    //            if (uploadedFile == null)
    //            {
    //                Console.WriteLine("[FAILED] Uploading: " + currentFile + " Locale: " + localeName);
    //            }
    //            else
    //            {
    //                Console.WriteLine("[SUCCESS] Uploading: " + currentFile + " Locale: " + localeName);
    //            }
    //        }
    //    }
    //}
}

