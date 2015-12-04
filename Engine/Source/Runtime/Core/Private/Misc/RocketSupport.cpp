// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "RocketSupport.h"
#include "EngineVersion.h"

DECLARE_LOG_CATEGORY_CLASS(LogRocket, Log, All);


/* FRocketSupport interface
 *****************************************************************************/

bool FRocketSupport::IsRocket( const TCHAR* CmdLine )
{
	static int32 RocketState = -1;

	if (RocketState == -1)
	{
		checkf(CmdLine, TEXT("RocketSupport::IsRocket first call must have a valid command line!"));

		// Pass "rocket" on the command-line in non-shipping editor builds to get rocket-like behavior
		static bool bIsRocket = FParse::Param(CmdLine, TEXT("rocket"));
		if (bIsRocket)
		{
			UE_LOG(LogRocket, Warning, TEXT("Use of -rocket argument on command-line to test Rocket behavior is deprecated, please ensure that you've built a true Rocket build."));
		}
		FString RocketFile = FPaths::RootDir() / TEXT("Engine/Build/Rocket.txt");
		FPaths::NormalizeFilename(RocketFile);
		bIsRocket |= IFileManager::Get().FileExists(*RocketFile);
		if (bIsRocket == true)
		{
			RocketState = 1;
		}
		else
		{
			// Rocket is not enabled
			RocketState = 0;
		}
	}

	return (RocketState == 1) ? true : false;
}
