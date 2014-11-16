// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense.h"
#include "AISense_Touch.generated.h"

USTRUCT()
struct AIMODULE_API FAITouchEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UAISense_Touch FSenseClass;

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
class AIMODULE_API UAISense_Touch : public UAISense
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FAITouchEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FAITouchEvent& Event);	

protected:
	virtual float Update() override;
};
