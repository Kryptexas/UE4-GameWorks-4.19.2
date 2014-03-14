// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

APainCausingVolume::APainCausingVolume(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	bPainCausing = true;
	DamageType = UDamageType::StaticClass();
	DamagePerSec = 1.0f;
	bEntryPain = true;
	PainInterval = 1.0f;
	bWantsInitialize = true;
}

void APainCausingVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	BACKUP_bPainCausing	= bPainCausing;
}

void APainCausingVolume::Reset()
{
	bPainCausing = BACKUP_bPainCausing;
	ForceNetUpdate();
}

void APainCausingVolume::ActorEnteredVolume(AActor* Other)
{
	Super::ActorEnteredVolume(Other);
	if ( bPainCausing && bEntryPain && Other->bCanBeDamaged )
	{
		CausePainTo(Other);
	}

	// Start timer if none is active
	if (!GetWorldTimerManager().IsTimerActive(this, &APainCausingVolume::PainTimer))
	{
		GetWorldTimerManager().SetTimer(this, &APainCausingVolume::PainTimer, PainInterval, true);
	}
}

void APainCausingVolume::PainTimer()
{
	if (bPainCausing)
	{
		TArray<AActor*> TouchingActors;
		GetOverlappingActors(TouchingActors, APawn::StaticClass());

		for (int32 iTouching = 0; iTouching < TouchingActors.Num(); ++iTouching)
		{
			AActor* const A = TouchingActors[iTouching];
			if (A && !A->IsPendingKill())
			{
				// @todo physicsVolume This won't work for other actor. Need to fix it properly
				if ( A->bCanBeDamaged && (Cast<APawn>(A) && Cast<APawn>(A)->GetPawnPhysicsVolume() == this) )
				{
					CausePainTo(A);
				}
			}
		}

		// Stop timer if nothing is overlapping us
		if (TouchingActors.Num() == 0)
		{
			GetWorldTimerManager().ClearTimer(this, &APainCausingVolume::PainTimer);
		}
	}
}

void APainCausingVolume::CausePainTo(AActor* Other)
{
	if (DamagePerSec > 0.f)
	{
		TSubclassOf<UDamageType> DmgTypeClass = DamageType ? *DamageType : UDamageType::StaticClass();
		Other->TakeDamage(DamagePerSec*PainInterval, FDamageEvent(DmgTypeClass), DamageInstigator, this);
	}
}

