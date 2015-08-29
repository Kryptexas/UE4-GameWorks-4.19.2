// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WebRTC : ModuleRules
{
	public WebRTC(TargetInfo Target)
	{
		string WebRtcSdkVer = "trunk";
		string WebRtcSdkPlatform = "";		

		Type = ModuleType.External;

		if ((Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32) &&
			WindowsPlatform.GetVisualStudioCompilerVersionName() == "2013") // @todo samz - VS2012 libs
		{
			WebRtcSdkPlatform = "win";
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			WebRtcSdkPlatform = "mac";
		}

		if (WebRtcSdkPlatform.Length > 0)
		{
			Definitions.Add("WITH_XMPP_JINGLE=1");

			string WebRtcSdkPath = UEBuildConfiguration.UEThirdPartySourceDirectory + "WebRTC/sdk_" + WebRtcSdkVer + "_" + WebRtcSdkPlatform;
			
			PublicSystemIncludePaths.Add(WebRtcSdkPath + "/include");

			if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
			{
				Definitions.Add("WEBRTC_WIN=1");

				string ConfigPath = (Target.Configuration == UnrealTargetConfiguration.Debug && BuildConfiguration.bDebugBuildsActuallyUseDebugCRT) ? "/Debug" : "/Release";
				string PlatformLibPath = Target.Platform == UnrealTargetPlatform.Win64 ? "/lib/Win64" : "/lib/Win32";
				string VSPath = "/VS" + WindowsPlatform.GetVisualStudioCompilerVersionName();

				PublicLibraryPaths.Add(WebRtcSdkPath + PlatformLibPath + VSPath + ConfigPath);

				PublicAdditionalLibraries.Add("rtc_base.lib");
				PublicAdditionalLibraries.Add("rtc_base_approved.lib");
				PublicAdditionalLibraries.Add("rtc_xmllite.lib");
				PublicAdditionalLibraries.Add("rtc_xmpp.lib");
				PublicAdditionalLibraries.Add("boringssl.lib");
				PublicAdditionalLibraries.Add("expat.lib"); 
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				Definitions.Add("WEBRTC_MAC=1");
				Definitions.Add("WEBRTC_POSIX=1");

				string ConfigPath = (Target.Configuration == UnrealTargetConfiguration.Debug) ? "/Debug" : "/Release";
				string LibPath = WebRtcSdkPath + "/lib" + ConfigPath;

				PublicAdditionalLibraries.Add(LibPath + "/librtc_base.a");
				PublicAdditionalLibraries.Add(LibPath + "/librtc_base_approved.a");
				PublicAdditionalLibraries.Add(LibPath + "/librtc_xmllite.a");
				PublicAdditionalLibraries.Add(LibPath + "/librtc_xmpp.a");
				PublicAdditionalLibraries.Add(LibPath + "/libboringssl.a");
				PublicAdditionalLibraries.Add(LibPath + "/libexpat.a");
			}
		}

    }
}
