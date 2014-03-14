// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;

class PromotableBuildMacOnly : BuildCommand
{
	bool LeanAndMean;
	void ExecuteInner(int WorkingCL)
	{

		var UE4Build = new UE4Build(this);
		var Agenda = new UE4Build.BuildAgenda();

		Agenda.DotNetSolutions.AddRange(
			new string[] 
			{
				//@"Engine\Source\Programs\UnrealDocTool\UnrealDocTool\UnrealDocTool.sln",
				//@"Engine\Source\Programs\NetworkProfiler\NetworkProfiler.sln",
			}
			);

		Agenda.DotNetProjects.AddRange(
			new string[] 
			{
				//@"Engine\Source\Programs\DotNETCommon\DotNETUtilities\DotNETUtilities.csproj",
				//@"Engine\Source\Programs\CrashReporter\CrashReportCommon\CrashReportCommon.csproj",
				//@"Engine\Source\Programs\CrashReporter\CrashReportInput\CrashReportInput.csproj",
				//@"Engine\Source\Programs\CrashReporter\CrashReportUploader\CrashReportUploader.csproj",
				//@"Engine\Source\Programs\CrashReporter\CrashReportReceiver\CrashReportReceiver.csproj",
				//@"Engine\Source\Programs\CrashReporter\CrashReportProcess\CrashReportProcess.csproj",
				//@"Engine\Source\Programs\CrashReporter\RegisterPII\RegisterPII.csproj",
				//@"Engine\Source\Programs\Distill\Distill.csproj",
				//@"Engine\Source\Programs\UnSetup\UnSetup.csproj",
			}
			);

		Agenda.ExtraDotNetFiles.AddRange(
			new string[] 
			{
				//"Interop.IWshRuntimeLibrary",
				//"UnrealMarkdown",
				//"CommonUnrealMarkdown",
			}
			);

		{
			if (LeanAndMean)
			{
				var ProgramTargets = new string[] 
				{
					"CrashReportClient",
					"ShaderCompileWorker",
					"UnrealHeaderTool",
					"UnrealLightmass",
				};
				Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);

				var MacDevTargets = new string[] 
				{
					"QAGameEditor",
				};
				Agenda.AddTargets(MacDevTargets, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);
			}
			else
			{
				var ProgramTargets = new string[] 
				{
					"BuildPatchTool",
					"CrashReportClient",
					"UnrealFileServer",
					"ShaderCompileWorker",
					//"MinidumpDiagnostics",
					//"SymbolDebugger",
					"UnrealFrontend",
					"UnrealHeaderTool",
					"UnrealLightmass",
					"UnrealPak",
					//"UnrealRemoteAgent",
				};
				Agenda.AddTargets(ProgramTargets, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);

				var MacDevTargets = new string[] 
				{
//					"FortniteEditor",
//					"OrionEditor",
					"PlatformerGameEditor",
					"QAGameEditor",
					"ShooterGameEditor",
					"StrategyGameEditor",
//					"VehicleGameEditor",
//					"FortniteGame",
//					"FortniteServer",
//					"OrionGame",
					"PlatformerGame",
					"QAGame",
					"ShooterGame",
					"StrategyGame",
//					"VehicleGame",
				};
				Agenda.AddTargets(MacDevTargets, UnrealTargetPlatform.Mac, UnrealTargetConfiguration.Development);

			}
		}


		UE4Build.Build(Agenda, InDeleteBuildProducts: true, InUpdateVersionFiles: true);

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
			MakeDownstreamLabel("PromotableMacOnly");
		}

	}

	public override void ExecuteBuild()
	{
		Log("************************* PromotableBuildMacOnly");

		LeanAndMean = ParseParam("LeanAndMean");

		Log("************************* Lean and Mean Build:	{0}", LeanAndMean);
		Log("************************* P4Enabled:			{0}", P4Enabled);

		int WorkingCL = -1;
		if (P4Enabled)
		{
			WorkingCL = CreateChange(P4Env.Client, String.Format("PromotableMacOnly build built from changelist {0}", P4Env.Changelist));
			Log("Build from {0}    Working in {1}", P4Env.Changelist, WorkingCL);
		}		

		ExecuteInner(WorkingCL);
		PrintRunTime();
	}
}
