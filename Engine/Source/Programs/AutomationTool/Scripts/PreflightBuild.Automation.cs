// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;

[Help("Constructs a build to test a shelve")]
[Help("LeanAndMean", "Toggle to construct a subset of the potential targets")]
[Help(typeof(UE4Build))]
class PreflightBuild : BuildCommand
{
	bool LeanAndMean;
	void ExecuteInner()
	{
		var UE4Build = new UE4Build(this);
		var Agenda = new UE4Build.BuildAgenda();

		Agenda.DotNetSolutions.AddRange(
			new string[] 
			{
				@"Engine\Source\Programs\UnrealDocTool\UnrealDocTool\UnrealDocTool.sln",
				@"Engine\Source\Programs\NetworkProfiler\NetworkProfiler.sln",
			}
			);

		Agenda.DotNetProjects.AddRange(
			new string[] 
			{
				@"Engine\Source\Programs\DotNETCommon\DotNETUtilities\DotNETUtilities.csproj",
				@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportCommon\CrashReportCommon.csproj",
				@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportReceiver\CrashReportReceiver.csproj",
				@"Engine\Source\Programs\NoRedist\CrashReportServer\CrashReportProcess\CrashReportProcess.csproj",
				@"Engine\Source\Programs\CrashReporter\RegisterPII\RegisterPII.csproj",
				@"Engine\Source\Programs\Distill\Distill.csproj",				
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

		{
			if (LeanAndMean && !ParseParam("NoXGE"))
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
				var ProgramTargets = new string[] 
				{
					"UnrealFileServer",
					"ShaderCompileWorker",
					"MinidumpDiagnostics",
					"SymbolDebugger",
					"UnrealFrontend",
					"UnrealHeaderTool",
					"BlankProgram",
					"UnrealLightmass",
					"UnrealPak",
				};
				Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);

				var Win64DevTargets = new string[] 
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
				Agenda.AddTargets(Win64DevTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development);

				var Win32Targets = new string[] 
				{
					"FortniteClient",
					"OrionServer",
					"ShooterGame",
				};
				Agenda.AddTargets(Win32Targets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development);

				var Win32ShipTargets = new string[] 
				{
					"OrionGame",
					"FortniteServer",
				};
				Agenda.AddTargets(Win32ShipTargets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Shipping);

				var PS4DevTargets = new string[]
				{
					// could also have "QAGame", "UE4Game",
					"FortniteGame",
				};
				Agenda.AddTargets(PS4DevTargets, UnrealTargetPlatform.PS4, UnrealTargetConfiguration.Development);

				var XboxOneDevTargets = new string[]
				{
					// could also have "FortniteGame", "UE4Game",
					"QAGame",
				};
				Agenda.AddTargets(XboxOneDevTargets, UnrealTargetPlatform.XboxOne, UnrealTargetConfiguration.Development);

				var IOSDevTargets = new string[]
				{
					// could also have "QAGame", "UE4Game", PlatformerGame", "StrategyGame",
					"Shadow",
					"Soul",
				};
				Agenda.AddTargets(IOSDevTargets, UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Development);
			}
		}

		Agenda.DoRetries = false;
		UE4Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true);

		UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);
	}

	public override void ExecuteBuild()
	{
		Log("************************* PreflightBuild");

		LeanAndMean = ParseParam("LeanAndMean");
		int Shelve = ParseParamInt("Shelve", 0);

		Log("************************* Lean and Mean Build:	{0}", LeanAndMean);
		Log("************************* P4Enabled:			{0}", P4Enabled);

		if (P4Enabled)
		{
			if (Shelve != 0 && ((Shelve - P4Env.Changelist) > 100000 || (P4Env.Changelist - Shelve) < -100000))
			{
				throw new AutomationException("Kindof a large difference between the start CL {0} and the shelve {1}, don't you think?", P4Env.Changelist, Shelve);
			}

			Log("Build from {0}  Shelve {1}", P4Env.Changelist, Shelve);
			if (Shelve != 0)
			{
				Log("{0} should already be unshelved...", Shelve);
			}
		}


		ExecuteInner();
		PrintRunTime();
	}
}

class DistributedLinkTest : BuildCommand
{
	void ExecuteInner()
	{
		var UE4Build = new UE4Build(this);

		var Agenda = new UE4Build.BuildAgenda();

		{
			var Win32Targets = new string[] 
			{
				"FortniteClient",
				"FortniteGame",
				"FortniteServer",
				"OrionServer",
				"OrionClient",
				"OrionGame",
			};
			Agenda.AddTargets(Win32Targets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Shipping);
			Agenda.AddTargets(Win32Targets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Test);
		}

		Agenda.DoRetries = false;
		Agenda.SpecialTestFlag = true;
		UE4Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true);

		UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);
	}

	public override void ExecuteBuild()
	{
		Log("************************* DistributedLinkTest");

		ExecuteInner();
		PrintRunTime();
	}
}


