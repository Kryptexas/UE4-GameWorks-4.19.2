// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseImplementation.h"
#include "AISenseImplementation_Damage.generated.h"

USTRUCT()
struct AIMODULE_API FAIDamageEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UAISenseImplementation_Damage FSenseClass;

	float Amount;
	FVector Location;
	FVector HitLocation;
	
	UPROPERTY()
	AActor* DamagedActor;

	UPROPERTY()
	AActor* Instigator;
	
	FAIDamageEvent(){}

	FAIDamageEvent(class AActor* InDamagedActor, class AActor* InInstigator, float DamageAmount, const FVector& EventLocation, const FVector& InHitLocation = FAISystem::InvalidLocation)
		: Amount(DamageAmount), Location(EventLocation), HitLocation(InHitLocation), DamagedActor(InDamagedActor), Instigator(InInstigator)
	{
		if (FAISystem::IsValidLocation(InHitLocation))
		{
			HitLocation = EventLocation;
		}
	}
};

UCLASS(ClassGroup=AI)
class AIMODULE_API UAISenseImplementation_Damage : public UAISenseImplementation
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FAIDamageEvent> RegisteredEvents;

public:
	FORCEINLINE static FAISenseId GetSenseIndex() { return FAISenseId(ECorePerceptionTypes::Damage); }
		
	void RegisterEvent(const FAIDamageEvent& Event);	

protected:
	virtual float Update() override;
};
