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
		else if (Target.Platform == UnrealTargetPlatform.Mac)
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
			// Tag it as a Rocket build
			OutCPPEnvironmentConfiguration.Definitions.Add("UE_ROCKET=1");

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
        NonCodeProjectNames.Add("Elemental", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Infiltrator", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("HoverShip", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Blueprint_Examples", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Reflections", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("ContentExamples", new List<UnrealTargetPlatform> { HostPlatform });

        NonCodeProjectNames.Add("MobileTemple", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TappyChicken", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("SwingNinja", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("Mobile", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });

        NonCodeProjectNames.Add("TP_FirstPersonBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TP_FlyingBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TP_SideScrollerBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TP_StarterContentBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TP_ThirdPersonBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TP_TopDownBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });
        NonCodeProjectNames.Add("TP_VehicleBP", new List<UnrealTargetPlatform> { HostPlatform, UnrealTargetPlatform.Android, UnrealTargetPlatform.IOS });

        return NonCodeProjectNames;
    }
    public override string GUBP_GetPromotionEMails_EditorTypeOnly(string Branch)
    {
        return "Engine-QA[epic] Engine-QA-Team[epic]";
    }
}
