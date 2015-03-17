// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense.h"
#include "AISense_Prediction.generated.h"

class UAISense_Prediction;

USTRUCT()
struct AIMODULE_API FAIPredictionEvent
{	
	GENERATED_BODY()

	typedef UAISense_Prediction FSenseClass;
	
	UPROPERTY()
	AActor* Requestor;

	UPROPERTY()
	AActor* PredictedActor;

	float TimeToPredict;
		
	FAIPredictionEvent(){}
	
	FAIPredictionEvent(AActor* InRequestor, AActor* InPredictedActor, float PredictionTime)
		: Requestor(InRequestor), PredictedActor(InPredictedActor), TimeToPredict(PredictionTime)
	{
	}
};

UCLASS(ClassGroup=AI)
class AIMODULE_API UAISense_Prediction : public UAISense
{
	GENERATED_BODY()
public:
	UAISense_Prediction(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	TArray<FAIPredictionEvent> RegisteredEvents;

public:		
	void RegisterEvent(const FAIPredictionEvent& Event);	

protected:
	virtual float Update() override;
};
