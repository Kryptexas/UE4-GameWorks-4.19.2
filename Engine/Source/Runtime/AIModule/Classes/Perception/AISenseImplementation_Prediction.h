// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseImplementation.h"
#include "AISenseImplementation_Prediction.generated.h"

USTRUCT()
struct AIMODULE_API FAIPredictionEvent
{	
	GENERATED_USTRUCT_BODY()

	typedef class UAISenseImplementation_Prediction FSenseClass;
	
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
class AIMODULE_API UAISenseImplementation_Prediction : public UAISenseImplementation
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FAIPredictionEvent> RegisteredEvents;

public:
	FORCEINLINE static FAISenseId GetSenseIndex() { return FAISenseId(ECorePerceptionTypes::Prediction); }
		
	void RegisterEvent(const FAIPredictionEvent& Event);	

protected:
	virtual float Update() override;
};
