// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;


class CIS : BuildCommand
{
	void ExecuteInner()
	{
		var UE4Build = new UE4Build(this);
        bool IsRunningOnMono = (Type.GetType("Mono.Runtime") != null);
		var Agenda = new UE4Build.BuildAgenda();

		var Config = ParseParamValue("Config");
		if (String.IsNullOrEmpty(Config))
		{
			Config = "tools";
		}

		if (Config.ToLower() == "tools")
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
		//This needs to be a separate target for distributed building because it is required to build anything else.
        if (!IsRunningOnMono)
        {
            var UHTTarget = new string[]
            {
                "UnrealHeaderTool",
            };

            Agenda.AddTargets(UHTTarget, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
        }

		var ProgramTargets = new string[] 
		{
			"UnrealFileServer",
			"ShaderCompileWorker",
			"MinidumpDiagnostics",
			"SymbolDebugger",
			"UnrealFrontend",
			"UnrealLightmass",
			"UnrealPak",			
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
		};

		var Win32Targets = new List<string> 
		{
			"FortniteGame",
			"FortniteServer",
            "FortniteClient",    
			"OrionGame",
			"PlatformerGame",			
			"ShooterGame",
			"StrategyGame",
			"VehicleGame",
			"Shadow",
            "Soul",
		};

		var MacDebugTargets = new List<string>
        {
            "FortniteGame",
            "FortniteEditor",
            "ShooterGameEditor",
            "ShooterGame",
        };

		var PS4DevTargets = new List<string>
		{
			"FortniteGame",
		};

		var XboxOneDevTargets = new List<string>
		{
			"QAGame",			
		};

		var IOSDevTargets = new List<string>
		{
			"QAGame",
			"Shadow",
			//"PlatformerGame",
			//"StrategyGame",
			"Soul",
		};

		var LinuxTargets = new List<string>
        {
            "FortniteServer",
        };

		var WinRTDevTargets = new List<string>
        {
            "QAGame",
        };

		var HTML5DevTargets = new List<string>
        {
            "QAGame",
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
                PS4DevTargets,
                XboxOneDevTargets,
                IOSDevTargets,
                LinuxTargets,
                HTML5DevTargets,
                WinRTDevTargets,
                MacDebugTargets,
                AndroidDevTargets
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
                "FortniteGame",
                "FortniteClient",
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
			}
		}
		// @todo: make this programmatic by looping over the TargetLists (which maybe should be a map from config to list)                
		string[] Win64Dev = Win64DevTargets.ToArray();
		string[] Win32Dev = Win32Targets.ToArray();
		string[] PS4Dev = PS4DevTargets.ToArray();
		string[] XboxOneDev = XboxOneDevTargets.ToArray();
		string[] IOSDev = IOSDevTargets.ToArray();
		string[] LinuxDev = LinuxTargets.ToArray();
		string[] WinRTDev = WinRTDevTargets.ToArray();
		string[] MacDebug = MacDebugTargets.ToArray();
		string[] HTML5Dev = HTML5DevTargets.ToArray();
        string[] AndroidDev = AndroidDevTargets.ToArray();

		switch (Config.ToUpperInvariant())
		{
			case "TOOLS":
				Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
				break;
			case "EDITORS":
				Agenda.AddTargets(Win64Dev, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);
				break;
			case "WIN32DEV":
				Agenda.AddTargets(Win32Dev, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development);
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
			case "LINUX":
				Agenda.AddTargets(LinuxDev, UnrealTargetPlatform.Linux, UnrealTargetConfiguration.Development);
				break;
			case "WINRT":
				Agenda.AddTargets(WinRTDev, UnrealTargetPlatform.WinRT, UnrealTargetConfiguration.Development);
				break;
			case "MAC":
				Agenda.AddTargets(MacDebug, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Debug);
				break;
			case "HTML5":
				Agenda.AddTargets(HTML5Dev, UnrealTargetPlatform.HTML5, UnrealTargetConfiguration.Development);
				break;
            case "ANDROID":
                Agenda.AddTargets(AndroidDev, UnrealTargetPlatform.Android, UnrealTargetConfiguration.Development);
                break;
		}
		UE4Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true);
		UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);
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
		Log("************************* P4Enabled:			{0}", P4Enabled);
		Log("MsDev.exe located at {0}", CmdEnv.MsDevExe);

		ExecuteInner();
		PrintRunTime();
	}


}

