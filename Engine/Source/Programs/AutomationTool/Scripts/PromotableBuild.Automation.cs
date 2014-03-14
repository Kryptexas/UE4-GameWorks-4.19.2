// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;

[Help("Constructs a build, if perforce is enabled it will submit it to the version control server")]
[Help("LeanAndMean", "Toggle to construct a subset of the potential targets")]
[Help(typeof(UE4Build))]
[Help(typeof(CodeSign))]
class PromotableBuild : BuildCommand
{
	bool LeanAndMean;
	bool UseXGE;

	void ExecuteInner(int WorkingCL)
	{
		var UE4Build = new UE4Build(this);
		var Agenda = new UE4Build.BuildAgenda();

		var Config = ParseParamValue("Config");

		if (String.IsNullOrEmpty(Config) || Config.ToLower() == "editors")
		{
			Agenda.DotNetSolutions.AddRange(
				new string[] 
			    {
				    @"Engine\Source\Programs\UnrealDocTool\UnrealDocTool\UnrealDocTool.sln",
				    @"Engine\Source\Programs\NetworkProfiler\NetworkProfiler.sln",   
			    }
				);

			Agenda.SwarmProject = @"Engine\Source\Programs\UnrealSwarm\UnrealSwarm.sln";

			Agenda.DotNetProjects.AddRange(
				new string[] 
			    {
				    @"Engine\Source\Programs\DotNETCommon\DotNETUtilities\DotNETUtilities.csproj",
					@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportCommon\CrashReportCommon.csproj",
					@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportReceiver\CrashReportReceiver.csproj",
					@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportProcess\CrashReportProcess.csproj",
				    @"Engine\Source\Programs\CrashReporter\RegisterPII\RegisterPII.csproj",
				    @"Engine\Source\Programs\Distill\Distill.csproj",		       
                    @"Engine\Source\Programs\RPCUtility\RPCUtility.csproj",
                    @"Engine\Source\Programs\UnrealControls\UnrealControls.csproj",
			    }
				);

			Agenda.IOSDotNetProjects.AddRange(
					new string[]
                {
                    @"Engine\Source\Programs\IOS\iPhonePackager\iPhonePackager.csproj",
                    @"Engine\Source\Programs\IOS\MobileDeviceInterface\MobileDeviceInterface.csproj",
                    @"Engine\Source\Programs\IOS\DeploymentInterface\DeploymentInterface.csproj",
                }
					);

			Agenda.ExtraDotNetFiles.AddRange(
				new string[] 
			    {
				    "Interop.IWshRuntimeLibrary",
				    "UnrealMarkdown",
				    "CommonUnrealMarkdown",
			    }
				);
		}

		{
			if (LeanAndMean && UseXGE)
			{
				// this is minimal to test XGE
				var ProgramTargets = new string[] 
				{
					"UnrealHeaderTool",
				};
				Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);

				var Win64Targets = new string[] 
				{
					"FortniteEditor",
					"OrionEditor",
				};
				Agenda.AddTargets(Win64Targets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);

				var Win32Targets = new string[] 
				{
					"FortniteGame",
					"OrionGame",
				};
				Agenda.AddTargets(Win32Targets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development);

			}
			else if (LeanAndMean)
			{
				var ProgramTargets = new string[] 
				{
					"ShaderCompileWorker",
					"UnrealHeaderTool",
					"UnrealLightmass",
				};
				Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);

				var Win64DevTargets = new string[] 
				{
					"FortniteEditor",
					"QAGameEditor",
				};
				Agenda.AddTargets(Win64DevTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
			}
			else
			{
				//This needs to be a separate target for distributed building because it is required to build anything else.
				var UHTTarget = new string[]
                {
                    "UnrealHeaderTool",
                };
				Agenda.AddTargets(UHTTarget, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);

				var ProgramTargets = new string[] 
				{
					"UnrealFileServer",
					"ShaderCompileWorker",
					"MinidumpDiagnostics",
					"SymbolDebugger",
					"UnrealFrontend",
					"UnrealLightmass",
					"UnrealPak",
					"CrashReportClient",
				};

				var MacProgramTargets = new string[] 
				{
					"UnrealFileServer",
					"ShaderCompileWorker",
					"UnrealFrontend",
					"UnrealHeaderTool",
					"UnrealLightmass",
					"UnrealPak",
					"CrashReportClient",
				};

				var Win64DevTargets = new List<string>
				{
					"FortniteEditor",
					"OrionEditor",
					"PlatformerGameEditor",                    
					"QAGameEditor",
					"ShooterGameEditor",
					"StrategyGameEditor",
					"VehicleGameEditor",
					"ShadowEditor",
                    "SoulEditor",
                    "StrategyVREditor",
				};

				var Win32Targets = new List<string> 
				{
					"FortniteGame",
                    "FortniteClient",
					"FortniteServer",                    
					"OrionGame",
					"PlatformerGame",
					"QAGame",
					"ShooterGame",
					"StrategyGame",
					"VehicleGame",
					"Shadow",
                    "Soul",
                    "StrategyVR",
				};

				var Win32ShipTargets = new List<string> 
				{
					"FortniteGame",
                    "FortniteClient",
					"FortniteServer",                    
				};

				var PS4DevTargets = new List<string>
				{
					"QAGame",
					"UE4Game",
					"FortniteGame",
				};

				var XboxOneDevTargets = new List<string>
				{
					"QAGame",
					"UE4Game",
					"FortniteGame",
				};

				var IOSDevTargets = new List<string>
				{
					"QAGame",
					"UE4Game",
					"Shadow",
					"PlatformerGame",
					"StrategyGame",
					"Soul",
				};

				var MacDevTargets = new List<string>
                {
                    "QAGameEditor",
                };

                var AndroidDevTargets = new List<string>
                {
                    "UE4Game",
                    "Soul",
                };

				//Check to see if we should Exclude any projects.  We would want to do this for branches that do not have all of the projects
				var Excludes = ParseParamValue("Exclude");

				if (!String.IsNullOrEmpty(Excludes))
				{
					List<List<string>> TargetLists = new List<List<string>>
                    {
                       Win32Targets,
                       Win64DevTargets,
                       Win32ShipTargets,
                       PS4DevTargets,
                       XboxOneDevTargets,
                       IOSDevTargets,
                       MacDevTargets,
                       AndroidDevTargets,
                    };

					List<string> Samples = new List<string>
                    {
                        "PlatformerGame",
                        "ShooterGame",
                        "StrategyGame",
                        "VehicleGame",                        
                        "PlatformerGameEditor",
                        "ShooterGameEditor",
                        "StrategyGameEditor",
                        "VehicleGameEditor",                        
                    };

					List<string> Orion = new List<string>
                    {
                        "OrionGame",
                        "OrionEditor",
                    };

					List<string> Fortnite = new List<string>
                    {
                        "FortniteServer",
                        "FortniteClient",
                        "FortniteGame",
                        "FortniteEditor",
                    };

					List<string> Shadow = new List<string>
                    {
                        "Shadow",
                        "ShadowEditor",
                    };

					List<string> Soul = new List<string>
                    {
                        "Soul",
                        "SoulEditor",
                    };

                    List<string> StrategyVR = new List<string>
                    {
                        "StrategyVR",
                        "StrategyVREditor",
                    };

					List<string> Exclude = new List<string>(Excludes.Split('+'));

					foreach (List<string> List in TargetLists)
					{
						if (Exclude.Contains("Samples"))
						{
							RemoveTargetsFromList(Samples, List);
						}
						if (Exclude.Contains("Orion"))
						{
							RemoveTargetsFromList(Orion, List);
						}
						if (Exclude.Contains("Fortnite"))
						{
							RemoveTargetsFromList(Fortnite, List);
						}
						if (Exclude.Contains("Shadow"))
						{
							RemoveTargetsFromList(Shadow, List);
						}
						if (Exclude.Contains("Soul"))
						{
							RemoveTargetsFromList(Soul, List);
						}
                        if (Exclude.Contains("StrategyVR"))
                        {
                            RemoveTargetsFromList(StrategyVR, List);
                        }
					}
				}

				// @todo: make this programmatic by looping over the TargetLists (which maybe should be a map from config to list)                
				string[] Win64Dev = Win64DevTargets.ToArray();
				string[] Win32Dev = Win32Targets.ToArray();
				string[] Win32Ship = Win32ShipTargets.ToArray();
				string[] PS4Dev = PS4DevTargets.ToArray();
				string[] XboxOneDev = XboxOneDevTargets.ToArray();
				string[] IOSDev = IOSDevTargets.ToArray();
				string[] MacDev = MacDevTargets.ToArray();
                string[] AndroidDev = AndroidDevTargets.ToArray();

				if (!String.IsNullOrEmpty(Config))
				{
					switch (Config.ToUpperInvariant())
					{
						case "EDITORS":
							Agenda.AddTargets(Win64Dev, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
							Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
							break;
						case "WIN32DEV":
							Agenda.AddTargets(Win32Dev, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development);
							break;
						case "WIN32SHIP":
							Agenda.AddTargets(Win32Ship, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Shipping);
							break;
						case "PS4DEV":
							Agenda.AddTargets(PS4Dev, UnrealTargetPlatform.PS4, UnrealTargetConfiguration.Development);
							break;
						case "XBOXONEDEV":
							Agenda.AddTargets(XboxOneDev, UnrealTargetPlatform.XboxOne, UnrealTargetConfiguration.Development);
							break;
						case "IOSDEV":
							Agenda.AddTargets(IOSDev, UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Development);
							break;
						case "MACDEV":
							Agenda.AddTargets(MacDev, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);
							Agenda.AddTargets(MacProgramTargets, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);
							break;
                        case "ANDROIDDEV":
                            Agenda.AddTargets(AndroidDev, UnrealTargetPlatform.Android, UnrealTargetConfiguration.Development);
                            break;
					}
				}
				else
				{
					Agenda.AddTargets(Win64Dev, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
					Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
					Agenda.AddTargets(Win32Dev, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development);
					Agenda.AddTargets(Win32Ship, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Shipping);
					Agenda.AddTargets(PS4Dev, UnrealTargetPlatform.PS4, UnrealTargetConfiguration.Development);
					Agenda.AddTargets(XboxOneDev, UnrealTargetPlatform.XboxOne, UnrealTargetConfiguration.Development);
					Agenda.AddTargets(IOSDev, UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Development);
				}
			}
		}

		UE4Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true);
		if (String.IsNullOrEmpty(Config) || Config.ToLower() == "editors")
		{
			UE4Build.AddUATFilesToBuildProducts();
		}
		UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);

		if (P4Enabled)
		{
			// Sign everything we built
			CodeSign.SignMultipleIfEXEOrDLL(this, UE4Build.BuildProductFiles);

			// Open files for add or edit
			UE4Build.AddBuildProductsToChangelist(WorkingCL, UE4Build.BuildProductFiles);

			// Check everything in!
			int SubmittedCL;
			Submit(WorkingCL, out SubmittedCL, true, true);

			// Label it            
			if (String.IsNullOrEmpty(Config))
			{
				MakeDownstreamLabel("Promotable");
			}
			else if (Config.ToLower() == "editors")
			{
				MakeDownstreamLabel("Promotable-Partial-" + Config);
			}
			else
			{
				MakeDownstreamLabel("Partial-Promotable-" + Config, UE4Build.BuildProductFiles);
			}
		}
	}

	private static void RemoveTargetsFromList(List<string> TargetsToRemove, List<string> List)
	{
		for (int TargetIndex = List.Count - 1; TargetIndex >= 0; --TargetIndex)
		{
			if (TargetsToRemove.Contains(List[TargetIndex]))
			{
				Log("Removing {0} from Agenda", List[TargetIndex]);
				List.RemoveAt(TargetIndex);
			}
		}
	}


	public override void ExecuteBuild()
	{
		Log("************************* PromotableBuild");

		LeanAndMean = ParseParam("LeanAndMean");
		UseXGE = !ParseParam("NoXGE");


		Log("************************* Lean and Mean Build:	{0}", LeanAndMean);
		Log("************************* Using XGE:			{0}", UseXGE);
		Log("************************* P4Enabled:			{0}", P4Enabled);


		Log("MsDev.exe located at {0}", CmdEnv.MsDevExe);

		int WorkingCL = -1;
		if (P4Enabled)
		{
			var Config = ParseParamValue("Config");
			if (String.IsNullOrEmpty(Config))
			{
				WorkingCL = CreateChange(P4Env.Client, String.Format("Promotable build built from changelist {0}", P4Env.Changelist));
			}
			else
			{
				WorkingCL = CreateChange(P4Env.Client, String.Format("{0} Partial Promotable build built from changelist {1}", Config, P4Env.Changelist));
			}
			Log("Build from {0}    Working in {1}", P4Env.Changelist, WorkingCL);
		}

		ExecuteInner(WorkingCL);
		PrintRunTime();
	}
}

class AggregatePromotable : BuildCommand
{
	public override void ExecuteBuild()
	{
		var Configs = ParseParamValue("Configs");
		if (String.IsNullOrEmpty(Configs))
		{
			Configs = "EditorsOnly";
		}
		bool UpstreamFailed = false;
		string[] ConfigList = Configs.Split('+');
		List<string> PartialLabels = new List<string>();
		if (P4Enabled)
		{
			//First sync the editors, which should contain the entire depot, then sync the individual partial labels to it
			//After syncing everything we'll create the proper 'Promotable' label.
			string EditorLabel = P4Env.BuildRootP4 + @"/Promotable-Partial-Editors-CL-" + P4Env.Changelist;
			Log("************ Syncing {0}", EditorLabel);
			if (!LabelExistsAndHasFiles(EditorLabel))
			{
				throw new P4Exception("The label {0} does not exist, or does not have files.  Check to see if the upstream job failed.", EditorLabel);
			}
			Sync("@" + EditorLabel);
			if (Configs.ToUpperInvariant() != "EDITORSONLY")
			{
				foreach (string Platform in ConfigList)
				{
					string PlatformLabel = P4Env.BuildRootP4 + @"/Partial-Promotable-" + Platform + "-CL-" + P4Env.Changelist;
					if (!LabelExistsAndHasFiles(PlatformLabel))
					{
						UpstreamFailed = true;
					}
					PartialLabels.Add(PlatformLabel);
				}

				if (UpstreamFailed)
				{
					Log("************ Upstream job failed.  Deleting Partial Labels");
					if (LabelExistsAndHasFiles(EditorLabel))
					{
						DeleteLabel(EditorLabel);
					}
					foreach (string Label in PartialLabels)
					{
						if (LabelExistsAndHasFiles(Label))
						{
							DeleteLabel(Label);
						}
					}
					throw new P4Exception("An upstream job has failed. We cannot sync all of the labels.");
				}
				else
				{
					foreach (string Label in PartialLabels)
					{
						Log("************* Syncing {0}", Label);
						Sync("@" + Label + ",@" + Label);
					}
				}
			}
			MakeDownstreamLabel("Promotable");

			//Clean up all the partial labels after the promotable is properly made
			DeleteLabel(EditorLabel);
			if (Configs.ToUpperInvariant() != "EDITORSONLY")
			{
				foreach (string Platform in ConfigList)
				{
					string PlatformLabel = P4Env.BuildRootP4 + @"/Partial-Promotable-" + Platform + "-CL-" + P4Env.Changelist;
					DeleteLabel(PlatformLabel);
				}
			}
		}
		else
		{
			throw new AutomationException("You must enable and initialize perforce to invoke AggregatePromotable");
		}

	}
}

class BuildUAT : BuildCommand
{
	void ExecuteInner(int WorkingCL)
	{
	}

	public override void ExecuteBuild()
	{
		Log("************************* BuildUAT");

		int WorkingCL = -1;
		if (P4Enabled)
		{
			WorkingCL = CreateChange(P4Env.Client, String.Format("UATOnly build built from changelist {0}", P4Env.Changelist));
			Log("Build from {0}    Working in {1}", P4Env.Changelist, WorkingCL);
		}

		ExecuteInner(WorkingCL);

		var UE4Build = new UE4Build(this);
		var Agenda = new UE4Build.BuildAgenda();

		Agenda.DotNetProjects.AddRange(
			new string[] 
			{
				@"Engine\Source\Programs\DotNETCommon\DotNETUtilities\DotNETUtilities.csproj",
			}
			);


		UE4Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true);

		UE4Build.AddUATFilesToBuildProducts();

		UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);

		if (P4Enabled)
		{
			// Sign everything we built
			CodeSign.SignMultipleIfEXEOrDLL(this, UE4Build.BuildProductFiles);

			// Open files for add or edit
			UE4Build.AddBuildProductsToChangelist(WorkingCL, UE4Build.BuildProductFiles);

			// Check everything in!
			int SubmittedCL;
			Submit(WorkingCL, out SubmittedCL, true, true);

			// Reset engine versions to 0 if required
			if (ParseParam("ZeroVersions"))
			{
				Execute<ZeroEngineVersions>(this);
			}
		}

		PrintRunTime();
	}
}
