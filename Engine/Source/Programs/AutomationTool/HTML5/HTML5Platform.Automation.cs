// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
#if !MONO
using Microsoft.Win32; 
#endif 

public class HTML5Platform : Platform
{
	public HTML5Platform()
		:base(UnrealTargetPlatform.HTML5)
	{
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		Log("Package {0}", Params.RawProjectPath);

		string FinalDataLocation = Path.Combine(Path.GetDirectoryName(Path.GetFullPath(Params.ProjectGameExeFilename)), Params.ProjectBinariesFolder, Params.ShortProjectName) +".data";

		// we need to operate in the root
		using (new PushedDirectory(Path.Combine(Params.BaseStageDirectory, "HTML5")))
		{

            string BaseSDKPath = "";
#if !MONO
            RegistryKey localKey = RegistryKey.OpenBaseKey(Microsoft.Win32.RegistryHive.LocalMachine, RegistryView.Registry64);
            RegistryKey key = localKey.OpenSubKey("Software\\Emscripten64");
            if (key != null)
            {
                string sdkdir = key.GetValue("Install_Dir") as string;
                // emscripten path is the highest numbered directory 
                DirectoryInfo dInfo = new DirectoryInfo(sdkdir + "\\emscripten");
                string Latest_Ver = (from S in dInfo.GetDirectories() select S.Name).ToList().Last();
                BaseSDKPath = sdkdir + @"\emscripten\" + Latest_Ver;
            }
            else
            {
                BaseSDKPath = Environment.ExpandEnvironmentVariables("%EMSCRIPTEN%");
                
            }
#else 
            BaseSDKPath = Environment.ExpandEnvironmentVariables("%EMSCRIPTEN%"); 
#endif
            // make the file_packager command line
            string PackagerPath = "\""  + BaseSDKPath + "\\tools\\file_packager.py\"";
			string CmdLine = string.Format("/c {0} {1} --preload . --pre-run --js-output={1}.js", PackagerPath, FinalDataLocation);

			// package it up!
			RunAndLog(CmdEnv, CommandUtils.CombinePaths(Environment.SystemDirectory, "cmd.exe"), CmdLine);
		}
		
		// put the HTML file to the binaries directory
		string TemplateFile = Path.Combine(CombinePaths(CmdEnv.LocalRoot, "Engine"), "Build", "HTML5", "Game.html.template");
		string OutputFile = Path.Combine(Path.GetDirectoryName(Path.GetFullPath(Params.ProjectGameExeFilename)), Params.ProjectBinariesFolder, Params.ShortProjectName) + ".html";
    	GenerateFileFromTemplate(TemplateFile, OutputFile, Params.ShortProjectName, Params.ClientConfigsToBuild[0].ToString(), Params.StageCommandline, !Params.IsCodeBasedProject);
		PrintRunTime();
	}

	protected void GenerateFileFromTemplate(string InTemplateFile, string InOutputFile, string InGameName, string InGameConfiguration, string InArguments, bool IsContentOnly)
	{
		StringBuilder outputContents = new StringBuilder();
		using (StreamReader reader = new StreamReader(InTemplateFile))
		{
			string LineStr = null;
			while (reader.Peek() != -1)
			{
				LineStr = reader.ReadLine();
				if (LineStr.Contains("%GAME%"))
				{
					LineStr = LineStr.Replace("%GAME%", InGameName);
				}

				if (LineStr.Contains("%CONFIG%"))
				{
                    if (IsContentOnly)
                        InGameName = "UE4Game";
					LineStr = LineStr.Replace("%CONFIG%", (InGameConfiguration != "Development" ? (InGameName + "-" + InGameConfiguration) : InGameName));
				}

				if (LineStr.Contains("%UE4CMDLINE%"))
				{
					string[] Arguments = InArguments.Split(' ');
                    string ArgumentString = IsContentOnly ? "'" + InGameName + "/" + InGameName + ".uproject ',"  : "";
					for (int i = 0; i < Arguments.Length-1; ++i)
					{
						ArgumentString += "'";
						ArgumentString += Arguments[i];
						ArgumentString += "'";
						ArgumentString += ",' ',";
					}
					if (Arguments.Length > 0)
					{
						ArgumentString += "'";
						ArgumentString += Arguments[Arguments.Length-1];
						ArgumentString += "'";
					}
					LineStr = LineStr.Replace("%UE4CMDLINE%", ArgumentString);

				}

				outputContents.AppendLine(LineStr);
			}
		}

		if (outputContents.Length > 0)
		{
			// Save the file
			try
			{
				Directory.CreateDirectory(Path.GetDirectoryName(InOutputFile));
				File.WriteAllText(InOutputFile, outputContents.ToString(), Encoding.UTF8);
			}
			catch (Exception)
			{
				// Unable to write to the project file.
			}
		}
	}

	public override void GetFilesToDeployOrStage(ProjectParams Params, DeploymentContext SC)
	{
	}

	public override ProcessResult RunClient(ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
		// look for firefox
		string[] PossiblePaths = new string[] 
		{
			Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), @"Nightly\firefox.exe"),
			Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), @"Nightly\firefox.exe"),
			Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), @"Mozilla Firefox\firefox.exe"),
			Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), @"Mozilla Firefox\firefox.exe"),
		};

		// set up a directory for temp profile
		string FirefoxProfileCommand = " -no-remote ";

		// look for the best firefox to tun
		string FirefoxPath = null;
		foreach (string PossiblePath in PossiblePaths)
		{
			if (File.Exists(PossiblePath))
			{
				FirefoxPath = PossiblePath;
				break;
			}
		}

		// if we didn't find one, just use the shell to open it
		if (FirefoxPath == null)
		{
			FirefoxPath = CmdEnv.CmdExe;
			FirefoxProfileCommand = " ";
		}

   		// open the webpage
		// what to do with the commandline!??
        string HTMLPath = Path.ChangeExtension(ClientApp, "html");
        if  ( !File.Exists(HTMLPath) )  // its probably a content only game - then it exists in the UE4 directory and not in the game directory. 
            HTMLPath = Path.Combine(CombinePaths(CmdEnv.LocalRoot, "Engine"), "Binaries", "HTML5", Path.GetFileName(HTMLPath)); 

		ProcessResult ClientProcess = Run(FirefoxPath, FirefoxProfileCommand + HTMLPath , null, ClientRunFlags | ERunOptions.NoWaitForExit);

		return ClientProcess;
	}

	public override string GetCookPlatform(bool bDedicatedServer, bool bIsClientOnly, string CookFlavor)
	{
		return "HTML5";
	}

	public override bool DeployPakInternalLowerCaseFilenames()
	{
		return false;
	}

	public override bool DeployLowerCaseFilenames(bool bUFSFile)
	{
		return false;
	}

	public override string LocalPathToTargetPath(string LocalPath, string LocalRoot)
	{
		return LocalPath;//.Replace("\\", "/").Replace(LocalRoot, "../../..");
	}

	public override bool IsSupported { get { return true; } }

    public override string GUBP_GetPlatformFailureEMails(string Branch)
    {
        return "Peter.Sauerbrei[epic]";
    }

	#region Hooks

	public override void PreBuildAgenda(UE4Build Build, UE4Build.BuildAgenda Agenda)
	{
	}

	public override List<string> GetExecutableNames(DeploymentContext SC, bool bIsRun = false)
	{
		var ExecutableNames = new List<String>();
		ExecutableNames.Add(Path.Combine(SC.ProjectRoot, "Binaries", "HTML5", SC.ShortProjectName));
		return ExecutableNames;
	}
	#endregion
}
