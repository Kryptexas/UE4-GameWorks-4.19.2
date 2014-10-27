// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISenseImplementation.h"

const float UAISenseImplementation::SuspendNextUpdate = FLT_MAX;

UAISenseImplementation::UAISenseImplementation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TimeUntilNextUpdate(SuspendNextUpdate)
{
}

void UAISenseImplementation::PostInitProperties() 
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false) 
	{
		PerceptionSystemInstance = Cast<UAIPerceptionSystem>(GetOuter());
	}
}

AIPerception::FListenerMap* UAISenseImplementation::GetListeners() 
{
	check(PerceptionSystemInstance);
	return &(PerceptionSystemInstance->GetListenersMap());
}