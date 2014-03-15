// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Security;
using System.Security.Principal;
using System.Security.Permissions;
using System.Runtime.InteropServices;
using System.Runtime.ConstrainedExecution;
using Microsoft.Win32.SafeHandles;
using AutomationTool;
using System.Threading;
using UnrealBuildTool;

[Help("Tests P4 functionality. Runs 'p4 info'.")]
[RequireP4]
class TestP4_Info : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("P4CLIENT={0}", GetEnvVar("P4CLIENT"));
		Log("P4PORT={0}", GetEnvVar("P4PORT"));

		var Result = P4("info");
		if (Result != 0)
		{
			throw new AutomationException("p4 info failed: {0}", Result.Output);
		}
	}
}

[Help("Throws an automation exception.")]
class TestFail : BuildCommand
{
	public override void ExecuteBuild()
	{
		throw new AutomationException("TestFail throwing an exception, cause that is what I do.");
	}
}

[Help("Prints a message and returns success.")]
class TestSuccess : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("TestSuccess message.");
	}
}

[Help("Prints a message and returns success.")]
class TestMessage : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("TestMessage message.");
		Log("you must be in error");
		Log("Behold the Error of you ways");
		Log("ERROR, you are silly");
		Log("ERROR: Something must be broken");
	}
}

[Help("Calls UAT recursively with a given command line.")]
class TestRecursion : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("TestRecursion *********************");
		string Params = ParseParamValue("Cmd");
		Log("Recursive Command: {0}", Params);
		RunUAT(CmdEnv, Params);

	}
}
[Help("Calls UAT recursively with a given command line.")]
class TestRecursionAuto : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("TestRecursionAuto *********************");
		RunUAT(CmdEnv, "TestMessage");
		RunUAT(CmdEnv, "TestMessage");
		RunUAT(CmdEnv, "TestRecursion -Cmd=TestMessage");
		RunUAT(CmdEnv, "TestRecursion -Cmd=TestFail");

	}
}

[Help("Makes a zip file in Rocket/QFE")]
class TestMacZip : BuildCommand
{
    public override void ExecuteBuild()
    {
        Log("TestMacZip *********************");

        if (UnrealBuildTool.Utils.IsRunningOnMono)
        {
			PushDir(CombinePaths(CmdEnv.LocalRoot, "Rocket/QFE"));
            RunAndLog(CommandUtils.CmdEnv, "zip", "-r TestZip .");
            PopDir();
        }
        else
        {
            throw new AutomationException("This probably only works on the mac.");
        }
    }
}

[Help("Tests the temp storage operations.")]
class TestTempStorage : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("TestTempStorage********");
		DeleteLocalTempStorageManifests(CmdEnv);
		DeleteSharedTempStorageManifests(CmdEnv, "Test");
		if (TempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("storage should not exist");
		}

		string UnitTestFile = CombinePaths(CmdEnv.LocalRoot, "Engine", "Build", "Batchfiles", "TestFile.Txt");
		Log("Test file {0}", UnitTestFile);

		if (FileExists(UnitTestFile))
		{
			DeleteFile(UnitTestFile);
		}
		string[] LinesToWrite = new string[]
		{
			"This is a test",
			"Of writing to file " + UnitTestFile
		};
		foreach (string Line in LinesToWrite)
		{
			WriteToFile(UnitTestFile, Line);
		}

		string UnitTestFile2 = CombinePaths(CmdEnv.LocalRoot, "TestFile2.Txt");
		Log("Test file {0}", UnitTestFile2);

		if (FileExists(UnitTestFile2))
		{
			DeleteFile(UnitTestFile2);
		}
		string[] LinesToWrite2 = new string[]
		{
			"This is a test",
			"Of writing to file " + UnitTestFile2
		};
		foreach (string Line in LinesToWrite2)
		{
			WriteToFile(UnitTestFile2, Line);
		}

		string UnitTestFile3 = CombinePaths(CmdEnv.LocalRoot, "engine", "plugins", "TestFile3.Txt");
		Log("Test file {0}", UnitTestFile3);

		if (FileExists(UnitTestFile3))
		{
			DeleteFile(UnitTestFile3);
		}
		string[] LinesToWrite3 = new string[]
		{
			"This is a test",
			"Of writing to file " + UnitTestFile3
		};
		foreach (string Line in LinesToWrite3)
		{
			WriteToFile(UnitTestFile3, Line);
		}

		{
			string[] LinesRead = ReadAllLines(UnitTestFile);
			if (LinesRead == null || LinesRead.Length != LinesToWrite.Length)
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
			for (int LineIndex = 0; LineIndex < LinesRead.Length; ++LineIndex)
			{
				if (LinesRead[LineIndex] != LinesToWrite[LineIndex])
				{
					throw new AutomationException("Contents of the file created is different to the file read.");
				}
			}

			string[] LinesRead2 = ReadAllLines(UnitTestFile2);
			if (LinesRead2 == null || LinesRead2.Length != LinesToWrite2.Length)
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
			for (int LineIndex = 0; LineIndex < LinesRead2.Length; ++LineIndex)
			{
				if (LinesRead2[LineIndex] != LinesToWrite2[LineIndex])
				{
					throw new AutomationException("Contents of the file created is different to the file read.");
				}
			}

			string[] LinesRead3 = ReadAllLines(UnitTestFile3);
			if (LinesRead3 == null || LinesRead3.Length != LinesToWrite3.Length)
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
			for (int LineIndex = 0; LineIndex < LinesRead3.Length; ++LineIndex)
			{
				if (LinesRead3[LineIndex] != LinesToWrite3[LineIndex])
				{
					throw new AutomationException("Contents of the file created is different to the file read.");
				}
			}
		}
		var TestFiles = new List<string> { UnitTestFile, UnitTestFile2, UnitTestFile3 };

		StoreToTempStorage(CmdEnv, "Test", TestFiles);

		if (!LocalTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("local storage should exist");
		}
		if (!SharedTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("shared storage should exist");
		}
		DeleteLocalTempStorageManifests(CmdEnv);
		if (LocalTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("local storage should not exist");
		}
		if (!TempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("some storage should exist");
		}
		DeleteFile(UnitTestFile);
		DeleteFile(UnitTestFile2);
		DeleteFile(UnitTestFile3);

		RetrieveFromTempStorage(CmdEnv, "Test");
		if (!LocalTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("local storage should exist");
		}
		if (!SharedTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("shared storage should exist");
		}
		{
			string[] LinesRead = ReadAllLines(UnitTestFile);
			if (LinesRead == null || LinesRead.Length != LinesToWrite.Length)
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
			for (int LineIndex = 0; LineIndex < LinesRead.Length; ++LineIndex)
			{
				if (LinesRead[LineIndex] != LinesToWrite[LineIndex])
				{
					throw new AutomationException("Contents of the file created is different to the file read.");
				}
			}

			string[] LinesRead2 = ReadAllLines(UnitTestFile2);
			if (LinesRead2 == null || LinesRead2.Length != LinesToWrite2.Length)
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
			for (int LineIndex = 0; LineIndex < LinesRead2.Length; ++LineIndex)
			{
				if (LinesRead2[LineIndex] != LinesToWrite2[LineIndex])
				{
					throw new AutomationException("Contents of the file created is different to the file read.");
				}
			}

			string[] LinesRead3 = ReadAllLines(UnitTestFile3);
			if (LinesRead3 == null || LinesRead3.Length != LinesToWrite3.Length)
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
			for (int LineIndex = 0; LineIndex < LinesRead3.Length; ++LineIndex)
			{
				if (LinesRead3[LineIndex] != LinesToWrite3[LineIndex])
				{
					throw new AutomationException("Contents of the file created is different to the file read.");
				}
			}
		}
		DeleteSharedTempStorageManifests(CmdEnv, "Test");
		if (SharedTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("shared storage should not exist");
		}
		RetrieveFromTempStorage(CmdEnv, "Test"); // this should just rely on the local
		if (!LocalTempStorageExists(CmdEnv, "Test"))
		{
			throw new AutomationException("local storage should exist");
		}

		// and now lets test tampering
		DeleteLocalTempStorageManifests(CmdEnv);
		{
			bool bFailedProperly = false;
			var MissingFile = new List<string>(TestFiles);
			MissingFile.Add(CombinePaths(CmdEnv.LocalRoot, "Engine", "SomeFileThatDoesntExist.txt"));
			try
			{
				StoreToTempStorage(CmdEnv, "Test", MissingFile);
			}
			catch (AutomationException)
			{
				bFailedProperly = true;
			}
			if (!bFailedProperly)
			{
				throw new AutomationException("Missing file did not fail.");
			}
		}
		DeleteSharedTempStorageManifests(CmdEnv, "Test"); // this ends up being junk
		StoreToTempStorage(CmdEnv, "Test", TestFiles);
		DeleteLocalTempStorageManifests(CmdEnv); // force a load from shared
		var SharedFile = CombinePaths(SharedTempStorageDirectory("Test"), "Engine", "Build", "Batchfiles", "TestFile.Txt");
		if (!FileExists_NoExceptions(SharedFile))
		{
			throw new AutomationException("Shared file {0} did not exist", SharedFile);
		}
		DeleteFile(SharedFile);
		{
			bool bFailedProperly = false;
			try
			{
				RetrieveFromTempStorage(CmdEnv, "Test");
			}
			catch (AutomationException)
			{
				bFailedProperly = true;
			}
			if (!bFailedProperly)
			{
				throw new AutomationException("Did not fail to load from missing file.");
			}
		}
		DeleteSharedTempStorageManifests(CmdEnv, "Test");
		StoreToTempStorage(CmdEnv, "Test", TestFiles);
		DeleteFile(UnitTestFile);
		{
			bool bFailedProperly = false;
			try
			{
				RetrieveFromTempStorage(CmdEnv, "Test");
			}
			catch (AutomationException)
			{
				bFailedProperly = true;
			}
			if (!bFailedProperly)
			{
				throw new AutomationException("Did not fail to load from missing local file.");
			}
		}
		DeleteSharedTempStorageManifests(CmdEnv, "Test");
		DeleteLocalTempStorageManifests(CmdEnv);
		DeleteFile(UnitTestFile);
		DeleteFile(UnitTestFile2);
		DeleteFile(UnitTestFile3);
	}
}

[Help("Tests P4 functionality. Creates a new changelist under the workspace %P4CLIENT%")]
[RequireP4]
class TestP4_CreateChangelist : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("P4CLIENT={0}", GetEnvVar("P4CLIENT"));
		Log("P4PORT={0}", GetEnvVar("P4PORT"));

		var CLDescription = "AutomationTool TestP4";

		Log("Creating new changelist \"{0}\" using client \"{1}\"", CLDescription, GetEnvVar("P4CLIENT"));
		var ChangelistNumber = CreateChange(Description: CLDescription);
		Log("Created changelist {0}", ChangelistNumber);
	}
}

[RequireP4]
class TestP4_StrandCheckout : BuildCommand
{
	public override void ExecuteBuild()
	{
		int WorkingCL = CreateChange(P4Env.Client, String.Format("TestP4_StrandCheckout, head={0}", P4Env.Changelist));

		Log("Build from {0}    Working in {1}", P4Env.Changelist, WorkingCL);

		List<string> Sign = new List<string>();
		Sign.Add(CombinePaths(CmdEnv.LocalRoot, @"\Engine\Binaries\DotNET\AgentInterface.dll"));

		Log("Signing and adding {0} build products to changelist {1}...", Sign.Count, WorkingCL);

		CodeSign.SignMultipleIfEXEOrDLL(this, Sign);
		foreach (var File in Sign)
		{
			Sync("-f -k " + File + "#head"); // sync the file without overwriting local one
			if (!FileExists(File))
			{
				throw new AutomationException("BUILD FAILED {0} was a build product but no longer exists", File);
			}

			ReconcileNoDeletes(WorkingCL, File);
		}
	}
}

[RequireP4]
class TestP4_LabelDescription : BuildCommand
{
	public override void ExecuteBuild()
	{
		string Label = GetEnvVar("SBF_LabelFromUser");
		string Desc;
		Log("LabelDescription {0}", Label);
		var Result = LabelDescription(Label, out Desc);
		if (!Result)
		{
			throw new AutomationException("Could not get label description");
		}
		Log("Result***\n{0}\n***\n", Desc);
	}
}

[RequireP4]
class TestP4_ClientOps : BuildCommand
{
    public override void ExecuteBuild()
    {
        string TemplateClient = "ue4_licensee_workspace";
        var Clients = P4GetClientsForUser("UE4_Licensee");

        string TestClient = "UAT_Test_Client";

        P4ClientInfo NewClient = null;
        foreach (var Client in Clients)
        {
            if (Client.Name == TemplateClient)
            {
                NewClient = Client;
                break;
            }
        }
        if (NewClient == null)
        {
            throw new AutomationException("Could not find template");
        }
        NewClient.Owner = P4Env.User; // this is not right, we need the actual licensee user!
        NewClient.Host = Environment.MachineName.ToLower();
        NewClient.RootPath = @"C:\TestClient";
        NewClient.Name = TestClient;
        if (DoesClientExist(TestClient))
        {
            DeleteClient(TestClient);
        }
        CreateClient(NewClient);

        //P4CLIENT         Name of client workspace        p4 help client
        //P4PASSWD         User password passed to server  p4 help passwd

        string[] VarsToSave = 
        {
            "P4CLIENT",
            //"P4PASSWD",

        };
        var KeyValues = new Dictionary<string, string>();
        foreach (var ToSave in VarsToSave)
        {
            KeyValues.Add(ToSave, GetEnvVar(ToSave));
        }

        SetEnvVar("P4CLIENT", NewClient.Name);
        //SetEnv("P4PASSWD", ParseParamValue("LicenseePassword");


        //Sync(CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "UE4Games.uprojectdirs"));
        string Output;
        P4Output(out Output, "files -m 10 " + CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "..."));
 
        var Lines = Output.Split(new string[] { Environment.NewLine }, StringSplitOptions.RemoveEmptyEntries);
        string SlashRoot = CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "/");
        string LocalRoot = CombinePaths(CmdEnv.LocalRoot, @"\");
        foreach (string Line in Lines)
        {
            if (Line.Contains(" - ") && Line.Contains("#"))
            {
                string depot = Line.Substring(0, Line.IndexOf("#"));
                if (!depot.Contains(SlashRoot))
                {
                    throw new AutomationException("{0} does not contain {1} and it is supposed to.", depot, P4Env.BuildRootP4);
                }
                string local = CombinePaths(depot.Replace(SlashRoot, LocalRoot));
                Log("{0}", depot);
                Log("    {0}", local);
            }
        }

        // should be used as a sanity check! make sure there are no files!
        LogP4Output(out Output, "files -m 10 " + CombinePaths(PathSeparator.Slash, P4Env.BuildRootP4, "...", "NoRedist", "..."));

        // caution this doesn't actually use the client _view_ from the template at all!
        // if you want that sync -f -k /...
        // then use client syntax in the files command instead
        // don't think we care, we don't rely on licensee _views_

        foreach (var ToLoad in VarsToSave)
        {
            SetEnvVar(ToLoad, KeyValues[ToLoad]);
        }
        DeleteClient(TestClient);
    }
}

class TestAgent : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("*********************** Agent Test, sign");

		List<string> Sign = new List<string>();
		Sign.Add(CombinePaths(CmdEnv.LocalRoot, @"\Engine\Binaries\DotNET\AgentInterface.dll"));

		CodeSign.SignMultipleIfEXEOrDLL(this, Sign);


		Log("*********************** Agent Test, P: drive");

		string UnitTestFile = @"p:\Builds\Fortnite\Test_p_drive.txt";
		Log("Test file {0}", UnitTestFile);

		if (FileExists(UnitTestFile))
		{
			DeleteFile(UnitTestFile);
		}
		string[] LinesToWrite = new string[]
		{
			"This is a test",
			"Of writing to file " + UnitTestFile
		};
		foreach (string Line in LinesToWrite)
		{
			WriteToFile(UnitTestFile, Line);
		}

		string[] LinesRead = ReadAllLines(UnitTestFile);
		DeleteFile(UnitTestFile);
		if (LinesRead == null || LinesRead.Length != LinesToWrite.Length)
		{
			throw new AutomationException("Contents of the file created is different to the file read.");
		}
		for (int LineIndex = 0; LineIndex < LinesRead.Length; ++LineIndex)
		{
			if (LinesRead[LineIndex] != LinesToWrite[LineIndex])
			{
				throw new AutomationException("Contents of the file created is different to the file read.");
			}
		}

		Log("*********************** Agent Test, Mount.exe");

		if (!FileExists(CmdEnv.MountExe))
		{
			throw new AutomationException("Mount.exe is missing from the agent.  Please install it.");
		}
	}
}

class CleanDDC : BuildCommand
{
    public override void ExecuteBuild()
    {
        Log("*********************** Clean DDC");

        bool DoIt = ParseParam("DoIt");

        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 10; j++)
            {
                for (int k = 0; k < 10; k++)
                {
                    string Dir = CombinePaths(String.Format(@"P:\UE4DDC\{0}\{1}\{2}", i, j, k));
                    if (!DirectoryExists_NoExceptions(Dir))
                    {
                        throw new AutomationException("Missing DDC dir {0}", Dir);
                    }
                    var files = FindFiles_NoExceptions("*.*", false, Dir);
                    foreach (var file in files)
                    {
                        if (FileExists_NoExceptions(file))
                        {
                            FileInfo Info = new FileInfo(file);
                            Log("{0}", file);
                            if ((DateTime.UtcNow - Info.LastWriteTimeUtc).TotalDays > 20)
                            {
                                Log("  is old.");
                                if (DoIt)
                                {
                                    DeleteFile_NoExceptions(file);
                                }
                            }
                            else
                            {
                                Log("  is NOT old.");
                            }
                        }
                    }
                }
            }
        }
    }

}

class TestTestFarm : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("*********************** TestTestFarm");

		string Exe = CombinePaths(CmdEnv.LocalRoot, "Engine", "Binaries", "Win64", "UE4Editor.exe");
		string ClientLogFile = CombinePaths(CmdEnv.LogFolder, "HoverGameRun");
		string CmdLine = " ../../../Samples/HoverShip/HoverShip.uproject -game -forcelogflush -log -abslog=" + ClientLogFile;

		ProcessResult App = Run(Exe, CmdLine, null, ERunOptions.AllowSpew | ERunOptions.NoWaitForExit | ERunOptions.AppMustExist | ERunOptions.NoStdOutRedirect);

		LogFileReaderProcess(ClientLogFile, App, (string Output) =>
		{
			if (!String.IsNullOrEmpty(Output) &&
					Output.Contains("Game Engine Initialized"))
			{
				Log("It looks started, let me kill it....");
				App.StopProcess();
			}
			return true; //keep reading
		});
	}
}

[Help("Test command line argument parsing functions.")]
class TestArguments : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("List of provided arguments: ");
		for (int Index = 0; Index < Params.Length; ++Index)
		{
			Log("{0}={1}", Index, Params[Index].ToString());
		}

		object[] TestArgs = new object[] { "NoXGE", "SomeArg", "Map=AwesomeMap", "run=whatever", "Stuff" };

		if (!ParseParam(TestArgs, "noxge"))
		{
			throw new AutomationException("ParseParam test failed.");
		}

		if (ParseParam(TestArgs, "Dude"))
		{
			throw new AutomationException("ParseParam test failed.");
		}

		if (!ParseParam(TestArgs, "Stuff"))
		{
			throw new AutomationException("ParseParam test failed.");
		}

		string Val = ParseParamValue(TestArgs, "map");
		if (Val != "AwesomeMap")
		{
			throw new AutomationException("ParseParamValue test failed.");
		}

		Val = ParseParamValue(TestArgs, "run");
		if (Val != "whatever")
		{
			throw new AutomationException("ParseParamValue test failed.");
		}
	}
}

[Help("Checks if combine paths works as excpected")]
public class TestCombinePaths : BuildCommand
{
	public override void ExecuteBuild()
	{
		var Results = new string[]
		{
			CombinePaths("This/", "Is", "A", "Test"),
			CombinePaths("This", "Is", "A", "Test"),
			CombinePaths("This/", @"Is\A", "Test"),
			CombinePaths(@"This\Is", @"A\Test"),
			CombinePaths(@"This\Is\A\Test"),
			CombinePaths("This/", @"Is\", "A", null, "Test", String.Empty),
			CombinePaths(null, "This/", "Is", @"A\", "Test")
		};

		for (int Index = 0; Index < Results.Length; ++Index)
		{
			var CombinedPath = Results[Index];
			Log(CombinedPath);
			if (CombinedPath != Results[0])
			{
				throw new AutomationException("Path {0} does not match path 0: {1} (expected: {2})", Index, CombinedPath, Results[0]);
			}
		}
	}
}

[Help("Tests file utility functions.")]
[Help("file=Filename", "Filename to perform the tests on.")]
class TestFileUtility : BuildCommand
{
	public override void ExecuteBuild()
	{
		string[] DummyFilenames = new string[] { ParseParamValue("file", "") };
		DeleteFile(DummyFilenames);
	}
}

class TestLog : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("************************* ShooterGameRun");
		string MapToRun = ParseParamValue("map", "/game/maps/sanctuary");

		PushDir(CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/"));
		//		string GameServerExe = CombinePaths(CmdEnv.LocalRoot, @"ShooterGame/Binaries/Win64/ShooterGameServer.exe");
		//		string GameExe = CombinePaths(CmdEnv.LocalRoot, @"ShooterGame/Binaries/Win64/ShooterGame.exe");
		string EditorExe = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/UE4Editor.exe");

		string ServerLogFile = CombinePaths(CmdEnv.LogFolder, "Server.log");

		string ServerApp = EditorExe;
		string OtherArgs = String.Format("{0} -server -abslog={1} -FORCELOGFLUSH -log", MapToRun, ServerLogFile);
		string StartArgs = "ShooterGame ";

		string ServerCmdLine = StartArgs + OtherArgs;

		ProcessResult ServerProcess = Run(ServerApp, ServerCmdLine, null, ERunOptions.AllowSpew | ERunOptions.NoWaitForExit | ERunOptions.AppMustExist | ERunOptions.NoStdOutRedirect);

		LogFileReaderProcess(ServerLogFile, ServerProcess, (string Output) =>
		{
			if (String.IsNullOrEmpty(Output) == false)
			{
				Console.Write(Output);
			}
			return true; // keep reading
		});
	}
}

[Help("Tests P4 change filetype functionality.")]
[Help("File", "Binary file to change the filetype to writeable")]
[RequireP4]
class TestChangeFileType : BuildCommand
{
	public override void ExecuteBuild()
	{
		var Filename = ParseParamValue("File", "");
		if (FStat(Filename).Type == P4FileType.Binary)
		{
			ChangeFileType(Filename, P4FileAttributes.Writeable);
		}
	}
}

class TestGamePerf : BuildCommand
{
	[DllImport("advapi32.dll", SetLastError = true)]
	private static extern bool LogonUser(string UserName, string Domain, string Password, int LogonType, int LogonProvider, out SafeTokenHandle SafeToken);

	public override void ExecuteBuild()
	{
		Log("*********************** TestGamePerf");

		string UserName = "test.automation";
		string Password = "JJGfh9CX";
		string Domain = "epicgames.net";
		string FileName = string.Format("UStats_{0:MM-d-yyyy_hh-mm-ss-tt}.csv", DateTime.Now);
		string Exe = CombinePaths(CmdEnv.LocalRoot, "Engine", "Binaries", "Win64", "UE4Editor.exe");

		string ClientLogFile = "";
		string CmdLine = "";
		string UStatsPath = "";

		string LevelParam = ParseParamValue("level", "");

		#region Arguments
		switch (LevelParam)
		{
			case "PerfShooterGame_Santuary":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "ShooterGame.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "ShooterGame.uproject ") + CmdEnv.LocalRoot + @"/ShooterGame/Content/Maps/TestMaps/Sanctuary_PerfLoader.umap?game=ffa -game -novsync";
				break;

			case "PerfShooterGame_Highrise":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "ShooterGame.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "ShooterGame", "ShooterGame.uproject ") + CmdEnv.LocalRoot + @"/ShooterGame/Content/Maps/TestMaps/Highrise_PerfLoader.umap?game=ffa -game -novsync";
				break;

			case "PerfHoverShip":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "HoverShip.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "SampleGames", "HoverShip", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "HoverShip", "HoverShip.uproject ") + CmdEnv.LocalRoot + @"/Samples/HoverShip/Content/Maps/HoverShip_01.umap -game -novsync";
				break;

			case "PerfInfiltrator":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "Infiltrator.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Infiltrator", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Infiltrator", "Infiltrator.uproject ") + CmdEnv.LocalRoot + @"/Samples/Showcases/Infiltrator/Content/Maps/TestMaps/Visuals/VIS_ArtDemo_PerfLoader.umap -game -novsync";
				break;

			case "PerfElemental":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "Elemental.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Elemental", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Elemental", "Elemental.uproject ") + CmdEnv.LocalRoot + @"/Samples/Showcases/Elemental/Content/Misc/Automated_Perf/Elemental_PerfLoader.umap -game -novsync";
				break;

			case "PerfStrategyGame":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "StrategyGame.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "StrategyGame", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "StrategyGame", "StrategyGame.uproject ") + CmdEnv.LocalRoot + @"/StrategyGame/Content/Maps/TestMaps/TowerDefenseMap_PerfLoader.umap -game -novsync";
				break;

			case "PerfVehicleGame":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "VehicleGame.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "VehicleGame", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "VehicleGame", "VehicleGame.uproject ") + CmdEnv.LocalRoot + @"/VehicleGame/Content/Maps/TestMaps/VehicleTrack_PerfLoader.umap -game -novsync";
				break;

			case "PerfPlatformerGame":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "PlatformerGame.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "PlatformerGame", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "PlatformerGame", "PlatformerGame.uproject ") + CmdEnv.LocalRoot + @"/PlatformerGame/Content/Maps/Platformer_StreetSection.umap -game -novsync";
				break;

			case "PerfLanderGame":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "LanderGame.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "LanderGame", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "LanderGame", "LanderGame.uproject ") + CmdEnv.LocalRoot + @"/LanderGame/Content/Maps/sphereworld.umap -game -novsync";
				break;

			case "PerfDemoLets_DynamicLighting":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_DynamicLighting.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/BasicMaterials/Content/Maps/Performance/Demolet_DynamicLighting_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_BasicMaterials":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_BasicMaterials.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/BasicMaterials_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_DemoRoom":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_DemoRoom.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/DemoRoom_Effects_hallway_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_Reflections":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_Reflections.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/DemoLet_Reflections_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_PostProcessing":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_PostProcessing.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/Demolet_PostProcessing_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_Physics":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_Physics.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/Demolet_Physics_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_ShadowMaps":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_ShadowMaps.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/Demolet_CascadingShadowMaps_PerfLoader.umap -game -novsync";
				break;

			case "PerfDemoLets_Weather":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "DemoLets_Weather.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "DemoLets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "DemoLets", "DemoLets.uproject ") + CmdEnv.LocalRoot + @"/EffectsGallery/Content/Maps/Performance/dynamic_weather_selector_PerfLoader.umap -game -novsync";
				break;

			case "PerfRealisticRendering_RoomNight":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "RealisticRendering_RoomNight.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "RealisticRendering", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "RealisticRendering", "RealisticRendering.uproject ") + CmdEnv.LocalRoot + @"/RealisticRendering/Content/Maps/Performance/RoomNight_PerfLoader.umap -game -novsync";
				break;

			case "PerfRealisticRendering_NoLight":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "RealisticRendering_NoLight.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "RealisticRendering", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "RealisticRendering", "RealisticRendering.uproject ") + CmdEnv.LocalRoot + @"/RealisticRendering/Content/Maps/Performance/RoomNightNoLights_PerfLoader.umap -game -novsync";
				break;

			case "PerfRealisticRendering_Room":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "RealisticRendering_Room.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "RealisticRendering", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "RealisticRendering", "RealisticRendering.uproject ") + CmdEnv.LocalRoot + @"/RealisticRendering/Content/Maps/Performance/Room_PerfLoader.umap -game -novsync";
				break;

			case "PerfMorphTargets":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "MorphTargets.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "MorphTargets", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "MorphTargets", "MorphTargets.uproject ") + CmdEnv.LocalRoot + @"/MorphTargets/Content/Maps/Performance/MorphTargets_PerfLoader.umap -game -novsync";
				break;

			case "PerfMatinee":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "Matinee.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "PostProcessMatinee", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "PostProcessMatinee", "PostProcessMatinee.uproject ") + CmdEnv.LocalRoot + @"/PostProcessMatinee/Content/Maps/Performance/PostProcessMatinee_PerfLoader.umap -game -novsync";
				break;

			case "PerfLandScape":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "LandScape.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "Landscape_WorldMachine", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Landscape_WorldMachine", "Landscape_WorldMap.uproject ") + CmdEnv.LocalRoot + @"/Landscape_WorldMachine/Content/Maps/Performance/LandscapeMap_PerfLoader.umap -game -novsync";
				break;

			case "PerfLandScape_Moutain":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "LandScape_Moutain.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "Landscape", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Landscape", "Landscape.uproject ") + CmdEnv.LocalRoot + @"/Landscape/Content/Maps/Performance/MoutainRange_PerfLoader.umap -game -novsync";
				break;

			case "PerfMobile":
				ClientLogFile = CombinePaths(CmdEnv.LogFolder, "Mobile.txt");
				UStatsPath = CombinePaths(CmdEnv.LocalRoot, "Samples", "ShowCases", "Mobile", "Saved", "Profiling", "UnrealStats");
				CmdLine = CombinePaths(CmdEnv.LocalRoot, "Samples", "Showcases", "Mobile", "Mobile.uproject ") + CmdEnv.LocalRoot + @"/Mobile/Content/Maps/Performance/Testmap_PerfLoader.umap -game -novsync";
				break;

			case "PerfSampleGames":
				ClientLogFile = CmdEnv.LogFolder;
				break;

			default:
				break;
		}
		#endregion

		try
		{
			SafeTokenHandle SafeToken;

			int LogonInteractive = 2;
			int ProviderDefualt = 0;

			/*bool bLogonSuccess = */
			LogonUser(UserName, Domain, Password, LogonInteractive, ProviderDefualt, out SafeToken);

			using (SafeToken)
			{
				using (WindowsIdentity NewUser = new WindowsIdentity(SafeToken.DangerousGetHandle()))
				{
					using (WindowsImpersonationContext TestAccount = NewUser.Impersonate())
					{
						if (LevelParam == "PerfSampleGames")
						{

						}
						else
						{
							if (!File.Exists(ClientLogFile))
							{
								Log("Creating log file");

								File.Create(ClientLogFile).Close();

								Log(ClientLogFile);

								Log("Log file created");
							}

							RunAndLog(Exe, CmdLine, ClientLogFile);
						}

						DirectoryInfo[] UStatsDirectories = new DirectoryInfo(UStatsPath).GetDirectories();
						DirectoryInfo CurrentUStatsDirectory = UStatsDirectories[0];

						for (int i = 0; i < UStatsDirectories.Length; i++)
						{
							if (UStatsDirectories[i].LastWriteTime > CurrentUStatsDirectory.LastWriteTime)
							{
								CurrentUStatsDirectory = UStatsDirectories[i];
							}
						}

						FileInfo[] UStatsFilePaths = new DirectoryInfo(CurrentUStatsDirectory.FullName).GetFiles();
						FileInfo CurrentUStatsFilePath = UStatsFilePaths[0];

						for (int i = 0; i < UStatsFilePaths.Length; i++)
						{
							if (UStatsFilePaths[i].LastWriteTime > CurrentUStatsFilePath.LastWriteTime)
							{
								CurrentUStatsFilePath = UStatsFilePaths[i];
							}
						}
						try
						{
							string FrontEndExe = CombinePaths(CmdEnv.LocalRoot, "Engine", "Binaries", "Win64", "UnrealFrontend.exe");

							CmdLine = " -run=convert -infile=" + CurrentUStatsFilePath.FullName + @" -outfile=D:\" + FileName + " -statlist=GameThread+RenderThread+StatsThread+STAT_FrameTime+STAT_PhysicalAllocSize+STAT_VirtualAllocSize";

							RunAndLog(FrontEndExe, CmdLine, ClientLogFile);
						}

						catch (Exception Err)
						{
							Log(Err.Message);
						}
					}
				}
			}
		}

		catch (Exception Err)
		{
			Log(Err.Message);
		}
	}
}

public sealed class SafeTokenHandle : SafeHandleZeroOrMinusOneIsInvalid
{
	private SafeTokenHandle()
		: base(true)
	{

	}

	[DllImport("kernel32.dll")]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
	[SuppressUnmanagedCodeSecurity]
	[return: MarshalAs(UnmanagedType.Bool)]
	private static extern bool CloseHandle(IntPtr Handle);

	protected override bool ReleaseHandle()
	{
		return CloseHandle(handle);
	}
}

[Help("Tests if UE4Build properly copies all relevent UAT build products to the Binaries folder.")]
public class TestUATBuildProducts : BuildCommand
{
	public override void ExecuteBuild()
	{
		UE4Build Build = new UE4Build(this);
		Build.AddUATFilesToBuildProducts();

		Log("Build products:");
		foreach (var Product in Build.BuildProductFiles)
		{
			Log("  " + Product);
		}
	}
}

[Help("Tests enumeration of a directory on P:")]
public class TestChunkDir : BuildCommand
{
	public override void ExecuteBuild()
	{
		var StartTime = DateTime.UtcNow;
		var Files = CommandUtils.FindFiles("*.*", true, @"P:\Builds\Fortnite\CloudDir\Chunks\");
		var Duration = (DateTime.UtcNow - StartTime).TotalSeconds;
		Log("Found {0} files in {1}s with C#", Files.Length, Duration);

		UE4Build Build = new UE4Build(this);
		var Agenda = new UE4Build.BuildAgenda();

		UnrealTargetPlatform EditorPlatform = HostPlatform.Current.HostEditorPlatform;
		const UnrealTargetConfiguration EditorConfiguration = UnrealTargetConfiguration.Development;

		Agenda.AddTargets(new string[] { "BlankProgram" }, EditorPlatform, EditorConfiguration);

		Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: false);

		var Exe = CombinePaths(CmdEnv.LocalRoot, @"Engine/Binaries/Win64/", "BlankProgram.exe");

		RunAndLog(CmdEnv, Exe, "");


	}
}

[Help("Tests WatchdogTimer functionality. The correct result is to exit the application with ExitCode=1 after a few seconds.")]
public class TestWatchdogTimer : BuildCommand
{
	public override void ExecuteBuild()
	{
		Log("Starting the first timer (1s). This should not crash.");
		using (var SafeTimer = new WatchdogTimer(1))
		{
			// Wait 500ms
			Thread.Sleep(500);
		}
		Log("First timer disposed successfully.");

		Log("Starting the second timer (2s). This should crash after 2 seconds.");
		using (var CrashTimer = new WatchdogTimer(2))
		{
			// Wait 5s (this will trigger the watchdog timer)
			Thread.Sleep(5000);
		}
	}
}

class TestOSSCommands : BuildCommand
{
	public override void ExecuteBuild()
	{
		string Exe = CombinePaths(CmdEnv.LocalRoot, "Engine", "Binaries", "Win64", "UE4Editor.exe");
		string CmdLine = "qagame automation-osscommands -game -log -logcmds=" + "\"" + "global none, logonline verbose, loghttp log, LogBlueprintUserMessages log" + "\"";
		string ClientLogFile = CombinePaths(CmdEnv.LogFolder, String.Format("QALog_{0:yyyy-MM-dd_hh-mm-ss-tt}.txt", DateTime.Now));
		string LogDirectory = CombinePaths(CmdEnv.LocalRoot, "Saved", "Logs");

		if (!File.Exists(ClientLogFile))
		{
			File.Create(ClientLogFile).Close();
		}

		try
		{
			RunAndLog(Exe, CmdLine, ClientLogFile);
		}
		catch (Exception Err)
		{
			Log(Err.Message);
		}

		FileInfo[] LogFiles = new DirectoryInfo(LogDirectory).GetFiles();
		FileInfo QALogFile = LogFiles[0];

		for (int i = 0; i < LogFiles.Length; i++)
		{
			if (LogFiles[i].LastWriteTime > QALogFile.LastWriteTime)
			{
				QALogFile = LogFiles[i];
			}
		}

		string[] Lines = File.ReadAllLines(QALogFile.FullName);

		StreamWriter WriteErrors = new StreamWriter(new FileStream(ClientLogFile, FileMode.Append, FileAccess.Write, FileShare.ReadWrite));

		# region Error List
		/* 
         * online sub=amazon test friends - Warning: Failed to get friends interface for amazon
         * online sub=amazon test identity - Error: RegisterUser() : OnlineSubsystemAmazon is improperly configured in DefaultEngine.ini
         * ErrorCode 00002EE7 - http test <iterations> <url> - Desc: The server name or address could not be resolved
         * Error/code 404 - online sub=mcp test validateaccount <email> <validation code> - Warning: MCP: Mcp validate Epic account request failed. Invalid response. Not found.
         * Error/code 404 - online sub=mcp test deleteaccount <email> <password> - Warning: MCP: Mcp delete Epic account request failed. Not found.
         * online sub=mcp test friends - Warning: Failed to get friends interface for mcp
         * bSucess: 0 - online sub=mcp test sessionhost - LogOnline:Verbose: OnCreateSessionComplete Game bSuccess: 0
         * Code 401 - online sub=mcp test time - Failed to query server time
         * Error code 500 - online sub=mcp test entitlements - profile template not found
        */
		#endregion

		using (WriteErrors)
		{
			for (int i = 0; i < Lines.Length; i++)
			{
				if (Lines[i].Contains("error=404") || Lines[i].Contains("code=404"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
				else if (Lines[i].Contains("code=401"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
				else if (Lines[i].Contains("error=500"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
				else if (Lines[i].Contains("bSucess: 0"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
				else if (Lines[i].Contains("Warning: Failed to get friends interface"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
				else if (Lines[i].Contains("Error: RegisterUser() : OnlineSubsystemAmazon is improperly configured"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
				else if (Lines[i].Contains("ErrorCode 00002EE7"))
				{
					WriteErrors.WriteLine(Lines[i]);
				}
			}
		}

	}
}

[Help("Builds a project using UBT. Syntax is similar to UBT with the exception of '-', i.e. UBT -QAGame -Development -Win32")]
[Help("NoXGE", "Disable XGE")]
[Help("Clean", "Clean build products before building")]
public class UBT : BuildCommand
{
	public override void ExecuteBuild()
	{
		var Build = new UE4Build(this);
		var Agenda = new UE4Build.BuildAgenda();
		var Platform = UnrealBuildTool.UnrealTargetPlatform.Win64;
		var Configuration = UnrealBuildTool.UnrealTargetConfiguration.Development;
		var Targets = new List<string>();

		foreach (var ObjParam in Params)
		{
			var Param = (string)ObjParam;
			UnrealBuildTool.UnrealTargetPlatform ParsePlatform;
			if (Enum.TryParse<UnrealBuildTool.UnrealTargetPlatform>(Param, true, out ParsePlatform))
			{
				Platform = ParsePlatform;
				continue;
			}
			UnrealBuildTool.UnrealTargetConfiguration ParseConfiguration;
			if (Enum.TryParse<UnrealBuildTool.UnrealTargetConfiguration>(Param, true, out ParseConfiguration))
			{
				Configuration = ParseConfiguration;
				continue;
			}
			if (String.Compare("NoXGE", Param, true) != 0 && String.Compare("Clean", Param, true) != 0)
			{
				Targets.Add(Param);
			}
		}

		var Clean = ParseParam("Clean");

		Agenda.AddTargets(Targets.ToArray(), Platform, Configuration);

		Log("UBT Buid");
		Log("Targets={0}", String.Join(",", Targets));
		Log("Platform={0}", Platform);
		Log("Configuration={0}", Configuration);
		Log("Clean={0}", Clean);

		Build.Build(Agenda, InUpdateVersionFiles: false);

		Log("UBT Completed");
	}
}
[Help("Zeroes engine versions in ObjectVersion.cpp and Version.h and checks them in.")]
[RequireP4]
public class ZeroEngineVersions : BuildCommand
{
	public override void ExecuteBuild()
	{
		string ObjectVersionFilename = CmdEnv.LocalRoot + @"/Engine/Source/Runtime/Core/Private/UObject/ObjectVersion.cpp";
		string VersionFilename = CmdEnv.LocalRoot + @"/Engine/Source/Runtime/Launch/Resources/Version.h";
		if (P4Env.Changelist > 0)
		{
			var Stat = FStat(ObjectVersionFilename);
			if (Stat.IsValid && Stat.Action != P4Action.None)
			{
				Log("Reverting {0}", ObjectVersionFilename);
				Revert(ObjectVersionFilename);
			}
			Stat = FStat(VersionFilename);
			if (Stat.IsValid && Stat.Action != P4Action.None)
			{
				Log("Reverting {0}", VersionFilename);
				Revert(VersionFilename);
			}

			Log("Gettting engine version files @{0}", P4Env.Changelist);
			Sync(String.Format("-f {0}@{1}", ObjectVersionFilename, P4Env.Changelist));
			Sync(String.Format("-f {0}@{1}", VersionFilename, P4Env.Changelist));
		}

		Log("Checking if engine version files need to be reset...");
		List<string> FilesToSubmit = new List<string>();
		{
			VersionFileUpdater ObjectVersionCpp = new VersionFileUpdater(ObjectVersionFilename);
			if (ObjectVersionCpp.Contains("#define	ENGINE_VERSION	0") == false)
			{
				Log("Zeroing out engine versions in {0}", ObjectVersionFilename);
				ObjectVersionCpp.ReplaceLine("#define	ENGINE_VERSION	", "0");
                ObjectVersionCpp.Commit();
				FilesToSubmit.Add(ObjectVersionFilename);
			}
		}
		{
			VersionFileUpdater VersionH = new VersionFileUpdater(VersionFilename);
			if (VersionH.Contains("#define ENGINE_VERSION 0") == false)
			{
				Log("Zeroing out engine versions in {0}", VersionFilename);
				VersionH.ReplaceLine("#define ENGINE_VERSION ", "0");
				VersionH.ReplaceLine("#define ENGINE_VERSION_HIWORD ", "0");
				VersionH.ReplaceLine("#define ENGINE_VERSION_LOWORD ", "0");
				VersionH.ReplaceLine("#define BRANCH_NAME ", "\"" + P4Env.BranchName + "\"");
				VersionH.ReplaceLine("#define BUILT_FROM_CHANGELIST ", "0");

                VersionH.Commit();
				FilesToSubmit.Add(VersionFilename);
			}
		}

		if (FilesToSubmit.Count > 0)
		{
			int CL = CreateChange(null, "Zero engine versions");
			foreach (var Filename in FilesToSubmit)
			{
				Edit(CL, Filename);
			}
			Log("Submitting CL #{0}...", CL);
			int SubmittedCL;
			Submit(CL, out SubmittedCL, false, true);
			Log("CL #{0} submitted as {1}", CL, SubmittedCL);
		}
		else
		{
			Log("Engine version files are set to 0.");
		}
	}
}

[Help("Helper command to sync only source code + engine files from P4.")]
[Help("Branch", "Optional branch depot path, default is: -Branch=//depot/UE4")]
[Help("CL", "Optional changelist to sync (default is latest), -CL=1234567")]
[Help("Sync", "Optional list of branch subforlders to always sync separated with '+', default is: -Sync=Engine/Binaries/ThirdParty+Engine/Content")]
[Help("Exclude", "Optional list of folder names to exclude from syncing, separated with '+', default is: -Exclude=Binaries+Content+Documentation+DataTables")]
[RequireP4]
public class SyncSource : BuildCommand
{
	private void SafeSync(string SyncCmdLine)
	{
		try
		{
			Sync(SyncCmdLine);
		}
		catch (Exception Ex)
		{
			LogError("Unable to sync {0}", SyncCmdLine);
			Log(System.Diagnostics.TraceEventType.Error, Ex);
		}
	}

	public override void ExecuteBuild()
	{
		var Changelist = ParseParamValue("cl", "");
		if (!String.IsNullOrEmpty(Changelist))
		{
			Changelist = "@" + Changelist;
		}

		var AlwaysSync = new List<string>(new string[]
			{
				"Engine/Binaries/ThirdParty",
				"Engine/Content",
			}
		);
		var AdditionalAlwaysSyncPaths = ParseParamValue("sync");
		if (!String.IsNullOrEmpty(AdditionalAlwaysSyncPaths))
		{
			var AdditionalPaths = AdditionalAlwaysSyncPaths.Split(new string[] { "+" }, StringSplitOptions.RemoveEmptyEntries);
			AlwaysSync.AddRange(AdditionalPaths);
		}

		var ExclusionList = new HashSet<string>(new string[]
			{
				"Binaries",
				"Content",
				"Documentation",
				"DataTables",
			},
			StringComparer.InvariantCultureIgnoreCase
		);
		var AdditionalExlusionPaths = ParseParamValue("exclude");
		if (!String.IsNullOrEmpty(AdditionalExlusionPaths))
		{
			var AdditionalPaths = AdditionalExlusionPaths.Split(new string[] { "+" }, StringSplitOptions.RemoveEmptyEntries);
			foreach (var Dir in AdditionalPaths)
			{
				ExclusionList.Add(Dir);
			}
		}

		var DepotPath = ParseParamValue("branch", "//depot/UE4/");
		foreach (var AlwaysSyncSubFolder in AlwaysSync)
		{
			var SyncCmdLine = CombinePaths(PathSeparator.Depot, DepotPath, AlwaysSyncSubFolder, "...") + Changelist;
			Log("Syncing {0}", SyncCmdLine, Changelist);
			SafeSync(SyncCmdLine);
		}

		DepotPath = CombinePaths(PathSeparator.Depot, DepotPath, "*");
		var ProjectDirectories = Dirs(String.Format("-D {0}", DepotPath));
		foreach (var ProjectDir in ProjectDirectories)
		{
			var ProjectDirPath = CombinePaths(PathSeparator.Depot, ProjectDir, "*");
			var SubDirectories = Dirs(ProjectDirPath);
			foreach (var SubDir in SubDirectories)
			{
				var SubDirName = Path.GetFileNameWithoutExtension(GetDirectoryName(SubDir));
				if (!ExclusionList.Contains(SubDirName))
				{
					var SyncCmdLine = CombinePaths(PathSeparator.Depot, SubDir, "...") + Changelist;
					Log("Syncing {0}", SyncCmdLine);
					SafeSync(SyncCmdLine);
				}
			}
		}
	}
}

[Help("Generates automation project based on a template.")]
[Help("project=ShortName", "Short name of the project, i.e. QAGame")]
[Help("path=FullPath", "Absolute path to the project root directory, i.e. C:\\UE4\\QAGame")]
public class GenerateAutomationProject : BuildCommand
{
	public override void ExecuteBuild()
	{
		var ProjectName = ParseParamValue("project");
		var ProjectPath = ParseParamValue("path");		

		{
			var CSProjFileTemplate = ReadAllText(CombinePaths(CmdEnv.LocalRoot, "Engine", "Extras", "UnsupportedTools", "AutomationTemplate", "AutomationTemplate.Automation.xml"));
			StringBuilder CSProjBuilder = new StringBuilder(CSProjFileTemplate);
			CSProjBuilder.Replace("AutomationTemplate", ProjectName);

			{
				const string ProjectGuidTag = "<ProjectGuid>";
				int ProjectGuidStart = CSProjFileTemplate.IndexOf(ProjectGuidTag) + ProjectGuidTag.Length;
				int ProjectGuidEnd = CSProjFileTemplate.IndexOf("</ProjectGuid>", ProjectGuidStart);
				string OldProjectGuid = CSProjFileTemplate.Substring(ProjectGuidStart, ProjectGuidEnd - ProjectGuidStart);
				string NewProjectGuid = Guid.NewGuid().ToString("B");
				CSProjBuilder.Replace(OldProjectGuid, NewProjectGuid);
			}

			{
				const string OutputPathTag = "<OutputPath>";
				var OutputPath = CombinePaths(CmdEnv.LocalRoot, "Engine", "Binaries", "DotNet", "AutomationScripts");
				int PathStart = CSProjFileTemplate.IndexOf(OutputPathTag) + OutputPathTag.Length;
				int PathEnd = CSProjFileTemplate.IndexOf("</OutputPath>", PathStart);
				string OldOutputPath = CSProjFileTemplate.Substring(PathStart, PathEnd - PathStart);
				string NewOutputPath = ConvertSeparators(PathSeparator.Slash, UnrealBuildTool.Utils.MakePathRelativeTo(OutputPath, ProjectPath)) + "/";
				CSProjBuilder.Replace(OldOutputPath, NewOutputPath);
			}

			if (!DirectoryExists(ProjectPath))
			{
				CreateDirectory(ProjectPath);
			}
			WriteAllText(CombinePaths(ProjectPath, ProjectName + ".Automation.csproj"), CSProjBuilder.ToString());
		}

		{
			var PropertiesFileTemplate = ReadAllText(CombinePaths(CmdEnv.LocalRoot, "Engine", "Extras", "UnsupportedTools", "AutomationTemplate", "Properties", "AssemblyInfo.cs"));
			StringBuilder PropertiesBuilder = new StringBuilder(PropertiesFileTemplate);
			PropertiesBuilder.Replace("AutomationTemplate", ProjectName);

			const string AssemblyGuidTag = "[assembly: Guid(\"";
			int AssemblyGuidStart = PropertiesFileTemplate.IndexOf(AssemblyGuidTag) + AssemblyGuidTag.Length;
			int AssemblyGuidEnd = PropertiesFileTemplate.IndexOf("\")]", AssemblyGuidStart);
			string OldAssemblyGuid = PropertiesFileTemplate.Substring(AssemblyGuidStart, AssemblyGuidEnd - AssemblyGuidStart);
			string NewAssemblyGuid = Guid.NewGuid().ToString("D");
			PropertiesBuilder.Replace(OldAssemblyGuid, NewAssemblyGuid);

			string PropertiesPath = CombinePaths(ProjectPath, "Properties");
			if (!DirectoryExists(PropertiesPath))
			{
				CreateDirectory(PropertiesPath);
			}
			WriteAllText(CombinePaths(PropertiesPath, "AssemblyInfo.cs"), PropertiesBuilder.ToString());
		}

	}
}

[Help("Sleeps for 20 seconds and then exits")]
public class DebugSleep : BuildCommand
{
	public override void ExecuteBuild()
	{
		Thread.Sleep(20000);
	}
}
