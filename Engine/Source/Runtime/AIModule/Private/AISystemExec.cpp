// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "Misc/CoreMisc.h"

struct FAISystemExec : public FSelfRegisteringExec
{
	FAISystemExec()
	{
	}

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;
	// End FExec Interface
};

FAISystemExec AISystemExecInstance;

bool FAISystemExec::Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;

	if (FParse::Command(&Cmd, TEXT("AIIgnorePlayers")))
	{
		AAIController::ToggleAIIgnorePlayers();		
		bHandled = true; 
	}
	else if (FParse::Command(&Cmd, TEXT("AILoggingVerbose")))
	{
		if (Inworld)
		{
			APlayerController* PC = Inworld->GetFirstPlayerController();
			if (PC)
			{
				PC->ConsoleCommand(TEXT("log lognavigation verbose | log logpathfollowing verbose | log LogCharacter verbose | log LogBehaviorTree verbose | log LogPawnAction verbose|"));
			}
		}
		bHandled = true;
	}

	return bHandled;
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)