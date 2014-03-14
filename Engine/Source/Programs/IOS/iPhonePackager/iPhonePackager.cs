/**
 * Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
 */

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using Ionic.Zlib;
using System.ComponentModel;

namespace iPhonePackager
{
	public partial class Program
	{
		static public int ReturnCode = 0;
		static string MainCommand = "";
		static string MainRPCCommand = "";
		static public string GameName = "";
		static private string GamePath = "";
		static public string GameConfiguration = "";
		static public string Architecture = "";

		static public SlowProgressDialog ProgressDialog = null;
		static public BackgroundWorker BGWorker = null;
		static public int ProgressIndex = 0;

		static public void UpdateStatus(string Line)
		{
			if (BGWorker != null)
			{
				int Percent = Math.Min(Math.Max(ProgressIndex, 0), 100);
				BGWorker.ReportProgress(Percent, Line);
			}
		}

		static public void Log(string Line)
		{
			UpdateStatus(Line);
			Console.WriteLine(Line);
		}

		static public void Log(string Line, params object [] Args)
		{
			Log(String.Format(Line, Args));
		}

		static public void Error( string Line )
		{
			if (Program.ReturnCode == 0)
			{
				Program.ReturnCode = 1;
			}
			
			Console.ForegroundColor = ConsoleColor.Red;
			Log("IPP ERROR: " + Line);

			Console.ResetColor();
		}

		static public void Error(string Line, params object[] Args)
		{
			Error(String.Format(Line, Args));
		}

		static public void Warning(string Line)
		{
			Console.ForegroundColor = ConsoleColor.Yellow;
			Log("IPP WARNING: " + Line);
			Console.ResetColor();
		}

		static public void Warning(string Line, params object[] Args)
		{
			Warning(String.Format(Line, Args));
		}

		static private bool ParseCommandLine(ref string[] Arguments)
		{
			if (Arguments.Length == 0)
			{
				Program.Warning("No arguments specified, assuming GUI mode");
				Arguments = new string[] { "gui" };
			}

			if (Arguments.Length == 1)
			{
				MainCommand = Arguments[0];
			}
			else if (Arguments.Length == 2)
			{
				MainCommand = Arguments[0];
				GamePath = Arguments[1];
			}
			else if (Arguments.Length >= 2)
			{
				MainCommand = Arguments[0];
				GamePath = Arguments[1];

				for (int ArgIndex = 2; ArgIndex < Arguments.Length; ArgIndex++)
				{
					string Arg = Arguments[ArgIndex].ToLowerInvariant();

					if (Arg.StartsWith("-"))
					{
						// Behavior switches
						switch (Arg)
						{
							case "-interactive":
								Config.bAllowInteractiveDialogsDuringNonInteractiveCommands = true;
								break;
							case "-verbose":
								Config.bVerbose = true;
								break;
							case "-strip":
								Config.bForceStripSymbols = true;
								break;
							case "-compress=best":
								Config.RecompressionSetting = (int)CompressionLevel.BestCompression;
								break;
							case "-compress=fast":
								Config.RecompressionSetting = (int)CompressionLevel.BestSpeed;
								break;
							case "-compress=none":
								Config.RecompressionSetting = (int)CompressionLevel.None;
								break;
							case "-distribution":
								Config.bForDistribution = true;
								break;
							case "-createstub":
								Config.bCreateStubSet = true;
								break;
							case "-sign":
								Config.bPerformResignWhenRepackaging = true;
								break;
							case "-cookonthefly":
								Config.bCookOnTheFly = true;
								break;
						}

						// get the stage dir path
						if (Arg == "-stagedir")
						{
							// make sure there's at least one more arg
							if (Arguments.Length > ArgIndex + 1)
							{
								Config.RepackageStagingDirectory = Arguments[++ArgIndex];
							}
							else
							{
								return false;
							}
						}
						else if (Arg == "-config")
						{
							// make sure there's at least one more arg
							if (Arguments.Length > ArgIndex + 1)
							{
								GameConfiguration = Arguments[++ArgIndex];
							}
							else
							{
								return false;
							}
						}
						// append a name to the bungle identifier and display name
						else if (Arg == "-bundlename")
						{
							if (Arguments.Length > ArgIndex + 1)
							{
								Config.OverrideBundleName = Arguments[++ArgIndex];
							}
							else
							{
								return false;
							}
						}
						else if (Arg == "-mac")
						{
							// make sure there's at least one more arg
							if (Arguments.Length > ArgIndex + 1)
							{
								Config.OverrideMacName = Arguments[++ArgIndex];
							}
							else
							{
								return false;
							}
						}
						else if (Arg == "-architecture" || Arg == "-arch")
						{
							// make sure there's at least one more arg
							if (Arguments.Length > ArgIndex + 1)
							{
								Architecture = "-" + Arguments[++ArgIndex];
							}
							else
							{
								return false;
							}
						}
					}
					else
					{
						// RPC command
						MainRPCCommand = Arguments[ArgIndex];
					}
				}
			}
	
			return ( true );
		}

		static private bool CheckArguments()
		{
			if (GameConfiguration.Length == 0)
			{
				GameConfiguration = "GameConfigurationWasNotSpecifiedToIPP";
			}

			if (GameName.Length == 0)
			{
				Error( "Invalid number of arguments" );
				return false;
			}

			if (Config.bCreateStubSet && Config.bForDistribution)
			{
				Error("-createstub and -distribution are mutually exclusive");
				return false;
			}

			// If -sign was specified, check to see if the user has configured yet.  If not, we will pop up a configuration dialog
			if (Config.bPerformResignWhenRepackaging && Config.bAllowInteractiveDialogsDuringNonInteractiveCommands)
			{
				string CWD = Directory.GetCurrentDirectory();
				if (!CodeSignatureBuilder.DoRequiredFilesExist())
				{
					StartVisuals();

					Form DisplayForm = new ToolsHub();
					Application.Run(DisplayForm);
					if (DisplayForm.DialogResult != DialogResult.OK)
					{
						Error("One or more files necessary for packaging are missing and configuration was cancelled");
						return false;
					}
				}
				Directory.SetCurrentDirectory(CWD);
			}

			return true;
		}

		static public void ExecuteCommand(string Command, string RPCCommand)
		{
			if( ReturnCode > 0 )
			{
				Warning( "Error in previous command; suppressing: " + Command + " " + RPCCommand ?? "" );
				return;
			}

			if (Config.bVerbose)
			{
				Log("");
				Log("----------");
				Log(String.Format("Executing command '{0}' {1}...", Command, (RPCCommand != null) ? ("'" + RPCCommand + "' ") : ""));
			}

			try
			{
				bool bHandledCommand = CookTime.ExecuteCookCommand(Command, RPCCommand);

				if (!bHandledCommand)
				{
					bHandledCommand = CompileTime.ExecuteCompileCommand(Command, RPCCommand);
				}

				if (!bHandledCommand)
				{
					bHandledCommand = DeploymentHelper.ExecuteDeployCommand(Command, GamePath, RPCCommand);
				}

				if (!bHandledCommand)
				{
					bHandledCommand = true;
					switch (Command)
					{
						case "configure":
							if (Config.bForDistribution)
							{
								Error("configure cannot be used with -distribution");
							}
							else
							{
								RunInVisualMode(delegate { return new ConfigureMobileGame(); });
							}
							break;

						default:
							bHandledCommand = false;
							break;
					}
				}

				if (!bHandledCommand)
				{
					Error("Unknown command");
					ReturnCode = 2;
				}
			}
			catch (Exception Ex)
			{
				Error( "Error while executing command: " + Ex.ToString() );
				ReturnCode = 200;
			}

			Console.WriteLine();
		}

		/**
		 * Assembly resolve method to pick correct StandaloneSymbolParser DLL
		 */
		static Assembly CurrentDomain_AssemblyResolve( Object sender, ResolveEventArgs args )
		{
			// Name is fully qualified assembly definition - e.g. "p4dn, Version=1.0.0.0, Culture=neutral, PublicKeyToken=ff968dc1933aba6f"
			string[] AssemblyInfo = args.Name.Split( ",".ToCharArray() );
			string AssemblyName = AssemblyInfo[0];

			if (AssemblyName.ToLowerInvariant() == "unrealcontrols")
			{
				AssemblyName = Path.GetFullPath(@"..\" + AssemblyName + ".dll");

				Debug.WriteLineIf(System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName);

				if (File.Exists(AssemblyName))
				{
					Assembly SymParser = Assembly.LoadFile(AssemblyName);
					return SymParser;
				}
			}
			else if (AssemblyName.ToLowerInvariant() == "rpcutility")
			{
				AssemblyName = Path.GetFullPath(@"..\" + AssemblyName + ".exe");

				Debug.WriteLineIf(System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName);

				if (File.Exists(AssemblyName))
				{
					Assembly SymParser = Assembly.LoadFile(AssemblyName);
					return SymParser;
				}
			}
			else if (AssemblyName.ToLowerInvariant() == "bouncycastle.crypto")
			{
				AssemblyName = Path.GetFullPath(@"..\..\ThirdParty\IOS\" + AssemblyName + ".dll");

				Debug.WriteLineIf(System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName);

				if (File.Exists(AssemblyName))
				{
					Assembly A = Assembly.LoadFile(AssemblyName);
					return A;
				}
			}
			else if (AssemblyName.ToLowerInvariant() == "ionic.zip.reduced")
			{
				AssemblyName = Path.GetFullPath(@"..\..\ThirdParty\Ionic\" + AssemblyName + ".dll");

				Debug.WriteLineIf(System.Diagnostics.Debugger.IsAttached, "Loading assembly: " + AssemblyName);

				if (File.Exists(AssemblyName))
				{
					Assembly A = Assembly.LoadFile(AssemblyName);
					return A;
				}
			}

			return ( null );
		}

		delegate Form CreateFormDelegate();

		static void StartVisuals()
		{
			Application.EnableVisualStyles();
			Application.SetCompatibleTextRenderingDefault(false);

			ProgressDialog = new SlowProgressDialog();
		}

		static void RunInVisualMode(CreateFormDelegate Work)
		{
			StartVisuals();

			Form DisplayForm = Work.Invoke();
			Application.Run(DisplayForm);
		}

		/**
		 * Main control loop
		 */
		[STAThread]
		static int Main(string[] args)
		{
			// remember the working directory at start, as the game path could be relative to this path
			string InitialCurrentDirectory = Environment.CurrentDirectory;

			// set the working directory to the location of the application (so relative paths always work)
			Environment.CurrentDirectory = Path.GetDirectoryName(Application.ExecutablePath);

			AppDomain.CurrentDomain.AssemblyResolve += new ResolveEventHandler( CurrentDomain_AssemblyResolve );

			// A simple, top-level try-catch block
			try
			{
				if (!ParseCommandLine(ref args))
				{
					Log("Usage: iPhonePackager <Command> <GameName> [RPCCommand &| Switch]");
					Log("");
					Log("Common commands:");
					Log(" ... RepackageIPA GameName");
					Log(" ... PackageIPA GameName");
					Log(" ... PackageApp GameName");
					Log(" ... Deploy PathToIPA");
					Log(" ... RepackageFromStage GameName");
					Log("");
					Log("Configuration switches:");
					Log("	 -stagedir <path>		  sets the directory to copy staged files from (defaults to none)");
					Log("	 -compress=fast|best|none  packaging compression level (defaults to none)");
					Log("	 -strip					strip symbols during packaging");
					Log("	 -config				   game configuration (e.g., Shipping, Development, etc...)");
					Log("	 -distribution			 packaging for final distribution");
					Log("	 -createstub			   packaging stub IPA for later repackaging");
					Log("	 -mac <MacName>			overrides the machine to use for any Mac operations");
					Log("	 -arch <Architecture>	  sets the architecture to use (blank for default, -simulator for simulator builds)");
					Log("");
					Log("Commands: RPC, Clean");
					Log("  StageMacFiles, GetIPA, Deploy, Install, Uninstall");
					Log("");
					Log("RPC Commands: SetExec, InstallProvision, MakeApp, DeleteIPA, Copy, Kill, Strip, Zip, GenDSYM");
					Log("");
					Log("Sample commandlines:");
					Log(" ... iPhonePackager Deploy UDKGame Release");
					Log(" ... iPhonePackager RPC SwordGame Shipping MakeApp");
					return 1;
				}

				Log("Executing iPhonePackager " + String.Join(" ", args));

				// Ensure shipping configuration for final distributions
				if (Config.bForDistribution && (GameConfiguration != "Shipping"))
				{
					Program.Warning("Distribution builds should be made in the Shipping configuration!");
				}

				// process the GamePath (if could be ..\Samples\MyDemo\ or ..\Samples\MyDemo\MyDemo.uproject
				GameName = Path.GetFileNameWithoutExtension(GamePath);
				if (GameName.Equals("UE4", StringComparison.InvariantCultureIgnoreCase) || GameName.Equals("Engine", StringComparison.InvariantCultureIgnoreCase))
				{
					GameName = "UE4Game";
				}

				// setup configuration
				if (!Config.Initialize(InitialCurrentDirectory, GamePath))
				{
					return 1;
				}

				switch (MainCommand.ToLowerInvariant())
				{
					case "packageapp":
						if (CheckArguments())
						{
							if (Config.bCreateStubSet)
							{
								Error("packageapp cannot be used with the -createstub switch");
							}
							else
							{
								// Create the .app on the Mac
								CompileTime.CreateApplicationDirOnMac();
							}
						}
						break;

					case "repackagefromstage":
						if (CheckArguments())
						{
							if (Config.bCreateStubSet)
							{
								Error("repackagefromstage cannot be used with the -createstub switches");
							}
							else
							{
								bool bProbablyCreatedStub = Utilities.GetEnvironmentVariable("ue.IOSCreateStubIPA", true);
								if (!bProbablyCreatedStub)
								{
									Warning("ue.IOSCreateStubIPA is currently FALSE, which means you may be repackaging with an out of date stub IPA!");
								}

								CookTime.RepackageIPAFromStub();
							}
						}
						break;

					// this is the "super fast just move executable" mode for quick programmer iteration
					case "dangerouslyfast":
						if (CheckArguments())
						{
							CompileTime.DangerouslyFastMode();
						}
						break;

					case "packageipa":
						if (CheckArguments())
						{
							CompileTime.PackageIPAOnMac();
						}
						break;

					case "resigntool":
						RunInVisualMode(delegate { return new GraphicalResignTool(); });
						break;

					case "certrequest":
						RunInVisualMode(delegate { return new GenerateSigningRequestDialog(); });
						break;

					case "gui":
						RunInVisualMode(delegate { return ToolsHub.CreateShowingTools(); });
						break;

					default:
						// Commands by themself default to packaging for the device
						if (CheckArguments())
						{
							ExecuteCommand(MainCommand, MainRPCCommand);
						}
						break;
				}
			}
			catch (Exception Ex)
			{
				Error("Application exception: " + Ex.ToString());
				ReturnCode = 1;
			}
			finally
			{
				if (DeploymentHelper.DeploymentServerProcess != null)
				{
					DeploymentHelper.DeploymentServerProcess.Close();
				}
			}

			return ( ReturnCode );
		}
	}
}

