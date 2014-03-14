// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using AutomationTool;
using UnrealBuildTool;

[Help("Builds all the main projects editors and confirms the results")]
[RequireP4]
class VerifyPromotableBuild : BuildCommand
{
	void ExecuteInner()
	{
		var UE4Build = new UE4Build(this);

		var Agenda_Mono = new UE4Build.BuildAgenda();
		var Agenda_NonUnity = new UE4Build.BuildAgenda();
		{
			{
				var Win64DevTargets = new string[] 
				{
					"FortniteEditor",
					"OrionEditor",
					"PlatformerGameEditor",
					"QAGameEditor",
					"ShooterGameEditor",
					"StrategyGameEditor",
					"VehicleGameEditor",
				};
				Agenda_Mono.AddTargets(Win64DevTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development, bForceMonolithic: true);
				Agenda_Mono.AddTargets(Win64DevTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Shipping, bForceMonolithic: true);
				Agenda_Mono.AddTargets(Win64DevTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Development, bForceNonUnity: true);
				Agenda_Mono.AddTargets(Win64DevTargets, UnrealTargetPlatform.Win64, UnrealTargetConfiguration.Shipping, bForceNonUnity: true);

				var Win32Targets = new string[] 
				{
					"FortniteGame",
					"FortniteServer",
					"OrionGame",
					"PlatformerGame",
					"QAGame",
					"ShooterGame",
					"StrategyGame",
					"VehicleGame",
				};
				Agenda_Mono.AddTargets(Win32Targets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development, bForceMonolithic: true);
				Agenda_Mono.AddTargets(Win32Targets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Development, bForceNonUnity: true);

				var Win32ShipTargets = new string[] 
				{
					"FortniteGame",
					"FortniteServer",
				};
				Agenda_Mono.AddTargets(Win32ShipTargets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Shipping, bForceMonolithic: true);
				Agenda_Mono.AddTargets(Win32ShipTargets, UnrealTargetPlatform.Win32, UnrealTargetConfiguration.Shipping, bForceNonUnity: true);
			}
		}

		UE4Build.Build(Agenda_Mono, InDeleteBuildProducts: true, InUpdateVersionFiles: true);

		UE4Build.Build(Agenda_NonUnity, InDeleteBuildProducts: true, InUpdateVersionFiles: true);
		UE4Build.CheckBuildProducts(UE4Build.BuildProductFiles);	
	}

	public override void ExecuteBuild()
	{
		Log("************************* PromotableBuild");
		Log("Verify label {0}", P4Env.LabelToSync);

		ExecuteInner();
	}
}
