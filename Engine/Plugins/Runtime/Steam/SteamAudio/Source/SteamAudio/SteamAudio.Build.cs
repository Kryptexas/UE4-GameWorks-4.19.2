//
// Copyright (C) Valve Corporation. All rights reserved.
//

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class SteamAudio : ModuleRules
	{
		public SteamAudio(ReadOnlyTargetRules Target) : base(Target)
		{

			OptimizeCode = CodeOptimization.Never;

			PrivateIncludePaths.AddRange(
				new string[] {
					"SteamAudio/Private",
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Engine",
					"AudioMixer",
					"InputCore",
					"RenderCore",
					"ShaderCore",
					"RHI"
				}
			);

			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"TargetPlatform",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"Projects",
					"AudioMixer"
				}
			);

			if (Target.bBuildEditor == true)
			{
				PrivateDependencyModuleNames.Add("UnrealEd");
				PrivateDependencyModuleNames.Add("Landscape");
			}

			AddEngineThirdPartyPrivateStaticDependencies(Target, "libPhonon");

			switch (Target.Platform)
			{
				case UnrealTargetPlatform.Win32:
				case UnrealTargetPlatform.Win64:
					PrivateDependencyModuleNames.Add("XAudio2");
					AddEngineThirdPartyPrivateStaticDependencies(Target, "DX11Audio");
					break;
				case UnrealTargetPlatform.Android:
					string PluginPath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
					AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginPath, "SteamAudio_APL.xml"));
					break;
				default:
					break;
			}
		}
	}
}
