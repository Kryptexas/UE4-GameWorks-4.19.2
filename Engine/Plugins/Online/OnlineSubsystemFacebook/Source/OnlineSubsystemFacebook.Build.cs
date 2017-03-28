// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class OnlineSubsystemFacebook : ModuleRules
{
    public OnlineSubsystemFacebook(ReadOnlyTargetRules Target) : base(Target)
	{
		Definitions.Add("ONLINESUBSYSTEMFACEBOOK_PACKAGE=1");
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.Add("Private");

        PrivateDependencyModuleNames.AddRange(
            new string[] { 
                "Core",
                "CoreUObject",
                "HTTP",
				"ImageCore",
				"Json",
                "OnlineSubsystem", 
            }
            );

        AddEngineThirdPartyPrivateStaticDependencies(Target, "Facebook");

        if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PrivateIncludePaths.Add("Private/IOS");
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
			Definitions.Add("UE4_FACEBOOK_VER=4.19.0");

			PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Launch",
			}
			);

			string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, BuildConfiguration.RelativeEnginePath);
            AdditionalPropertiesForReceipt.Add(new ReceiptProperty("AndroidPlugin", Path.Combine(PluginPath, "OnlineSubsystemFacebook_UPL.xml")));

            PrivateIncludePaths.Add("Private/Android");
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32 || Target.Platform == UnrealTargetPlatform.Win64)
        {
            PrivateIncludePaths.Add("Private/Windows");
        }
		else
		{
			PrecompileForTargets = PrecompileTargetsType.None;
		}
	}
}
