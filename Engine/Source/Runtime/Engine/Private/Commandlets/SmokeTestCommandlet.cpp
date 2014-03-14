// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SmokeTestCommandlet.cpp: Commandled used for smoke testing.

=============================================================================*/

#include "EnginePrivate.h"

USmokeTestCommandlet::USmokeTestCommandlet(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	IsClient = false;
	IsEditor = false;
	LogToConsole = true;
}



int32 USmokeTestCommandlet::Main( const FString& Params )
{
	const TCHAR* Parms = *Params;

	GIsRequestingExit = true; // so CTRL-C will exit immediately
	bool bAllSuccessful = FAutomationTestFramework::GetInstance().RunSmokeTests();

	return bAllSuccessful ? 0 : 1;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRunServer, "Core.Misc.Run Server", EAutomationTestFlags::ATF_Commandlet | EAutomationTestFlags::ATF_SmokeTest)

bool FRunServer::RunTest(const FString& Parameters)
{
	// put various smoke test testing code in here before moving off to their
	// own commandlet
	if( GEngine && IsRunningDedicatedServer() )
	{
		// Update GCurrentTime/ GDeltaTime while taking into account max tick rate.
		GEngine->UpdateTimeAndHandleMaxTickRate();

		// Tick the engine.
		GEngine->Tick( GDeltaTime, false );
	}

	return true;
}


