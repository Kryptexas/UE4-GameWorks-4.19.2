// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RocketSupport.cpp: Implements the FRocketSupport class.
=============================================================================*/

#include "CorePrivate.h"
#include "RocketSupport.h"
#include "EngineVersion.h"


/* FRocketSupport interface
 *****************************************************************************/

bool FRocketSupport::IsRocket( const TCHAR* CmdLine )
{
	static int32 RocketState = -1;

	if (RocketState == -1)
	{
		checkf(CmdLine, TEXT("RocketSupport::IsRocket first call must have a valid command line!"));
#if UE_ROCKET
		// Shipping editor builds are always in rocket mode
		RocketState = 1;
#else
		// Pass "rocket" on the command-line in non-shipping editor builds to get rocket-like behavior
		static bool bIsRocket = FParse::Param(CmdLine, TEXT("rocket"));
		if (bIsRocket == true)
		{
			RocketState = 1;
		}
		else
		{
			// Rocket is not enabled
			RocketState = 0;
		}
#endif
	}

	return (RocketState == 1) ? true : false;
}

FString FRocketSupport::GetInstalledEngineDirectory()
{
#if PLATFORM_WINDOWS

	FString RegistryEntry = FString::Printf(TEXT("EpicGames/Unreal Engine/%s"), *GEngineVersion.ToString(EVersionComponent::Minor));

	FString InstalledDirectory;
	if (FPlatformMisc::GetRegistryString(RegistryEntry, TEXT("InstalledDirectory"), true, InstalledDirectory))
	{
		return InstalledDirectory.Replace(TEXT("\\"), TEXT("/")) / TEXT("Engine/");
	}
	if (FPlatformMisc::GetRegistryString(RegistryEntry, TEXT("InstalledDirectory"), false, InstalledDirectory))
	{
		return InstalledDirectory.Replace(TEXT("\\"), TEXT("/")) / TEXT("Engine/");
	}

#elif PLATFORM_MAC

	NSString* AppName = @"UE4Editor";
	NSFileManager* FileManager = [NSFileManager defaultManager];

	// First, check if there is a self-contained editor bundle in /Applications folder
	NSString* InstalledPath = [NSString stringWithFormat: @"/Applications/%@.app/Contents/UE4/Engine/", AppName];
	if ([FileManager fileExistsAtPath: InstalledPath])
	{
		return InstalledPath;
	}

	// No editor found in /Applications. In that case, we ask OS X to find the editor app bundle for us
	NSURL* AppURL = [[NSWorkspace sharedWorkspace] URLForApplicationWithBundleIdentifier: [@"com.epicgames." stringByAppendingString: AppName]];
	if (AppURL)
	{
		if ([FileManager fileExistsAtPath: [[AppURL path] stringByAppendingPathComponent: @"/Contents/UE4/Engine"]])
		{
			// It's a self-contained app
			return FString([AppURL path]) / TEXT("Contents/UE4/Engine/");
		}
		else
		{
			return FString([[AppURL path] stringByDeletingLastPathComponent]) / TEXT("../../Engine/");
		}
	}

#endif // PLATFORM

	return TEXT("");
}