// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISense_Hearing.h"

//----------------------------------------------------------------------//
// FAINoiseEvent
//----------------------------------------------------------------------//
FAINoiseEvent::FAINoiseEvent(AActor* InInstigator, const FVector& InNoiseLocation, float InLoudness)
	: Age(0.f), NoiseLocation(InNoiseLocation), Loudness(InLoudness), TeamIdentifier(uint8(-1)), Instigator(InInstigator)
{
	TeamIdentifier = FGenericTeamId::GetTeamIdentifier(InInstigator);
}

//----------------------------------------------------------------------//
// UAISense_Hearing
//----------------------------------------------------------------------//
UAISense_Hearing::UAISense_Hearing(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		static bool bMakeNoiseInterceptionSetUp = false;
		if (bMakeNoiseInterceptionSetUp == false)
		{
			AActor::SetMakeNoiseDelegate(FMakeNoiseDelegate::CreateStatic(&UAIPerceptionSystem::MakeNoiseImpl));
			bMakeNoiseInterceptionSetUp = true;
		}
	}
}

float UAISense_Hearing::Update()
{
	AIPerception::FListenerMap& ListenersMap = *GetListeners();
	UAIPerceptionSystem* PerseptionSys = GetPerceptionSystem();

	for (AIPerception::FListenerMap::TIterator ListenerIt(ListenersMap); ListenerIt; ++ListenerIt)
	{
		FPerceptionListener& Listener = ListenerIt->Value;

		if (Listener.HasSense(GetSenseIndex()) == false)
		{
			// skip listeners not interested in this sense
			continue;
		}

		for (int32 EventIndex = 0; EventIndex < NoiseEvents.Num(); ++EventIndex)
		{
			const FAINoiseEvent& Event = NoiseEvents[EventIndex];
		
			// @todo implement some kind of TeamIdentifierType that would supply comparison operator 
			if (Listener.TeamIdentifier == Event.TeamIdentifier 
				|| FVector::DistSquared(Event.NoiseLocation, Listener.CachedLocation) > Listener.HearingRangeSq * Event.Loudness)
			{
				continue;
			}
			// calculate delay and fake it with Age
			const float Delay = FVector::DistSquared(Event.NoiseLocation, Listener.CachedLocation)/900000;
			// pass over to listener to process 			
			PerseptionSys->RegisterDelayedStimulus(Listener.GetListenerId(), Delay, Event.Instigator
				, FAIStimulus(GetSenseIndex(), Event.Loudness, Event.NoiseLocation, Listener.CachedLocation) );
		}
	}

	NoiseEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UAISense_Hearing::RegisterEvent(const FAINoiseEvent& Event)
{
	NoiseEvents.Add(Event);

	RequestImmediateUpdate();
}
