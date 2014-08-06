// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIPerceptionTypes.h"
#include "AIPerceptionSystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAIPerception, Warning, All);

/**
 *	By design checks perception between hostile teams
 */
UCLASS(ClassGroup=AI, config=Game)
class AIMODULE_API UAIPerceptionSystem : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
	
protected:	
	AIPerception::FListenerMap ListenerContainer;

	UPROPERTY()
	TArray<class UAISense*> Senses;

	UPROPERTY(config)
	float PerceptionAgingRate;

	struct FDelayedStimulus
	{
		float DeliveryTimestamp;
		uint32 ListenerId;
		TWeakObjectPtr<AActor> Instigator;
		FAIStimulus Stimulus;
	};

	TArray<FDelayedStimulus> DelayedStimuli;

	/** Primary tick function */
	/*UPROPERTY()
	struct FActorTickFunction PrimaryActorTick;*/

public:
	/** UObject begin */
	virtual void PostInitProperties() override;
	/* UObject end */

	/** Registers listener if not registered */
	void UpdateListener(class UAIPerceptionComponent* Listener);
	void UnregisterListener(class UAIPerceptionComponent* Listener);

	template<typename FEventClass>
	void OnEvent(const FEventClass& Event)
	{
		check(Senses[FEventClass::FSenseClass::GetSenseIndex()]);
		((typename FEventClass::FSenseClass*)Senses[FEventClass::FSenseClass::GetSenseIndex()])->RegisterEvent(Event);
	}

	template<typename FEventClass>
	static void OnEvent(UWorld* World, const FEventClass& Event)
	{
		UAIPerceptionSystem* PerceptionSys = GetCurrent(World);
		if (PerceptionSys != NULL)
		{
			PerceptionSys->OnEvent(Event);
		}
	}

	void RegisterSource(FAISenseId SenseIndex, class AActor* SourceActor);

	void ManagerTick(float DeltaSeconds);

	void RegisterDelayedStimulus(uint32 ListenerId, float Delay, AActor* Instigator, const FAIStimulus& Stimulus);

	static UAIPerceptionSystem* GetCurrent(class UObject* WorldContextObject);

	static void MakeNoiseImpl(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation);

protected:
	enum EDelayedStimulusSorting 
	{
		RequiresSorting,
		NoNeedToSort,
	};
	/** sorts DelayedStimuli and delivers all the ones that are no longer "in the future"
	 *	@return true if any stimuli has become "current" stimuli (meaning being no longer in future) */
	bool DeliverDelayedStimuli(EDelayedStimulusSorting Sorting);
	void OnNewListener(const FPerceptionListener& NewListener);
	void OnListenerUpdate(const FPerceptionListener& UpdatedListener);
	void OnListenerRemoved(const FPerceptionListener& UpdatedListener);

	void AgeStimuli();

	friend class UAISense;
	FORCEINLINE AIPerception::FListenerMap& GetListenersMap() { return ListenerContainer; }

private:
	uint32 NextFreeListenerId;
	/** cached world's timestamp */
	float CurrentTime;
};