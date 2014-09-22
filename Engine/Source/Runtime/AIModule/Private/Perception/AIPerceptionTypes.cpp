// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"

// mind that this needs to be > 0 since checks are done if Age < FAIStimulus::NeverHappenedAge
// @todo maybe should be a function (IsValidAge)
const float FAIStimulus::NeverHappenedAge = FLT_MAX;

//----------------------------------------------------------------------//
// FPerceptionListener
//----------------------------------------------------------------------//
const FPerceptionListener FPerceptionListener::NullListener((UAIPerceptionComponent*)NULL);

FPerceptionListener::FPerceptionListener(class UAIPerceptionComponent* InListener) 
	: Listener(InListener)
	, PeripheralVisionAngleCos(1.f)
	, HearingRangeSq(-1.f)
	, LOSHearingRangeSq(-1.f)
	, SightRadiusSq(-1.f)
	, LoseSightRadiusSq(-1.f)
	, ListenerId(AIPerception::InvalidListenerId)
{
	if (InListener != NULL)
	{
		UpdateListenerProperties(InListener);
		ListenerId = InListener->GetListenerId();
	}
}

void FPerceptionListener::CacheLocation()
{
	if (Listener.IsValid())
	{
		Listener->GetLocationAndDirection(CachedLocation, CachedDirection);
	}
}

void FPerceptionListener::UpdateListenerProperties(UAIPerceptionComponent* InListener)
{
	check(InListener != NULL);
	verify(InListener == Listener.Get());

	// using InListener rather then Listener to avoid slight overhead of TWeakObjectPtr
	TeamIdentifier = InListener->GetTeamIdentifier();
	Filter = InListener->GetPerceptionFilter();

	PeripheralVisionAngleCos = FMath::Cos(FMath::DegreesToRadians(InListener->GetPeripheralVisionAngle()));
	HearingRangeSq = FMath::Square(InListener->GetHearingRange());
	LOSHearingRangeSq = FMath::Square(InListener->GetLOSHearingRange());
	SightRadiusSq = FMath::Square(InListener->GetSightRadius());
	LoseSightRadiusSq = FMath::Square(InListener->GetLoseSightRadius());
}

void FPerceptionListener::RegisterStimulus(AActor* Source, const FAIStimulus& Stimulus)
{
	bHasStimulusToProcess = true;
	Listener->RegisterStimulus(Source, Stimulus);
}

void FPerceptionListener::ProcessStimuli()
{
	ensure(bHasStimulusToProcess);
	Listener->ProcessStimuli();
	bHasStimulusToProcess = false;
}

FName FPerceptionListener::GetBodyActorName() const 
{
	const AActor* OwnerActor = Listener.IsValid() ? Listener->GetBodyActor() : NULL;
	return OwnerActor ? OwnerActor->GetFName() : NAME_None;
}

const AActor* FPerceptionListener::GetBodyActor() const 
{ 
	return Listener.IsValid() ? Listener->GetBodyActor() : NULL; 
}

const IGenericTeamAgentInterface* FPerceptionListener::GetTeamAgent() const
{
	const UAIPerceptionComponent* PercComponent = Listener.Get();
	if (PercComponent == NULL)
	{	// This could be NULL if the Listener is pending kill; in order to get the pointer ANYWAY, we'd need to use
		// Listener.Get(true) instead.  This issue was hit when using KillPawns cheat at the same moment that pawns
		// were spawning into the world.
		return NULL;
	}

	const AActor* OwnerActor = PercComponent->GetOwner();
	const IGenericTeamAgentInterface* OwnerTeamAgent = InterfaceCast<const IGenericTeamAgentInterface>(OwnerActor);
	return OwnerTeamAgent != NULL ? OwnerTeamAgent : InterfaceCast<const IGenericTeamAgentInterface>(PercComponent->GetBodyActor());
}