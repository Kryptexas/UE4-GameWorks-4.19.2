// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISense.h"

const float UAISense::SuspendNextUpdate = FLT_MAX;

UAISense::UAISense(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, TimeUntilNextUpdate(SuspendNextUpdate)
{
}

void UAISense::PostInitProperties() 
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false) 
	{
		PerceptionSystemInstance = Cast<UAIPerceptionSystem>(GetOuter());
	}
}

AIPerception::FListenerMap* UAISense::GetListeners() 
{
	check(PerceptionSystemInstance);
	return &(PerceptionSystemInstance->GetListenersMap());
}