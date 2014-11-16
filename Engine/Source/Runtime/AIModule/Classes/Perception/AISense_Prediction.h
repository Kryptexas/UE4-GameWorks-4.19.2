// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense.h"
#include "AISense_Prediction.generated.h"

USTRUCT()
struct AIMODULE_API FAIPredictionEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UAISense_Prediction FSenseClass;
	
	UPROPERTY()
	class AActor* Requestor;

	UPROPERTY()
	class AActor* PredictedActor;

	float TimeToPredict;
		
	FAIPredictionEvent(){}
	
	FAIPredictionEvent(class AActor* InRequestor, class AActor* InPredictedActor, float PredictionTime)
		: Requestor(InRequestor), PredictedActor(InPredictedActor), TimeToPredict(PredictionTime)
	{
	}
};

UCLASS(ClassGroup=AI)
class AIMODULE_API UAISense_Prediction : public UAISense
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FAIPredictionEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FAIPredictionEvent& Event);	

protected:
	virtual float Update() override;
};
