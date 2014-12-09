// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Team.h"
#include "Perception/AISense_Touch.h"
#include "Perception/AISense_Prediction.h"
#include "Perception/AISense_Damage.h"

DECLARE_CYCLE_STAT(TEXT("Perception System"),STAT_AI_PerceptionSys,STATGROUP_AI);

DEFINE_LOG_CATEGORY(LogAIPerception);

//----------------------------------------------------------------------//
// UAIPerceptionSystem
//----------------------------------------------------------------------//
UAIPerceptionSystem::UAIPerceptionSystem(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PerceptionAgingRate(0.3f)
	, NextFreeListenerId(AIPerception::InvalidListenerId + 1)
	, CurrentTime(0.f)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		Senses.AddZeroed(ECorePerceptionTypes::MAX);
		Senses[ECorePerceptionTypes::Hearing] = ConstructObject<UAISense>(UAISense_Hearing::StaticClass(), this);
		Senses[ECorePerceptionTypes::Sight] = ConstructObject<UAISense>(UAISense_Sight::StaticClass(), this);
		Senses[ECorePerceptionTypes::Damage] = ConstructObject<UAISense>(UAISense_Damage::StaticClass(), this);
		Senses[ECorePerceptionTypes::Touch] = ConstructObject<UAISense>(UAISense_Touch::StaticClass(), this);		
		Senses[ECorePerceptionTypes::Team] = ConstructObject<UAISense>(UAISense_Team::StaticClass(), this);		
		Senses[ECorePerceptionTypes::Prediction] = ConstructObject<UAISense>(UAISense_Prediction::StaticClass(), this);
	}
}

void UAIPerceptionSystem::PostInitProperties() 
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(GetOuter());
		if (World)
		{
			World->GetTimerManager().SetTimer(this, &UAIPerceptionSystem::AgeStimuli, PerceptionAgingRate, /*inbLoop=*/true);
		}
	}
}

void UAIPerceptionSystem::RegisterSource(FAISenseId SenseIndex, AActor& SourceActor)
{
	SourcesToRegister.Add(FPerceptionSourceRegistration(SenseIndex, &SourceActor));
}

void UAIPerceptionSystem::PerformSourceRegistration()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

	for (const auto& PercSource : SourcesToRegister)
	{
		AActor* SourceActor = PercSource.Source.Get();
		if (SourceActor)
		{
			Senses[PercSource.SenseId]->RegisterSource(*SourceActor);
		}
	}

	SourcesToRegister.Reset();
}

void UAIPerceptionSystem::OnNewListener(const FPerceptionListener& NewListener)
{
	UAISense** SenseInstance = Senses.GetData();
	for (int32 SenseIndex = 0; SenseIndex < Senses.Num(); ++SenseIndex, ++SenseInstance)
	{
		if (*SenseInstance != NULL)
		{
			(*SenseInstance)->OnNewListener(NewListener);
		}
	}
}

void UAIPerceptionSystem::OnListenerUpdate(const FPerceptionListener& UpdatedListener)
{
	UAISense** SenseInstance = Senses.GetData();
	for (int32 SenseIndex = 0; SenseIndex < Senses.Num(); ++SenseIndex, ++SenseInstance)
	{
		if (*SenseInstance != NULL)
		{
			(*SenseInstance)->OnListenerUpdate(UpdatedListener);
		}
	}
}

void UAIPerceptionSystem::ManagerTick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

	// if no new stimuli
	// and it's not time to remove stimuli from "know events"

	UWorld* World = GEngine->GetWorldFromContextObject(GetOuter());
	check(World);
	// cache it
	CurrentTime = World->GetTimeSeconds();

	bool bNeedsUpdate = false;

	if (SourcesToRegister.Num() > 0)
	{
		PerformSourceRegistration();
	}

	{
		UAISense** SenseInstance = Senses.GetData();
		for (int32 SenseIndex = 0; SenseIndex < Senses.Num(); ++SenseIndex, ++SenseInstance)
		{
			bNeedsUpdate |= *SenseInstance != NULL && (*SenseInstance)->ProgressTime(DeltaSeconds);
		}
	}

	if (bNeedsUpdate)
	{
		// first update cached location of all listener, and remove invalid listeners
		for (AIPerception::FListenerMap::TIterator ListenerIt(ListenerContainer); ListenerIt; ++ListenerIt)
		{
			if (ListenerIt->Value.Listener.IsValid())
			{
				ListenerIt->Value.CacheLocation();
			}
			else
			{
				OnListenerRemoved(ListenerIt->Value);
				ListenerIt.RemoveCurrent();
			}
		}

		UAISense** SenseInstance = Senses.GetData();
		for (int32 SenseIndex = 0; SenseIndex < Senses.Num(); ++SenseIndex, ++SenseInstance)
		{
			if (*SenseInstance != NULL)
			{
				(*SenseInstance)->Tick();
			}
		}
	}

	/** no point in soring in no new stimuli was processed */
	bool bStimuliDelivered = DeliverDelayedStimuli(bNeedsUpdate ? RequiresSorting : NoNeedToSort);

	if (bNeedsUpdate || bStimuliDelivered)
	{
		for (AIPerception::FListenerMap::TIterator ListenerIt(ListenerContainer); ListenerIt; ++ListenerIt)
		{
			check(ListenerIt->Value.Listener.IsValid())
			if (ListenerIt->Value.HasAnyNewStimuli())
			{
				ListenerIt->Value.ProcessStimuli();
			}
		}
	}
}

void UAIPerceptionSystem::AgeStimuli()
{
	// age all stimuli in all listeners by PerceptionAgingRate
	const float ConstPerceptionAgingRate = PerceptionAgingRate;

	for (AIPerception::FListenerMap::TIterator ListenerIt(ListenerContainer); ListenerIt; ++ListenerIt)
	{
		FPerceptionListener& Listener = ListenerIt->Value;
		if (Listener.Listener.IsValid())
		{
			Listener.Listener->AgeStimuli(ConstPerceptionAgingRate);
		}
	}
}

UAIPerceptionSystem* UAIPerceptionSystem::GetCurrent(UObject* WorldContextObject)
{
	UWorld* World = Cast<UWorld>(WorldContextObject);

	if (World == NULL && WorldContextObject != NULL)
	{
		World = GEngine->GetWorldFromContextObject(WorldContextObject);
	}

	if (World && World->GetAISystem())
	{
		check(Cast<UAISystem>(World->GetAISystem()));
		UAISystem* AISys = (UAISystem*)(World->GetAISystem());

		return AISys->GetPerceptionSystem();
	}

	return NULL;
}

void UAIPerceptionSystem::UpdateListener(UAIPerceptionComponent* Listener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

    if (Listener == NULL || Listener->IsPendingKill())
    {
        return;
    }

    const int32 ListenerId = Listener->GetListenerId();

    if (ListenerId != AIPerception::InvalidListenerId)
    {
		FPerceptionListener& ListenerEntry = ListenerContainer[ListenerId];
		ListenerEntry.UpdateListenerProperties(Listener);
		OnListenerUpdate(ListenerEntry);
	}
	else
	{
		const uint32 NewListenerId = NextFreeListenerId++;
		Listener->StoreListenerId(NewListenerId);
        ListenerContainer.Add(NewListenerId, FPerceptionListener(Listener));
		
		OnNewListener(ListenerContainer[NewListenerId]);
    }
}

void UAIPerceptionSystem::UnregisterListener(UAIPerceptionComponent* Listener)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PerceptionSys);

    if (Listener != NULL)
    {
		const int32 ListenerId = Listener->GetListenerId();

		// can already be removed from ListenerContainer as part of cleaning up 
		// listeners with invalid WeakObjectPtr to UAIPerceptionComponent
		if (ListenerId != AIPerception::InvalidListenerId && ListenerContainer.Contains(ListenerId))
		{
			check(ListenerContainer[ListenerId].Listener.IsValid() == false
				|| ListenerContainer[ListenerId].Listener.Get() == Listener);
			OnListenerRemoved(ListenerContainer[ListenerId]);
			ListenerContainer.Remove(ListenerId);

			// make it as unregistered
			Listener->StoreListenerId(AIPerception::InvalidListenerId);
		}
	}
}

void UAIPerceptionSystem::OnListenerRemoved(const FPerceptionListener& NewListener)
{
	UAISense** SenseInstance = Senses.GetData();
	for (int32 SenseIndex = 0; SenseIndex < Senses.Num(); ++SenseIndex, ++SenseInstance)
	{
		if (*SenseInstance != NULL)
		{
			(*SenseInstance)->OnListenerRemoved(NewListener);
		}
	}
}

void UAIPerceptionSystem::RegisterDelayedStimulus(uint32 ListenerId, float Delay, AActor* Instigator, const FAIStimulus& Stimulus)
{
	FDelayedStimulus DelayedStimulus;
	DelayedStimulus.DeliveryTimestamp = CurrentTime + Delay;
	DelayedStimulus.ListenerId = ListenerId;
	DelayedStimulus.Instigator = Instigator;
	DelayedStimulus.Stimulus = Stimulus;
	DelayedStimuli.Add(DelayedStimulus);
}

bool UAIPerceptionSystem::DeliverDelayedStimuli(UAIPerceptionSystem::EDelayedStimulusSorting Sorting)
{
	struct FTimestampSort
	{ 
		bool operator()(const FDelayedStimulus& A, const FDelayedStimulus& B) const
		{
			return A.DeliveryTimestamp < B.DeliveryTimestamp;
		}
	};

	if (DelayedStimuli.Num() <= 0)
	{
		return false;
	}

	if (Sorting == RequiresSorting)
	{
		DelayedStimuli.Sort(FTimestampSort());
	}

	int Index = 0;
	while (Index < DelayedStimuli.Num() && DelayedStimuli[Index].DeliveryTimestamp < CurrentTime)
	{
		FDelayedStimulus& DelayedStimulus = DelayedStimuli[Index];
		
		if (DelayedStimulus.ListenerId != AIPerception::InvalidListenerId && ListenerContainer.Contains(DelayedStimulus.ListenerId))
		{
			FPerceptionListener& ListenerEntry = ListenerContainer[DelayedStimulus.ListenerId];
			// this has been already checked during tick, so if it's no longer the case then it's a bug
			check(ListenerEntry.Listener.IsValid());

			// deliver
			ListenerEntry.RegisterStimulus(DelayedStimulus.Instigator.Get(), DelayedStimulus.Stimulus);
		}

		++Index;
	}

	DelayedStimuli.RemoveAt(0, Index, /*bAllowShrinking=*/false);

	return Index > 0;
}

void UAIPerceptionSystem::MakeNoiseImpl(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation)
{
	check(NoiseMaker != NULL || NoiseInstigator != NULL);

	UWorld* World = NoiseMaker ? NoiseMaker->GetWorld() : NoiseInstigator->GetWorld();

	UAIPerceptionSystem::OnEvent(World, FAINoiseEvent(NoiseInstigator ? NoiseInstigator : NoiseMaker
			, NoiseLocation
			, Loudness));
}