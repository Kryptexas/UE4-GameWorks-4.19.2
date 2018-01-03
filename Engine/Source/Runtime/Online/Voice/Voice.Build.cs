// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Voice : ModuleRules
{
	public Voice(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDefinitions.Add("VOICE_PACKAGE=1");

		bool bDontNeedCapture = (Target.Type == TargetType.Server);

		if (bDontNeedCapture)
		{
			PublicDefinitions.Add("VOICE_MODULE_WITH_CAPTURE=0");
		}
		else
		{
			PublicDefinitions.Add("VOICE_MODULE_WITH_CAPTURE=1");
		}

		PublicIncludePathModuleNames.AddRange(
			new string[] {
				"AndroidPermission"
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Engine",
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				"Runtime/Online/Voice/Private",
			}
			);

		PrivateDependencyModuleNames.AddRange(
			new string[] { 
				"Core",
                "AudioMixer"
            }
			);

		if (Target.Platform == UnrealTargetPlatform.Win32 ||
			Target.Platform == UnrealTargetPlatform.Win64)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "DirectSound");
		}
		else if(Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicFrameworks.AddRange(new string[] { "CoreAudio", "AudioUnit", "AudioToolbox" });
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux && !bDontNeedCapture)
		{
			AddEngineThirdPartyPrivateStaticDependencies(Target, "SDL2");
		}

		AddEngineThirdPartyPrivateStaticDependencies(Target, "libOpus");

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			string ModulePath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModulePath, "AndroidVoiceImpl_UPL.xml"));
		}
	}
}


		



		
