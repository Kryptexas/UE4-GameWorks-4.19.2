// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseImplementation.h"
#include "AISenseImplementation_Touch.generated.h"

USTRUCT()
struct AIMODULE_API FAITouchEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UAISenseImplementation_Touch FSenseClass;

	FVector Location;
	
	UPROPERTY()
	class AActor* TouchReceiver;

	UPROPERTY()
	class AActor* OtherActor;
		
	FAITouchEvent(){}
	
	FAITouchEvent(class AActor* InTouchReceiver, class AActor* InOtherActor, const FVector& EventLocation)
		: Location(EventLocation), TouchReceiver(InTouchReceiver), OtherActor(InOtherActor)
	{
	}
};

UCLASS(ClassGroup=AI)
class AIMODULE_API UAISenseImplementation_Touch : public UAISenseImplementation
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FAITouchEvent> RegisteredEvents;

public:
	FORCEINLINE static FAISenseId GetSenseIndex() { return FAISenseId(ECorePerceptionTypes::Touch); }
		
	void RegisterEvent(const FAITouchEvent& Event);	

protected:
	virtual float Update() override;
};
