// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class IOSTapJoy : ModuleRules
	{
		public IOSTapJoy( TargetInfo Target )
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Advertising",
					// ... add private dependencies that you statically link with here ...
				}
				);

			PublicIncludePathModuleNames.Add( "Advertising" );

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

			PublicAdditionalShadowFiles.Add( "../Plugins/Runtime/Advertising/IOSTapJoy/ThirdPartyFrameworks/Tapjoy.embeddedframework.zip" );

			PublicAdditionalFrameworks.Add( "../Plugins/Runtime/Advertising/IOSTapJoy/ThirdPartyFrameworks/Tapjoy.embeddedframework/TapJoy" );

			PublicFrameworks.AddRange( 
				new string[] 
				{ 
					"EventKit",
					"MediaPlayer",
					"AdSupport",
					"CoreLocation",
					"SystemConfiguration",
					"MessageUI",
					"CoreMotion",
					"Security",
					"CoreTelephony",
					"Twitter",
					"Social"
				}
				);
		}
	}
}