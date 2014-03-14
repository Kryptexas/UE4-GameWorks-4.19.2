// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.IO;
using System.Text.RegularExpressions;
using System.Threading;
using System.Reflection;
using System.Linq;
using System.Net.NetworkInformation;
using System.Collections;
using AutomationTool;
using UnrealBuildTool;

/// <summary>
/// Helper command to run a game.
/// </summary>
/// <remarks>
/// Uses the following command line params:
/// -cooked
/// -cookonthefly
/// -dedicatedserver
/// -win32
/// -noclient
/// -logwindow
/// </remarks>
public partial class Project : CommandUtils
{
	#region Fields

	/// <summary>
	/// Thread used to read client log file.
	/// </summary>
	private static Thread ClientLogReaderThread;

	#endregion

	#region Run Command

    // debug commands for the engine to crash
    public static string[] CrashCommands = 
    {
        "crash",
        "CHECK",
        "GPF",
        "ASSERT",
        "ENSURE",
        "RENDERCRASH",
        "RENDERCHECK",
        "RENDERGPF",
        "THREADCRASH",
        "THREADCHECK",
        "THREADGPF",
    };

	public static void Run(ProjectParams Params)
	{
		Params.ValidateAndLog();
		if (!Params.Run)
		{
			return;
		}

		var ServerLogFile = CombinePaths(CmdEnv.LogFolder, "Server.log");
		var ClientLogFile = CombinePaths(CmdEnv.LogFolder, Params.EditorTest ? "Editor.log" : "Client.log");

		// Setup server process if required.
		ProcessResult ServerProcess = null;
		if (Params.CookOnTheFly && !Params.SkipServer)
		{
			if (Params.ClientTargetPlatforms.Count > 0)
			{
				UnrealTargetPlatform ClientPlatform = Params.ClientTargetPlatforms[0];
				Platform ClientPlatformInst = Params.ClientTargetPlatformInstances[0];
				string TargetCook = ClientPlatformInst.GetCookPlatform( false, Params.HasDedicatedServerAndClient, Params.CookFlavor );
				ServerProcess = RunCookOnTheFlyServer(Params.RawProjectPath, Params.NoClient ? "" : ServerLogFile, TargetCook, Params.RunCommandline);
			}
			else
			{
				throw new AutomationException("Failed to run, client target platform not specified");
			}
		}
		else if (Params.DedicatedServer && !Params.SkipServer)
		{
			if (Params.ServerTargetPlatforms.Count > 0)
			{
				UnrealTargetPlatform ServerPlatform = Params.ServerTargetPlatforms[0];
				ServerProcess = RunDedicatedServer(Params, ServerLogFile, Params.RunCommandline);
				// With dedicated server, the client connects to local host to load a map.
				if (ServerPlatform == UnrealTargetPlatform.Linux)
				{
					Params.MapToRun = Params.ServerDeviceAddress;
				}
				else
				{
					Params.MapToRun = "127.0.0.1";
				}
			}
			else
			{
				throw new AutomationException("Failed to run, server target platform not specified");
			}
		}
		else if (Params.FileServer && !Params.SkipServer)
		{
			ServerProcess = RunFileServer(Params, ServerLogFile, Params.RunCommandline);
		}

		if (ServerProcess != null)
		{
			Log("Waiting a few seconds for the server to start...");
			Thread.Sleep(5000);
		}

		if (!Params.NoClient)
		{
			Log("Starting Client....");

			var SC = CreateDeploymentContext(Params, false);

			ERunOptions ClientRunFlags;
			string ClientApp;
			string ClientCmdLine;
			SetupClientParams(SC, Params, ClientLogFile, out ClientRunFlags, out ClientApp, out ClientCmdLine);

			// Run the client.
			if (ServerProcess != null)
			{
				RunClientWithServer(ServerLogFile, ServerProcess, ClientApp, ClientCmdLine, ClientRunFlags, ClientLogFile, Params);
			}
			else
			{
				RunStandaloneClient(SC, ClientLogFile, ClientRunFlags, ClientApp, ClientCmdLine, Params);
			}
		}
	}

	#endregion

	#region Client

	private static void RunStandaloneClient(List<DeploymentContext> DeployContextList, string ClientLogFile, ERunOptions ClientRunFlags, string ClientApp, string ClientCmdLine, ProjectParams Params)
	{
		if (Params.Unattended)
		{
            int Timeout = 60 * 30;
            if (Params.RunAutomationTests)
            {
                Timeout = 6 * 60 * 60;
            }
            using (var Watchdog = new WatchdogTimer(Timeout))
			{

                string AllClientOutput = "";
                ProcessResult ClientProcess = null;
				FileStream ClientProcessLog = null;
				StreamReader ClientLogReader = null;
				Log("Starting Client for unattended test....");
				ClientProcess = Run(ClientApp, ClientCmdLine + " -FORCELOGFLUSH", null, ClientRunFlags | ERunOptions.NoWaitForExit);
				while (!FileExists(ClientLogFile) && !ClientProcess.HasExited)
				{
					Log("Waiting for client logging process to start...{0}", ClientLogFile);
					Thread.Sleep(2000);
				}
				if (!ClientProcess.HasExited)
				{
					Thread.Sleep(2000);
					Log("Client logging process started...{0}", ClientLogFile);
					ClientProcessLog = File.Open(ClientLogFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
					ClientLogReader = new StreamReader(ClientProcessLog);
				}
				if (ClientProcess.HasExited || ClientLogReader == null)
				{
					throw new AutomationException("Client exited before we asked it to.");
				}
				bool bKeepReading = true;
				bool WelcomedCorrectly = false;
				while (!ClientProcess.HasExited && bKeepReading)
				{
					while (!ClientLogReader.EndOfStream && bKeepReading && !ClientProcess.HasExited)
					{
						string ClientOutput = ClientLogReader.ReadToEnd();
						if (!String.IsNullOrEmpty(ClientOutput))
						{
                            AllClientOutput += ClientOutput;
							Console.Write(ClientOutput);

                            string LookFor = "Bringing up level for play took";
                            if (Params.RunAutomationTest != "")
                            {
                                LookFor = "Automation Test Succeeded";
                            }
                            else if (Params.RunAutomationTests)
                            {
                                LookFor = "Automation Test Queue Empty";
                            }
                            else if (Params.EditorTest)
                            {
                                LookFor = "Asset discovery search completed in";
                            }

                            if (AllClientOutput.Contains(LookFor))
							{
								Log("Client loaded, lets wait 30 seconds...");
								Thread.Sleep(30000);
								if (!ClientProcess.HasExited)
								{
									WelcomedCorrectly = true;
								}
								Log("Test complete...");
								bKeepReading = false;
							}
                            else if (Params.RunAutomationTests)
                            {
                                int FailIndex = AllClientOutput.IndexOf("Automation Test Failed");
                                int ParenIndex = AllClientOutput.LastIndexOf(")");
                                if (FailIndex >= 0 && ParenIndex > FailIndex)
                                {
                                    string Tail = AllClientOutput.Substring(FailIndex);
                                    int CloseParenIndex = Tail.IndexOf(")");
                                    int OpenParenIndex = Tail.IndexOf("(");
                                    string Test = "";
                                    if (OpenParenIndex >= 0 && CloseParenIndex > OpenParenIndex)
                                    {
                                        Test = Tail.Substring(OpenParenIndex + 1, CloseParenIndex - OpenParenIndex - 1);
                                    }
#if true
                                    Log("Stopping client...");
                                    ClientProcess.StopProcess();
                                    if (IsBuildMachine)
                                    {
                                        Thread.Sleep(70000);
                                    }
#endif
                                    throw new AutomationException("Automated test failed ({0}).", Test);
                                }
                            }
                        }
					}
				}
				if (ClientProcess != null && !ClientProcess.HasExited)
				{
					Log("Stopping client...");
					ClientProcess.StopProcess();
				}

                // this is hack that prevents a hang
                if (IsBuildMachine)
                {
                    Thread.Sleep(70000);
                }
				if (!WelcomedCorrectly)
				{
					throw new AutomationException("Client exited before we asked it to.");
				}
			}
		}
		else
		{
			var SC = DeployContextList[0];
			ProcessResult ClientProcess = SC.StageTargetPlatform.RunClient(ClientRunFlags, ClientApp, ClientCmdLine, Params);
			if (ClientProcess != null)
			{
				// If the client runs without StdOut redirect we're going to read the log output directly from log file on
				// a separate thread.
				if ((ClientRunFlags & ERunOptions.NoStdOutRedirect) == ERunOptions.NoStdOutRedirect)
				{
					ClientLogReaderThread = new System.Threading.Thread(ClientLogReaderProc);
					ClientLogReaderThread.Start(new object[] { ClientLogFile, ClientProcess });
				}

				do
				{
					Thread.Sleep(100);
				}
				while (ClientProcess.HasExited == false);
			}
		}
	}

	private static void RunClientWithServer(string ServerLogFile, ProcessResult ServerProcess, string ClientApp, string ClientCmdLine, ERunOptions ClientRunFlags, string ClientLogFile, ProjectParams Params)
	{
		ProcessResult ClientProcess = null;
		var OtherClients = new List<ProcessResult>();

		bool WelcomedCorrectly = false;
		int NumClients = Params.NumClients;
        string AllClientOutput = "";

        if (Params.Unattended)
		{
            int Timeout = 60 * 5;
            if (Params.FakeClient)
            {
                Timeout = 60 * 15;
            }
            if (Params.RunAutomationTests)
            {
                Timeout = 6 * 60 * 60;
            }
			using (var Watchdog = new WatchdogTimer(Timeout))
			{
				while (!FileExists(ServerLogFile) && !ServerProcess.HasExited)
				{
					Log("Waiting for logging process to start...");
					Thread.Sleep(2000);
				}
				Thread.Sleep(1000);

                string AllServerOutput = "";
                using (FileStream ProcessLog = File.Open(ServerLogFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
				{
					StreamReader LogReader = new StreamReader(ProcessLog);
					bool bKeepReading = true;

					FileStream ClientProcessLog = null;
					StreamReader ClientLogReader = null;

					// Read until the process has exited.
					while (!ServerProcess.HasExited && bKeepReading)
					{
						while (!LogReader.EndOfStream && bKeepReading && ClientProcess == null)
						{
							string Output = LogReader.ReadToEnd();
							if (!String.IsNullOrEmpty(Output))
							{
                                AllServerOutput += Output;
								if (ClientProcess == null &&
                                        (AllServerOutput.Contains("Game Engine Initialized") || AllServerOutput.Contains("Unreal Network File Server is ready")))
								{
									Log("Starting Client for unattended test....");
									ClientProcess = Run(ClientApp, ClientCmdLine + " -FORCELOGFLUSH", null, ClientRunFlags | ERunOptions.NoWaitForExit);
									//@todo no testing is done on these
									if (NumClients > 1 && NumClients < 9)
									{
										for (int i = 1; i < NumClients; i++)
										{
											Log("Starting Extra Client....");
											OtherClients.Add(Run(ClientApp, ClientCmdLine, null, ClientRunFlags | ERunOptions.NoWaitForExit));
										}
									}
									while (!FileExists(ClientLogFile) && !ClientProcess.HasExited)
									{
										Log("Waiting for client logging process to start...{0}", ClientLogFile);
										Thread.Sleep(2000);
									}
									if (!ClientProcess.HasExited)
									{
										Thread.Sleep(2000);
										Log("Client logging process started...{0}", ClientLogFile);
										ClientProcessLog = File.Open(ClientLogFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
										ClientLogReader = new StreamReader(ClientProcessLog);
									}
								}
								else if (ClientProcess == null && !ServerProcess.HasExited)
								{
									Log("Waiting for server to start....");
									Thread.Sleep(2000);
								}
								if (ClientProcess != null && ClientProcess.HasExited)
								{
									ServerProcess.StopProcess();
                                    if (IsBuildMachine)
                                    {
                                        Thread.Sleep(70000);
                                    }
									throw new AutomationException("Client exited before we asked it to.");
								}
							}
						}
						if (ClientLogReader != null)
						{
							if (ClientProcess.HasExited)
							{
								ServerProcess.StopProcess();
                                if (IsBuildMachine)
                                {
                                    Thread.Sleep(70000);
                                }
								throw new AutomationException("Client exited or closed the log before we asked it to.");
							}
							while (!ClientProcess.HasExited && !ServerProcess.HasExited && bKeepReading)
							{
								while (!ClientLogReader.EndOfStream && bKeepReading && !ServerProcess.HasExited && !ClientProcess.HasExited)
								{
									string ClientOutput = ClientLogReader.ReadToEnd();
									if (!String.IsNullOrEmpty(ClientOutput))
									{
                                        AllClientOutput += ClientOutput;
										Console.Write(ClientOutput);

                                        string LookFor = "Bringing up level for play took";
                                        if (Params.DedicatedServer)
                                        {
                                            LookFor = "Welcomed by server";
                                        }
                                        else if (Params.RunAutomationTest != "")
                                        {
                                            LookFor = "Automation Test Succeeded";
                                        }
                                        else if (Params.RunAutomationTests)
                                        {
                                            LookFor = "Automation Test Queue Empty";
                                        }
                                        if (AllClientOutput.Contains(LookFor))
										{
											if (Params.FakeClient)
											{
												Log("Welcomed by server or client loaded, lets wait ten minutes...");
												Thread.Sleep(60000 * 10);
											}
											else
											{
												Log("Welcomed by server or client loaded, lets wait 30 seconds...");
												Thread.Sleep(30000);
											}
											if (!ServerProcess.HasExited && !ClientProcess.HasExited)
											{
												WelcomedCorrectly = true;
											}
											if (!ServerProcess.HasExited)
											{
												Log("Test complete");
												if (ClientProcess != null && !ClientProcess.HasExited)
												{
													Log("Stopping client....");
													ClientProcess.StopProcess();
												}
												Log("Stopping server....");
												ServerProcess.StopProcess();
                                                if (IsBuildMachine)
                                                {
                                                    Thread.Sleep(70000);
                                                }
											}
											bKeepReading = false;
										}
                                        else if (Params.RunAutomationTests)
                                        {
                                            int FailIndex = AllClientOutput.IndexOf("Automation Test Failed");
                                            int ParenIndex = AllClientOutput.LastIndexOf(")");
                                            if (FailIndex >= 0 && ParenIndex > FailIndex)
                                            {
                                                string Tail = AllClientOutput.Substring(FailIndex);
                                                int CloseParenIndex = Tail.IndexOf(")");
                                                int OpenParenIndex = Tail.IndexOf("(");
                                                string Test = "";
                                                if (OpenParenIndex >= 0 && CloseParenIndex > OpenParenIndex)
                                                {
                                                    Test = Tail.Substring(OpenParenIndex + 1, CloseParenIndex - OpenParenIndex - 1);
                                                }
#if true
                                                if (!ServerProcess.HasExited)
                                                {
                                                    Log("Stopping server....");
                                                    ServerProcess.StopProcess();
                                                }
                                                if (ClientProcess != null && !ClientProcess.HasExited)
                                                {
                                                    Log("Stopping client....");
                                                    ClientProcess.StopProcess();
                                                }
                                                foreach (var OtherClient in OtherClients)
                                                {
                                                    if (OtherClient != null && !OtherClient.HasExited)
                                                    {
                                                        Log("Stopping client....");
                                                        OtherClient.StopProcess();
                                                    }
                                                }
                                                if (IsBuildMachine)
                                                {
                                                    Thread.Sleep(70000);
                                                }

#endif
                                                throw new AutomationException("Automated test failed ({0}).", Test);
                                            }
                                        }
									}
								}
							}
						}
					}
				}
			}
		}
		else
		{

			LogFileReaderProcess(ServerLogFile, ServerProcess, (string Output) =>
			{
				bool bKeepReading = true;
                if (ClientProcess == null && !String.IsNullOrEmpty(Output))
				{
                    AllClientOutput += Output;
                    if (ClientProcess == null && (AllClientOutput.Contains("Game Engine Initialized") || AllClientOutput.Contains("Unreal Network File Server is ready")))
                    {
					    Log("Starting Client....");
					    ClientProcess = Run(ClientApp, ClientCmdLine, null, ClientRunFlags | ERunOptions.NoWaitForExit);
					    if (NumClients > 1 && NumClients < 9)
					    {
						    for (int i = 1; i < NumClients; i++)
						    {
							    Log("Starting Extra Client....");
							    OtherClients.Add(Run(ClientApp, ClientCmdLine, null, ClientRunFlags | ERunOptions.NoWaitForExit));
						    }
					    }
                    }
				}
				else if (ClientProcess == null && !ServerProcess.HasExited)
				{
					Log("Waiting for server to start....");
					Thread.Sleep(2000);
				}

				if (String.IsNullOrEmpty(Output) == false)
				{
					Console.Write(Output);
				}

				if (ClientProcess != null && ClientProcess.HasExited)
				{

					Log("Client exited, stopping server....");
					ServerProcess.StopProcess();
					bKeepReading = false;
				}

				return bKeepReading; // Keep reading
			});
		}
		Log("Server exited....");
		if (ClientProcess != null && !ClientProcess.HasExited)
		{
			ClientProcess.StopProcess();
		}
		foreach (var OtherClient in OtherClients)
		{
			if (OtherClient != null && !OtherClient.HasExited)
			{
				OtherClient.StopProcess();
			}
		}
		if (Params.Unattended)
		{
            // this is a hack to prevent hangs on exit
            if (IsBuildMachine)
            {
                Thread.Sleep(70000);
            }
            if (!WelcomedCorrectly)
            {
			    throw new AutomationException("Server or client exited before we asked it to.");
            }
		}
	}

	private static void SetupClientParams(List<DeploymentContext> DeployContextList, ProjectParams Params, string ClientLogFile, out ERunOptions ClientRunFlags, out string ClientApp, out string ClientCmdLine)
	{
		if (Params.ClientTargetPlatforms.Count == 0)
		{
			throw new AutomationException("No ClientTargetPlatform set for SetupClientParams.");
		}

		//		var DeployContextList = CreateDeploymentContext(Params, false);

		if (DeployContextList.Count == 0)
		{
			throw new AutomationException("No DeployContextList for SetupClientParams.");
		}

		var SC = DeployContextList[0];

		// Get client app name and command line.
		ClientRunFlags = ERunOptions.AllowSpew | ERunOptions.AppMustExist;
		ClientApp = "";
		ClientCmdLine = "";
		string TempCmdLine = "";
		var PlatformName = Params.ClientTargetPlatforms[0].ToString();
		if (Params.Cook || Params.CookOnTheFly)
		{
			List<string> Exes = SC.StageTargetPlatform.GetExecutableNames(SC, true);
			ClientApp = Exes[0];
			if (SC.StageTargetPlatform.PlatformType != UnrealTargetPlatform.IOS)
			{
				TempCmdLine += SC.ProjectArgForCommandLines + " ";
			}
			TempCmdLine += Params.MapToRun + " ";

			if (Params.CookOnTheFly || Params.FileServer)
			{
				TempCmdLine += "-filehostip=";
				if (UnrealBuildTool.ExternalExecution.GetRuntimePlatform () == UnrealTargetPlatform.Mac)
				{
					NetworkInterface[] Interfaces = NetworkInterface.GetAllNetworkInterfaces ();
					foreach (NetworkInterface adapter in Interfaces)
					{
						if (adapter.NetworkInterfaceType != NetworkInterfaceType.Loopback)
						{
							IPInterfaceProperties IP = adapter.GetIPProperties ();
							for (int Index = 0; Index < IP.UnicastAddresses.Count; ++Index)
							{
								if (IP.UnicastAddresses [Index].IsDnsEligible && IP.UnicastAddresses [Index].Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
								{
									TempCmdLine += IP.UnicastAddresses[Index].Address.ToString();
									if (String.IsNullOrEmpty (Params.Port) == false)
									{
										TempCmdLine += ":";
										TempCmdLine += Params.Port;
									}
									TempCmdLine += "+";
								}
							}
						}
					}
				}
				else
				{
					NetworkInterface[] Interfaces = NetworkInterface.GetAllNetworkInterfaces ();
					foreach (NetworkInterface adapter in Interfaces)
					{
						if (adapter.OperationalStatus == OperationalStatus.Up)
						{
							IPInterfaceProperties IP = adapter.GetIPProperties();
							for (int Index = 0; Index < IP.UnicastAddresses.Count; ++Index)
							{
								if (IP.UnicastAddresses[Index].IsDnsEligible)
								{
									TempCmdLine += IP.UnicastAddresses[Index].Address.ToString();
									if (String.IsNullOrEmpty(Params.Port) == false)
									{
										TempCmdLine += ":";
										TempCmdLine += Params.Port;
									}
									TempCmdLine += "+";
								}
							}
						}
					}
				}
				TempCmdLine += "127.0.0.1";
				if (String.IsNullOrEmpty(Params.Port) == false)
				{
					TempCmdLine += ":";
					TempCmdLine += Params.Port;
				}
				TempCmdLine += " ";

				if (!Params.Stage)
				{
					TempCmdLine += "-streaming ";
				}
				else if (SC.StageTargetPlatform.PlatformType != UnrealTargetPlatform.IOS)
				{
					// per josh, allowcaching is deprecated/doesn't make sense for iOS.
					TempCmdLine += "-allowcaching ";
				}
			}
			else if (Params.Pak || Params.SignedPak)
			{
				if (Params.SignedPak)
				{
					TempCmdLine += "-signedpak ";
				}
				else
				{
					TempCmdLine += "-pak ";
				}
			}
			else if (!Params.Stage)
			{
				var SandboxPath = CombinePaths(SC.RuntimeProjectRootDir, "Saved/Sandboxes", "Cooked-" + SC.CookPlatform);
				if (!SC.StageTargetPlatform.LaunchViaUFE)
				{
					TempCmdLine += "-sandbox=" + CommandUtils.MakePathSafeToUseWithCommandLine(SandboxPath) + " ";
				}
				else
				{
					TempCmdLine += "-sandbox=\'" + SandboxPath + "\' ";
				}
			}
		}
		else
		{
			ClientApp = CombinePaths(CmdEnv.LocalRoot, "Engine/Binaries", PlatformName, "UE4Editor.exe");
            TempCmdLine += SC.ProjectArgForCommandLines + " ";
            if (!Params.EditorTest)
            {
                TempCmdLine += "-game " + Params.MapToRun + " ";
            }
		}
		if (Params.LogWindow)
		{
			// Without NoStdOutRedirect '-log' doesn't log anything to the window
			ClientRunFlags |= ERunOptions.NoStdOutRedirect;
			TempCmdLine += "-log ";
		}
		else
		{
			TempCmdLine += "-stdout ";
		}
		if (Params.Unattended)
		{
			TempCmdLine += "-unattended ";
		}
        if (IsBuildMachine)
        {
            TempCmdLine += "-buildmachine ";
        }
        if (Params.CrashIndex > 0)
        {
            int RealIndex = Params.CrashIndex - 1;
            if (RealIndex < 0 || RealIndex >= CrashCommands.Count())
            {
                throw new AutomationException("CrashIndex {0} is out of range...max={1}", Params.CrashIndex, CrashCommands.Count());
            }
            TempCmdLine += String.Format("-execcmds=\"debug {0}\" ", CrashCommands[RealIndex]);
        }
        else if (Params.RunAutomationTest != "")
        {
            TempCmdLine += "-execcmds=\"automation list, automation run " + Params.RunAutomationTest + "\" ";
        }
        else if (Params.RunAutomationTests)
        {
            TempCmdLine += "-execcmds=\"automation list, automation runall\" ";
        }
        if (!SC.StageTargetPlatform.LaunchViaUFE)
		{
			TempCmdLine += "-abslog=" + CommandUtils.MakePathSafeToUseWithCommandLine(ClientLogFile) + " ";	
		}
		if (SC.StageTargetPlatform.PlatformType != UnrealTargetPlatform.IOS)
		{
			TempCmdLine += "-Messaging -nomcp -Windowed ";
		}
		else
		{
			// skip arguments which don't make sense for iOS
			TempCmdLine += "-Messaging -nomcp ";
		}
		if (Params.NullRHI)
		{
			TempCmdLine += "-nullrhi ";
		}
        TempCmdLine += "-CrashForUAT ";
		TempCmdLine += Params.RunCommandline;

		// todo: move this into the platform
		if (SC.StageTargetPlatform.LaunchViaUFE)
		{
			ClientCmdLine = "-run=Launch ";
			ClientCmdLine += "-Device=" + Params.Device + " ";
			ClientCmdLine += "-Exe=" + ClientApp + " ";
			ClientCmdLine += "-Targetplatform=" + Params.ClientTargetPlatforms[0].ToString() + " ";
			ClientCmdLine += "-Params=\"" + TempCmdLine + "\"";
			ClientApp = CombinePaths(CmdEnv.LocalRoot, "Engine/Binaries/Win64/UnrealFrontend.exe");

			Log("Launching via UFE:");
			Log("\tClientCmdLine: " + ClientCmdLine + "");
		}
		else
		{
			ClientCmdLine = TempCmdLine;
		}
	}

	#endregion

	#region Client Thread

	private static void ClientLogReaderProc(object ArgsContainer)
	{
		var Args = ArgsContainer as object[];
		var ClientLogFile = (string)Args[0];
		var ClientProcess = (ProcessResult)Args[1];
		LogFileReaderProcess(ClientLogFile, ClientProcess, (string Output) =>
		{
			if (String.IsNullOrEmpty(Output) == false)
			{
				Log(Output);
			}
			return true;
		});
	}

	#endregion

	#region Servers

	private static ProcessResult RunDedicatedServer(ProjectParams Params, string ServerLogFile, string AdditionalCommandLine)
	{
		ProjectParams ServerParams = new ProjectParams(Params);
		ServerParams.Device = Params.ServerDevice;

		if (ServerParams.ServerTargetPlatforms.Count == 0)
		{
			throw new AutomationException("No ServerTargetPlatform set for RunDedicatedServer.");
		}

		var DeployContextList = CreateDeploymentContext(ServerParams, true);

		if (DeployContextList.Count == 0)
		{
			throw new AutomationException("No DeployContextList for RunDedicatedServer.");
		}

		var SC = DeployContextList[0];

		var ServerApp = CombinePaths(CmdEnv.LocalRoot, "Engine/Binaries/Win64/UE4Editor.exe");
		if (ServerParams.Cook)
		{
			List<string> Exes = SC.StageTargetPlatform.GetExecutableNames(SC);
			ServerApp = Exes[0];
		}
		var Args = ServerParams.Cook ? "" : (SC.ProjectArgForCommandLines + " ");
		Console.WriteLine(Params.ServerDeviceAddress);
		UnrealTargetPlatform ServerPlatform = ServerParams.ServerTargetPlatforms[0];
		if (ServerParams.Cook && ServerPlatform == UnrealTargetPlatform.Linux && !String.IsNullOrEmpty(ServerParams.ServerDeviceAddress))
		{
			ServerApp = @"C:\Windows\system32\cmd.exe";

			string plinkPath = CombinePaths(Environment.GetEnvironmentVariable("LINUX_ROOT"), "bin/PLINK.exe ");
			string exePath = CombinePaths(SC.ShortProjectName, "Binaries", ServerPlatform.ToString(), SC.ShortProjectName + "Server");
			if (ServerParams.ServerConfigsToBuild[0] != UnrealTargetConfiguration.Development)
			{
				exePath += "-" + ServerPlatform.ToString() + "-" + ServerParams.ServerConfigsToBuild[0].ToString();
			}
			exePath = CombinePaths("LinuxServer", exePath.ToLower()).Replace("\\", "/");
			Args = String.Format("/k {0} -batch -ssh -t -i {1} {2}@{3} {4} {5} {6} -server -Messaging", plinkPath, ServerParams.DevicePassword, ServerParams.DeviceUsername, ServerParams.ServerDeviceAddress, exePath, Args, ServerParams.MapToRun);
		}
		else
		{
			var Map = ServerParams.MapToRun;
			if (Params.RawProjectPath.Contains("FortniteGame"))
			{
				// hack, to work around a fortnite issue with not running right fromt he command line
				Map += "?game=zone";
			}
			if (Params.FakeClient)
			{
				Map += "?fake";
			}
			Args += String.Format("{0} -server -abslog={1}  -unattended -FORCELOGFLUSH -log -Messaging -nomcp", Map, CommandUtils.MakePathSafeToUseWithCommandLine(ServerLogFile));
		}

		if (ServerParams.Pak || ServerParams.SignedPak)
		{
			if (ServerParams.SignedPak)
			{
				Args += " -signedpak";
			}
			else
			{
				Args += " -pak";
			}
		}
        if (IsBuildMachine)
        {
            Args += " -buildmachine"; 
        }
        Args += " -CrashForUAT"; 
        Args += " " + AdditionalCommandLine;


		if (ServerParams.Cook && ServerPlatform == UnrealTargetPlatform.Linux && !String.IsNullOrEmpty(ServerParams.ServerDeviceAddress))
		{
			Args += String.Format(" 2>&1 > {0}", ServerLogFile);
		}

		PushDir(Path.GetDirectoryName(ServerApp));
		var Result = Run(ServerApp, Args, null, ERunOptions.AllowSpew | ERunOptions.NoWaitForExit | ERunOptions.AppMustExist | ERunOptions.NoStdOutRedirect);
		PopDir();

		return Result;
	}

	private static ProcessResult RunCookOnTheFlyServer(string ProjectName, string ServerLogFile, string TargetPlatform, string AdditionalCommandLine)
	{
		var ServerApp = HostPlatform.Current.GetUE4ExePath("UE4Editor.exe");
        var Args = String.Format("{0} -run=cook -targetplatform={1} -cookonthefly -abslog={2}  -unattended -CrashForUAT -FORCELOGFLUSH -log {3}",
			CommandUtils.MakePathSafeToUseWithCommandLine(ProjectName),
			TargetPlatform,
			CommandUtils.MakePathSafeToUseWithCommandLine(ServerLogFile),
			AdditionalCommandLine);
        if (IsBuildMachine)
        {
            Args += " -buildmachine";
        }
		// Run the server (Without NoStdOutRedirect -log doesn't log anything to the window)
		PushDir(Path.GetDirectoryName(ServerApp));
		var Result = Run(ServerApp, Args, null, ERunOptions.AllowSpew | ERunOptions.NoWaitForExit | ERunOptions.AppMustExist | ERunOptions.NoStdOutRedirect);
		PopDir();
		return Result;
	}

	private static ProcessResult RunFileServer(ProjectParams Params, string ServerLogFile, string AdditionalCommandLine)
	{
#if false
		// this section of code would provide UFS with a more accurate file mapping
		var SC = new StagingContext(Params, false);
		CreateStagingManifest(SC);
		MaybeConvertToLowerCase(Params, SC);
		var UnrealFileServerResponseFile = new List<string>();

		foreach (var Pair in SC.UFSStagingFiles)
		{
			string Src = Pair.Key;
			string Dest = Pair.Value;

			Dest = CombinePaths(PathSeparator.Slash, SC.UnrealFileServerInternalRoot, Dest);

			UnrealFileServerResponseFile.Add("\"" + Src + "\" \"" + Dest + "\"");
		}


		string UnrealFileServerResponseFileName = CombinePaths(CmdEnv.LogFolder, "UnrealFileServerList.txt");
		File.WriteAllLines(UnrealFileServerResponseFileName, UnrealFileServerResponseFile);
#endif
		var UnrealFileServerExe = HostPlatform.Current.GetUE4ExePath("UnrealFileServer.exe");

		Log("Running UnrealFileServer *******");
        var Args = String.Format("{0} -abslog={1} -unattended -CrashForUAT -FORCELOGFLUSH -log {2}",
						CommandUtils.MakePathSafeToUseWithCommandLine(Params.RawProjectPath),
						CommandUtils.MakePathSafeToUseWithCommandLine(ServerLogFile),
						AdditionalCommandLine);
        if (IsBuildMachine)
        {
            Args += " -buildmachine";
        }
        PushDir(Path.GetDirectoryName(UnrealFileServerExe));
		var Result = Run(UnrealFileServerExe, Args, null, ERunOptions.AllowSpew | ERunOptions.NoWaitForExit | ERunOptions.AppMustExist | ERunOptions.NoStdOutRedirect);
		PopDir();
		return Result;
	}

	#endregion
}
