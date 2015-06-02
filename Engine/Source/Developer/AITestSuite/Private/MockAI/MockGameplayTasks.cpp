// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
UMockTask_Log::UMockTask_Log(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Logger(nullptr)
{

}

UMockTask_Log* UMockTask_Log::CreateTask(IGameplayTaskOwnerInterface& TaskOwner, FTestLogger<int32>& InLogger)
{
	UMockTask_Log* Task = NewTask<UMockTask_Log>(TaskOwner);
	if (Task)
	{
		Task->Logger = &InLogger;
	}
	return Task;
}

void UMockTask_Log::Activate()
{
	Logger->Log(ETestTaskMessage::Activate);
	Super::Activate();
}

void UMockTask_Log::OnDestroy(bool bOwnerFinished)
{
	Logger->Log(ETestTaskMessage::Ended);
	Super::OnDestroy(bOwnerFinished);
}

void UMockTask_Log::TickTask(float DeltaTime)
{
	Logger->Log(ETestTaskMessage::Tick);
	Super::TickTask(DeltaTime);
}

void UMockTask_Log::ExternalConfirm(bool bEndTask)
{
	Logger->Log(ETestTaskMessage::ExternalConfirm);
	Super::ExternalConfirm(bEndTask);
}

void UMockTask_Log::ExternalCancel()
{
	Logger->Log(ETestTaskMessage::ExternalCancel);
	Super::ExternalCancel();
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
