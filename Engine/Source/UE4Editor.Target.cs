// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UE4EditorTarget : TargetRules
{
	public UE4EditorTarget( TargetInfo Target )
	{
		Type = TargetType.Editor;
	}

	public override bool GetSupportedPlatforms(ref List<UnrealTargetPlatform> OutPlatforms)
	{
		return UnrealBuildTool.UnrealBuildTool.GetAllDesktopPlatforms(ref OutPlatforms, false);
	}

	public override void SetupBinaries(
		TargetInfo Target,
		ref List<UEBuildBinaryConfiguration> OutBuildBinaryConfigurations,
		ref List<string> OutExtraModuleNames
		)
	{
		if ((Target.Platform == UnrealTargetPlatform.Win32) || (Target.Platform == UnrealTargetPlatform.Win64))
		{
			OutExtraModuleNames.Add("OnlineSubsystemNull");
			OutExtraModuleNames.Add("OnlineSubsystemAmazon");
			if (UEBuildConfiguration.bCompileSteamOSS == true)
			{
				OutExtraModuleNames.Add("OnlineSubsystemSteam");
			}
			OutExtraModuleNames.Add("OnlineSubsystemFacebook");
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac || Target.Platform == UnrealTargetPlatform.Linux)
		{
			OutExtraModuleNames.Add("OnlineSubsystemNull");
			if (UEBuildConfiguration.bCompileSteamOSS == true)
			{
				OutExtraModuleNames.Add("OnlineSubsystemSteam");
			}
		}
	}

	public override void SetupGlobalEnvironment(
		TargetInfo Target,
		ref LinkEnvironmentConfiguration OutLinkEnvironmentConfiguration,
		ref CPPEnvironmentConfiguration OutCPPEnvironmentConfiguration
		)
	{
		if( UnrealBuildTool.UnrealBuildTool.BuildingRocket() )
		{ 
			// no exports, so no need to verify that a .lib and .exp file was emitted by the linker.
			OutLinkEnvironmentConfiguration.bHasExports = false;
		}
	}
    public override GUBPProjectOptions GUBP_IncludeProjectInPromotedBuild_EditorTypeOnly(UnrealTargetPlatform HostPlatform)
    {
        var Result = new GUBPProjectOptions();
        Result.bIsPromotable = true;
        return Result;
    }
    public override Dictionary<string, List<UnrealTargetPlatform>> GUBP_NonCodeProjects_BaseEditorTypeOnly(UnrealTargetPlatform HostPlatform)
    {
        var NonCodeProjectNames = new Dictionary<string, List<UnrealTargetPlatform>>();

        List<UnrealTargetPlatform> DesktopPlats = null;
        if (HostPlatform == UnrealTargetPlatform.Win64)
        {
            DesktopPlats = new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Linux };
        }
        else
        {
            DesktopPlats = new List<UnrealTargetPlatform> { HostPlatform };
        }

        NonCodeProjectNames.Add("ElementalDemo", DesktopPlats);
        //NonCodeProjectNames.Add("InfiltratorDemo", DesktopPlats);
        NonCodeProjectNames.Add("HoverShip", DesktopPlats);
        NonCodeProjectNames.Add("BlueprintOffice", DesktopPlats);
        NonCodeProjectNames.Add("ReflectionsSubway", DesktopPlats);
        NonCodeProjectNames.Add("ElementalVR", DesktopPlats);
        NonCodeProjectNames.Add("CouchKnights", DesktopPlats);
        NonCodeProjectNames.Add("Stylized", DesktopPlats);
        //NonCodeProjectNames.Add("Landscape", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("MatineeFightScene", DesktopPlats);
        NonCodeProjectNames.Add("RealisticRendering", DesktopPlats);
        NonCodeProjectNames.Add("EffectsCave", DesktopPlats);
        NonCodeProjectNames.Add("GDC2014", DesktopPlats);
        NonCodeProjectNames.Add("ContentExamples", DesktopPlats);
        NonCodeProjectNames.Add("PhysicsPirateShip", DesktopPlats);
        NonCodeProjectNames.Add("TowerDefenseGame", DesktopPlats);
        NonCodeProjectNames.Add("LandscapeMountains", DesktopPlats);
        NonCodeProjectNames.Add("MorphTargets", DesktopPlats);
        NonCodeProjectNames.Add("PostProcessMatinee", DesktopPlats);
        NonCodeProjectNames.Add("SciFiHallway", DesktopPlats);

        List<UnrealTargetPlatform> MobilePlats = null;
        if (HostPlatform == UnrealTargetPlatform.Mac)
        {
            MobilePlats = new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.IOS };
        }
        else
        {
            MobilePlats = new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android };
        }

        NonCodeProjectNames.Add("BlackJack", MobilePlats);
        NonCodeProjectNames.Add("MemoryGame", MobilePlats);
        NonCodeProjectNames.Add("TappyChicken", MobilePlats);
        NonCodeProjectNames.Add("SwingNinja", MobilePlats);
        NonCodeProjectNames.Add("MobileTemple", MobilePlats);
        NonCodeProjectNames.Add("AnimationStarterPack", MobilePlats);

        NonCodeProjectNames.Add("StarterContent", MobilePlats);
        NonCodeProjectNames.Add("TP_FirstPersonBP", MobilePlats);
        NonCodeProjectNames.Add("TP_FlyingBP", MobilePlats);
        NonCodeProjectNames.Add("TP_RollingBP", MobilePlats);
        NonCodeProjectNames.Add("TP_SideScrollerBP", MobilePlats);
        NonCodeProjectNames.Add("TP_ThirdPersonBP", MobilePlats);
        NonCodeProjectNames.Add("TP_TopDownBP", MobilePlats);
        NonCodeProjectNames.Add("TP_VehicleBP", MobilePlats);

        return NonCodeProjectNames;
    }
    public override Dictionary<string, List<GUBPFormalBuild>> GUBP_GetNonCodeFormalBuilds_BaseEditorTypeOnly()
    {
        var NonCodeProjectNames = new Dictionary<string, List<GUBPFormalBuild>>();
        NonCodeProjectNames.Add("TappyChicken",
            new List<GUBPFormalBuild>
            {
                    new GUBPFormalBuild(UnrealTargetPlatform.Android, UnrealTargetConfiguration.Shipping, true),
                    new GUBPFormalBuild(UnrealTargetPlatform.Android, UnrealTargetConfiguration.Test, true),                    
                    new GUBPFormalBuild(UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Shipping, true),
                    new GUBPFormalBuild(UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Test, true),
            }
        );
        return NonCodeProjectNames;
    }
}
