// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISenseImplementation_Damage.h"

UAISenseImplementation_Damage::UAISenseImplementation_Damage(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	
}

float UAISenseImplementation_Damage::Update()
{
	AIPerception::FListenerMap& ListenersMap = *GetListeners();

	for (int32 EventIndex = 0; EventIndex < RegisteredEvents.Num(); ++EventIndex)
	{
		const FAIDamageEvent& Event = RegisteredEvents[EventIndex];

		IAIPerceptionListenerInterface* PerceptionListener = Cast<IAIPerceptionListenerInterface>(Event.DamagedActor);
		if (PerceptionListener != NULL)
		{
			UAIPerceptionComponent* PerceptionComponent = PerceptionListener->GetPerceptionComponent();
			if (PerceptionComponent != NULL)
			{
				// this has to succeed, will assert a failure
				FPerceptionListener& Listener = ListenersMap[PerceptionComponent->GetListenerId()];

				Listener.RegisterStimulus(Event.Instigator, FAIStimulus(GetSenseIndex(), Event.Amount, Event.Location, Event.HitLocation));
			}
		}
	}

	RegisteredEvents.Reset();

	// return decides when next tick is going to happen
	return SuspendNextUpdate;
}

void UAISenseImplementation_Damage::RegisterEvent(const FAIDamageEvent& Event)
{
	RegisteredEvents.Add(Event);

	RequestImmediateUpdate();
}