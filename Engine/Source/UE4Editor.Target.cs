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
        NonCodeProjectNames.Add("BlueprintExamples", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Reflections", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("ElementalVR", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("VersusVR", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Stylized", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Landscape", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Matinee", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("RealisticRendering", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("Effects", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("GDC2014", new List<UnrealTargetPlatform> { HostPlatform });
        NonCodeProjectNames.Add("ContentExamples", new List<UnrealTargetPlatform> { HostPlatform });

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
        NonCodeProjectNames.Add("Card", MobilePlats);
        NonCodeProjectNames.Add("TappyChicken", MobilePlats);
        NonCodeProjectNames.Add("SwingNinja", MobilePlats);
        NonCodeProjectNames.Add("Mobile", MobilePlats);

        NonCodeProjectNames.Add("StarterContent", MobilePlats);
        NonCodeProjectNames.Add("TP_FirstPersonBP", MobilePlats);
        NonCodeProjectNames.Add("TP_FlyingBP", MobilePlats);
        NonCodeProjectNames.Add("TP_RollingBP", MobilePlats);
        NonCodeProjectNames.Add("TP_SideScrollerBP", MobilePlats);
        NonCodeProjectNames.Add("TP_ThirdPersonBP", MobilePlats);
        NonCodeProjectNames.Add("TP_TopDownBP", MobilePlats);

        return NonCodeProjectNames;
    }
    public override Dictionary<string, List<KeyValuePair<UnrealTargetPlatform, UnrealTargetConfiguration>>> GUBP_NonCodeFormalBuilds_BaseEditorTypeOnly()
    {
        var NonCodeProjectNames = new Dictionary<string, List<KeyValuePair<UnrealTargetPlatform, UnrealTargetConfiguration>>>();
        NonCodeProjectNames.Add("TappyChicken", 
            new List<KeyValuePair<UnrealTargetPlatform, UnrealTargetConfiguration>>
            {
                    new KeyValuePair<UnrealTargetPlatform, UnrealTargetConfiguration>(UnrealTargetPlatform.IOS, UnrealTargetConfiguration.Shipping),
                    new KeyValuePair<UnrealTargetPlatform, UnrealTargetConfiguration>(UnrealTargetPlatform.Android, UnrealTargetConfiguration.Shipping)
            }
        );
        return NonCodeProjectNames;
    }
}
