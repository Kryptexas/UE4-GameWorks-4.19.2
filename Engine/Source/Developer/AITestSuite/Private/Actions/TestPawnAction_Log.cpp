// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"

UTestPawnAction_Log::UTestPawnAction_Log(const FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, Logger(nullptr)
{

}

UTestPawnAction_Log* UTestPawnAction_Log::CreateAction(UWorld& World, FTestLogger<int32>& InLogger)
{
	UTestPawnAction_Log* Action = UPawnAction::CreateActionInstance<UTestPawnAction_Log>(World);
	if (Action)
	{
		Action->Logger = &InLogger;
	}

	return Action;
}

bool UTestPawnAction_Log::Start()
{
	Logger->Log(ETestPawnActionMessage::Started);

	return Super::Start();
}

bool UTestPawnAction_Log::Pause(const UPawnAction* PausedBy)
{
	Logger->Log(ETestPawnActionMessage::Paused);

	return Super::Pause(PausedBy);
}

bool UTestPawnAction_Log::Resume()
{
	Logger->Log(ETestPawnActionMessage::Resumed);

	return Super::Resume();
}

void UTestPawnAction_Log::OnFinished(EPawnActionResult::Type WithResult)
{
	Logger->Log(ETestPawnActionMessage::Finished);
}

void UTestPawnAction_Log::OnChildFinished(UPawnAction* Action, EPawnActionResult::Type WithResult)
{
	Logger->Log(ETestPawnActionMessage::ChildFinished);
}
