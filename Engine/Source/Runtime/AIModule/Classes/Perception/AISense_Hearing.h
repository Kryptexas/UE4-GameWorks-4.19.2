// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense.h"
#include "AISense_Hearing.generated.h"

USTRUCT()
struct AIMODULE_API FAINoiseEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UAISense_Hearing FSenseClass;

	float Age;
	FVector NoiseLocation;
	float Loudness;
	FGenericTeamId TeamIdentifier;
	
	UPROPERTY()
	AActor* Instigator;
	
	FAINoiseEvent(){}
	FAINoiseEvent(class AActor* InInstigator, const FVector& InNoiseLocation, float InLoudness = 1.f);
};

UCLASS(ClassGroup=AI)
class AIMODULE_API UAISense_Hearing : public UAISense
{
	GENERATED_UCLASS_BODY()
		
protected:
	UPROPERTY()
	TArray<FAINoiseEvent> NoiseEvents;

public:
	FORCEINLINE static FAISenseId GetSenseIndex() { return FAISenseId(ECorePerceptionTypes::Hearing); }
	
	void RegisterEvent(const FAINoiseEvent& Event);	

protected:
	virtual float Update() override;
};
