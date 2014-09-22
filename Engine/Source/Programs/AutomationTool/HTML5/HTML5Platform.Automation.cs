// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Linq;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;
using System.Diagnostics;
using System.Threading;

public class HTML5Platform : Platform
{
    static string EmscriptenSettingsPath = CombinePaths(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "/.emscripten");

    Dictionary<string, string> ReadEmscriptenSettings()
    {
        // Check HTML5ToolChain.cs
        if (!System.IO.File.Exists(EmscriptenSettingsPath))
        {
            return new Dictionary<string, string>();
        }

        Dictionary<string, string> Settings = new Dictionary<string, string>();
        System.IO.StreamReader SettingFile = new System.IO.StreamReader(EmscriptenSettingsPath);
        string EMLine = SettingFile.ReadToEnd();
        string Pattern = @"(\w+)\s*=\s*['\[]((?:[^'\\]|\\.^)*)['\]]";
        Regex Rgx = new Regex(Pattern, RegexOptions.IgnoreCase);
        MatchCollection Matches = Rgx.Matches(EMLine);
        foreach (Match Matched in Matches)
        {
            if (Matched.Groups.Count == 3 && Matched.Groups[2].ToString() != "")
            {
                Settings[Matched.Groups[1].ToString()] = Matched.Groups[2].ToString();
            }
        }

        return Settings;
    }

	public HTML5Platform()
		: base(UnrealTargetPlatform.HTML5)
	{
	}

	public override void Package(ProjectParams Params, DeploymentContext SC, int WorkingCL)
	{
		Log("Package {0}", Params.RawProjectPath);
        var EmscriptenSettings = ReadEmscriptenSettings();

		string PackagePath = Path.Combine(Path.GetDirectoryName(Params.RawProjectPath), "Binaries", "HTML5");
		if (!Directory.Exists(PackagePath))
		{
			Directory.CreateDirectory(PackagePath);
		}
		string FinalDataLocation = Path.Combine(PackagePath, Params.ShortProjectName) + ".data";


		// we need to operate in the root
		using (new PushedDirectory(Path.Combine(Params.BaseStageDirectory, "HTML5")))
		{
			string BaseSDKPath = Environment.GetEnvironmentVariable("EMSCRIPTEN");
            string PythonPath = null;

            // Check the .emscripten file for a possible python path
            if (EmscriptenSettings.ContainsKey("PYTHON"))
            {
                PythonPath = EmscriptenSettings["PYTHON"];
            }
            // The AutoSDK defines this env var as part of its install. See setup.bat/unsetup.bat
            // If it's missing then just assume that python lives on the path
            if (PythonPath == null)
            {
                PythonPath = Environment.GetEnvironmentVariable("PYTHON");
            }
			// make the file_packager command line
			if (Utils.IsRunningOnMono)
			{
				string PackagerPath = BaseSDKPath + "/tools/file_packager.py";
                if (PythonPath == null)
                {
                    string CmdLine = string.Format("-c \" python {0} '{1}' --preload . --js-output='{1}.js' \" ", PackagerPath, FinalDataLocation);
                    RunAndLog(CmdEnv, "/bin/bash", CmdLine);
                }
                else
                {
                    string CmdLine = string.Format("{0} '{1}' --preload . --js-output='{1}.js' ", PackagerPath, FinalDataLocation);
                    RunAndLog(CmdEnv, PythonPath, CmdLine);
                }
			}
			else
			{
				string PackagerPath = "\"" + BaseSDKPath + "\\tools\\file_packager.py\"";
                if (PythonPath == null)
                {
                    string CmdLine = string.Format("/c python {0} \"{1}\" --preload . --js-output=\"{1}.js\"", PackagerPath, FinalDataLocation);
                    RunAndLog(CmdEnv, CommandUtils.CombinePaths(Environment.SystemDirectory, "cmd.exe"), CmdLine);
                }
                else 
                {
                    string CmdLine = string.Format("{0} \"{1}\" --preload . --js-output=\"{1}.js\"", PackagerPath, FinalDataLocation);
                    RunAndLog(CmdEnv, PythonPath, CmdLine);
                }
			}
		}

		// copy the "Executable" to the package directory
		string GameExe = Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename);
		if (Params.ClientConfigsToBuild[0].ToString() != "Development")
		{
			GameExe += "-HTML5-" + Params.ClientConfigsToBuild[0].ToString();
		}
		GameExe += ".js";
		if (Path.Combine(Path.GetDirectoryName(Params.ProjectGameExeFilename), GameExe) != Path.Combine(PackagePath, GameExe))
		{
			File.Copy(Path.Combine(Path.GetDirectoryName(Params.ProjectGameExeFilename), GameExe), Path.Combine(PackagePath, GameExe), true);
			File.Copy(Path.Combine(Path.GetDirectoryName(Params.ProjectGameExeFilename), GameExe) + ".mem", Path.Combine(PackagePath, GameExe) + ".mem", true);
		}
		File.SetAttributes(Path.Combine(PackagePath, GameExe), FileAttributes.Normal);
		File.SetAttributes(Path.Combine(PackagePath, GameExe) + ".mem", FileAttributes.Normal);

		// put the HTML file to the package directory
		string TemplateFile = Path.Combine(CombinePaths(CmdEnv.LocalRoot, "Engine"), "Build", "HTML5", "Game.html.template");
		string OutputFile = Path.Combine(PackagePath, (Params.ClientConfigsToBuild[0].ToString() != "Development" ? (Params.ShortProjectName + "-HTML5-" + Params.ClientConfigsToBuild[0].ToString()) : Params.ShortProjectName)) + ".html";

		// find Heap Size.
		ulong HeapSize;
		var ConfigCache = new UnrealBuildTool.ConfigCacheIni(UnrealTargetPlatform.HTML5, "Engine", Path.GetDirectoryName(Params.RawProjectPath), CombinePaths(CmdEnv.LocalRoot, "Engine"));

		int ConfigHeapSize;
		if (!ConfigCache.GetInt32("BuildSettings", "HeapSize" + Params.ClientConfigsToBuild[0].ToString(), out ConfigHeapSize)) // in Megs.
		{
			// we couldn't find a per config heap size, look for a common one.
			if (!ConfigCache.GetInt32("BuildSettings", "HeapSize", out ConfigHeapSize))
			{
				ConfigHeapSize = Params.IsCodeBasedProject ? 1024 : 512;
				Log("Could not find Heap Size setting in .ini for Client config {0}", Params.ClientConfigsToBuild[0].ToString());
			}
		}

		HeapSize = (ulong)ConfigHeapSize * 1024L * 1024L; // convert to bytes.
		Log("Setting Heap size to {0} Mb ", ConfigHeapSize);


		GenerateFileFromTemplate(TemplateFile, OutputFile, Params.ShortProjectName, Params.ClientConfigsToBuild[0].ToString(), Params.StageCommandline, !Params.IsCodeBasedProject, HeapSize);

		// copy the jstorage files to the binaries directory
		string JSDir = Path.Combine(CombinePaths(CmdEnv.LocalRoot, "Engine"), "Build", "HTML5");
		string OutDir = PackagePath;
		File.Copy(JSDir + "/json2.js", OutDir + "/json2.js", true);
		File.SetAttributes(OutDir + "/json2.js", FileAttributes.Normal);
		File.Copy(JSDir + "/jstorage.js", OutDir + "/jstorage.js", true);
		File.SetAttributes(OutDir + "/jstorage.js", FileAttributes.Normal);
		File.Copy(JSDir + "/moz_binarystring.js", OutDir + "/moz_binarystring.js", true);
		File.SetAttributes(OutDir + "/moz_binarystring.js", FileAttributes.Normal);
		PrintRunTime();
	}

	protected void GenerateFileFromTemplate(string InTemplateFile, string InOutputFile, string InGameName, string InGameConfiguration, string InArguments, bool IsContentOnly, ulong HeapSize)
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

				if (LineStr.Contains("%HEAPSIZE%"))
				{
					LineStr = LineStr.Replace("%HEAPSIZE%", HeapSize.ToString());
				}

				if (LineStr.Contains("%CONFIG%"))
				{
					if (IsContentOnly)
						InGameName = "UE4Game";
					LineStr = LineStr.Replace("%CONFIG%", (InGameConfiguration != "Development" ? (InGameName + "-HTML5-" + InGameConfiguration) : InGameName));
				}

				if (LineStr.Contains("%UE4CMDLINE%"))
				{
					InArguments = InArguments.Replace ("\"", "");
					string[] Arguments = InArguments.Split(' ');
					string ArgumentString = IsContentOnly ? "'" + InGameName + "/" + InGameName + ".uproject '," : "";
					for (int i = 0; i < Arguments.Length - 1; ++i)
					{
						ArgumentString += "'";
			   		    ArgumentString += Arguments[i];
						ArgumentString += "'";
						ArgumentString += ",' ',";
					}
					if (Arguments.Length > 0)
					{
						ArgumentString += "'";
						ArgumentString += Arguments[Arguments.Length - 1];
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

	public override void GetFilesToArchive(ProjectParams Params, DeploymentContext SC)
	{
		if (SC.StageTargetConfigurations.Count != 1)
		{
			throw new AutomationException("iOS is currently only able to package one target configuration at a time, but StageTargetConfigurations contained {0} configurations", SC.StageTargetConfigurations.Count);
		}

		string PackagePath = Path.Combine(Path.GetDirectoryName(Params.RawProjectPath), "Binaries", "HTML5");
		string FinalDataLocation = Path.Combine(PackagePath, Params.ShortProjectName) + ".data";

		// copy the "Executable" to the archive directory
		string GameExe = Path.GetFileNameWithoutExtension(Params.ProjectGameExeFilename);
		if (Params.ClientConfigsToBuild[0].ToString() != "Development")
		{
			GameExe += "-HTML5-" + Params.ClientConfigsToBuild[0].ToString();
		}
		GameExe += ".js";

		// put the HTML file to the package directory
		string OutputFile = Path.Combine(PackagePath, (Params.ClientConfigsToBuild[0].ToString() != "Development" ? (Params.ShortProjectName + "-HTML5-" + Params.ClientConfigsToBuild[0].ToString()) : Params.ShortProjectName)) + ".html";

		SC.ArchiveFiles(PackagePath, Path.GetFileName(FinalDataLocation));
		SC.ArchiveFiles(PackagePath, Path.GetFileName(FinalDataLocation + ".js"));
		SC.ArchiveFiles(PackagePath, Path.GetFileName(GameExe));
		SC.ArchiveFiles(PackagePath, Path.GetFileName(GameExe + ".mem"));
		SC.ArchiveFiles(PackagePath, Path.GetFileName("json2.js"));
		SC.ArchiveFiles(PackagePath, Path.GetFileName("jstorage.js"));
		SC.ArchiveFiles(PackagePath, Path.GetFileName("moz_binarystring.js"));
		SC.ArchiveFiles(PackagePath, Path.GetFileName(OutputFile));
	}

	public override ProcessResult RunClient(ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
        // look for browser
        var ConfigCache = new UnrealBuildTool.ConfigCacheIni(UnrealTargetPlatform.HTML5, "Engine", Path.GetDirectoryName(Params.RawProjectPath), CombinePaths(CmdEnv.LocalRoot, "Engine"));

        string DeviceSection;

        if ( Utils.IsRunningOnMono )
        {
            DeviceSection = "HTML5DevicesMac";
        }
        else
        {
            DeviceSection = "HTML5DevicesWindows";
        }

        string browserPath;
        string DeviceName = Params.Device.Split('@')[1];
        bool ok = ConfigCache.GetString(DeviceSection, DeviceName, out browserPath);

		if (!ok)
			throw new System.Exception ("Incorrect browser configuration in HTML5Engine.ini ");

		// open the webpage
        string directory = Path.GetDirectoryName(ClientApp);
        string url = Path.GetFileName(ClientApp) +".html";
		// Are we running via cook on the fly server?
		// find our http url - This is awkward because RunClient doesn't have real information that NFS is running or not.
		bool IsCookOnTheFly = false;
		string[] Arguments = ClientCmdLine.Split(' ');
		foreach (var Argument in Arguments)
		{
			string[] KeyVal = Argument.Split('=');
			if (KeyVal[0].ToLower().Contains("filehostip"))
			{
				// split urls.
				string[] Urls = KeyVal[1].Split('+');
				foreach (var Url in Urls)
				{
					if (Url.Contains("http"))
					{
						url = Url + "/" + Path.GetFileName(url);
						IsCookOnTheFly = true;
					}
				}
			}
			if (IsCookOnTheFly)
				break;
		}

        if (IsCookOnTheFly)
        {
            url += "?cookonthefly=true";
        }
        else
        {

            url = "http://127.0.0.1:8000/" + url;
            // this will be killed UBT instances dies.
            string input = String.Format(" -m SimpleHTTPServer 8000");


			string PythonName = "python2.exe";

			if (Utils.IsRunningOnMono)
				PythonName = "python2";

			ProcessResult Result = ProcessManager.CreateProcess(PythonName, true, "html5server.log");
			Result.ProcessObject.StartInfo.FileName = PythonName;
            Result.ProcessObject.StartInfo.UseShellExecute = false;
            Result.ProcessObject.StartInfo.RedirectStandardOutput = true;
            Result.ProcessObject.StartInfo.RedirectStandardInput = true;
            Result.ProcessObject.StartInfo.WorkingDirectory = directory;
            Result.ProcessObject.StartInfo.Arguments = input;
            Result.ProcessObject.Start();


            Result.ProcessObject.OutputDataReceived += delegate(object sender, System.Diagnostics.DataReceivedEventArgs e)
            {
                System.Console.WriteLine(e.Data);
            };

            System.Console.WriteLine("Starting Browser Process");


			// safari specific hack.
			string argument = url; 
			if (browserPath.Contains ("Safari") && Utils.IsRunningOnMono)
				argument = ""; 

            ProcessResult ClientProcess = Run(browserPath, argument, null, ClientRunFlags | ERunOptions.NoWaitForExit);

            ClientProcess.ProcessObject.EnableRaisingEvents = true;
            ClientProcess.ProcessObject.Exited += delegate(System.Object o, System.EventArgs e)
            {
                System.Console.WriteLine("Browser Process Ended - Killing Webserver");
                // send kill.
                Result.ProcessObject.StandardInput.Close();
                Result.ProcessObject.Kill();
            };

			// safari needs a hack. 
			// http://superuser.com/questions/689315/run-safari-from-terminal-with-given-url-address-without-open-command

			if (browserPath.Contains ("Safari") && Utils.IsRunningOnMono) 
			{
				// ClientProcess.ProcessObject.WaitForInputIdle ();
				Thread.Sleep (2000);
				Process.Start("/usr/bin/osascript"," -e 'tell application \"Safari\" to open location \"" + url + "\"'" );
			}

            return ClientProcess;
        }

        System.Console.WriteLine("Browser Path " + browserPath);
        ProcessResult BrowserProcess = Run(browserPath, url, null, ClientRunFlags | ERunOptions.NoWaitForExit);
	
        return BrowserProcess;

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

	public override List<string> GetDebugFileExtentions()
	{
		return new List<string> { ".pdb" };
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
