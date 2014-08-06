// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISense_Damage.h"

UAISense_Damage::UAISense_Damage(const class FPostConstructInitializeProperties& PCIP) :
	Super(PCIP)
{
	
}

float UAISense_Damage::Update()
{
	AIPerception::FListenerMap& ListenersMap = *GetListeners();

	for (int32 EventIndex = 0; EventIndex < RegisteredEvents.Num(); ++EventIndex)
	{
		const FAIDamageEvent& Event = RegisteredEvents[EventIndex];

		IAIPerceptionListenerInterface* PerceptionListener = InterfaceCast<IAIPerceptionListenerInterface>(Event.DamagedActor);
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

void UAISense_Damage::RegisterEvent(const FAIDamageEvent& Event)
{
	RegisteredEvents.Add(Event);

	RequestImmediateUpdate();
}