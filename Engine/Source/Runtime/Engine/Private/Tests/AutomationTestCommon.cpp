// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AutomationTestCommon.h"


///////////////////////////////////////////////////////////////////////
// Common Latent commands which are used across test type. I.e. Engine, Network, etc...


bool FPlayMatineeLatentCommand::Update() 
{
	if (MatineeActor)
	{
		//force this matinee to not be looping so it doesn't infinitely loop
		MatineeActor->bLooping = false;
		MatineeActor->Play();
	}
	return true;
}


bool FWaitForMatineeToCompleteLatentCommand::Update()
{
	bool bTestComplete = true;
	if (MatineeActor)
	{
		bTestComplete = !MatineeActor->bIsPlaying;
	}
	return bTestComplete;
}


bool FExecStringLatentCommand::Update()
{
	if (GEngine->GameViewport)
	{
		GEngine->GameViewport->Exec( NULL, *ExecCommand, *GLog);
	}
	else
	{
		GEngine->Exec( NULL, *ExecCommand);
	}
	return true;
}


bool FEngineWaitLatentCommand::Update()
{
	float NewTime = FPlatformTime::Seconds();
	if (NewTime - StartTime >= Duration)
	{
		return true;
	}
	return false;
}


bool FEnqueuePerformanceCaptureCommands::Update()
{
	//for every matinee actor in the level
	for (TObjectIterator<AMatineeActor> It; It; ++It)
	{
		AMatineeActor* MatineeActor = *It;
		if(MatineeActor && MatineeActor->GetName().Contains(TEXT("Automation")) )
		{
			//add latent action to execute this matinee
			ADD_LATENT_AUTOMATION_COMMAND(FPlayMatineeLatentCommand(MatineeActor));

			//add action to wait until matinee is complete
			ADD_LATENT_AUTOMATION_COMMAND(FWaitForMatineeToCompleteLatentCommand(MatineeActor));
		}
	}

	return true;
}