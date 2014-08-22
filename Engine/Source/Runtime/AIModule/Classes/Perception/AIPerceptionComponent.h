// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AISystem.h"
#include "AIPerceptionSystem.h"

#include "AIPerceptionComponent.generated.h"

class AAIController;

struct AIMODULE_API FActorPerceptionInfo
{
	TWeakObjectPtr<AActor> Target;

	FAIStimulus LastSensedStimuli[ECorePerceptionTypes::MAX];

	/** if != MAX indicated the sense that takes precedense over other senses when it comes
		to determining last stimulus location */
	ECorePerceptionTypes::Type DominantSense;

	/** indicates whether this Actor is hostile to perception holder */
	uint32 bIsHostile : 1;
	
	FActorPerceptionInfo(AActor* InTarget = NULL)
		: Target(InTarget), DominantSense(ECorePerceptionTypes::MAX)
	{}

	FORCEINLINE_DEBUGGABLE FVector GetLastStimulusLocation(float* OptionalAge = NULL) const 
	{
		FVector Location(FAISystem::InvalidLocation);
		float BestAge = FLT_MAX;
		for (int32 Sense = 0; Sense < ECorePerceptionTypes::MAX; ++Sense)
		{
			const float Age = LastSensedStimuli[Sense].GetAge();
			if (Age >= 0 && (Age < BestAge 
				|| (Sense == DominantSense && LastSensedStimuli[Sense].WasSuccessfullySensed())))
			{
				BestAge = Age;
				Location = LastSensedStimuli[Sense].StimulusLocation;
			}
		}

		if (OptionalAge)
		{
			*OptionalAge = BestAge;
		}

		return Location;
	}

	/** @note will return FAISystem::InvalidLocation if given sense has never registered related Target actor */
	FORCEINLINE FVector GetStimulusLocation(FAISenseId Sense) const
	{
		return LastSensedStimuli[Sense].GetAge() < FAIStimulus::NeverHappenedAge ? LastSensedStimuli[Sense].StimulusLocation : FAISystem::InvalidLocation;
	}

	FORCEINLINE FVector GetReceiverLocation(FAISenseId Sense) const
	{
		return LastSensedStimuli[Sense].GetAge() < FAIStimulus::NeverHappenedAge ? LastSensedStimuli[Sense].ReceiverLocation : FAISystem::InvalidLocation;
	}

	FORCEINLINE bool IsSenseRegistered(FAISenseId Sense) const
	{
		return LastSensedStimuli[Sense].WasSuccessfullySensed() && (LastSensedStimuli[Sense].GetAge() < FAIStimulus::NeverHappenedAge);
	}
	
	/** takes all "newer" info from Other and absorbs it */
	void Merge(const FActorPerceptionInfo& Other);
};

/**
 *	AIPerceptionComponent is used to register as stimuli listener in AIPerceptionSystem
 *	and gathers registered stimuli. UpdatePerception is called when component gets new stimuli (batched)
 */
UCLASS(ClassGroup=AI, HideCategories=(Activation, Collision), meta=(BlueprintSpawnableComponent), config=Game)
class AIMODULE_API UAIPerceptionComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
	
	static const int32 InitialStimuliToProcessArraySize;

	typedef TMap<AActor*, FActorPerceptionInfo> TActorPerceptionContainer;

protected:
	/** Max distance at which a makenoise(1.0) loudness sound can be heard, regardless of occlusion */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AI)
	float HearingRange;

	/** Max distance at which a makenoise(1.0) loudness sound can be heard if unoccluded (LOSHearingThreshold should be > HearingThreshold) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AI)
	float LOSHearingRange;

	/** Maximum sight distance to notice a target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AI)
	float SightRadius;

	/** Maximum sight distance to see target that has been already seen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AI)
	float LoseSightRadius;

	/** How far to the side AI can see, in degrees. Use SetPeripheralVisionAngle to change the value at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AI)
	float PeripheralVisionAngle;

	UPROPERTY(Transient)
	AAIController* AIOwner;

	FPerceptionChannelFilter PerceptionFilter;

private:
	TActorPerceptionContainer PerceptualData;

	/** Indicated sense that takes precedence over other senses when determining sensed actor's location */
	UPROPERTY(config)
	TEnumAsByte<ECorePerceptionTypes::Type> DominantSense;

protected:	
	struct FStimulusToProcess
	{
		AActor* Source;
		FAIStimulus Stimulus;

		FStimulusToProcess(AActor* InSource, const FAIStimulus& InStimulus)
			: Source(InSource), Stimulus(InStimulus)
		{

		}
	};

	TArray<FStimulusToProcess> StimuliToProcess; 
	
	/** max age of stimulus to consider it "active" (e.g. target is visible) */
	float MaxActiveAge[ECorePerceptionTypes::MAX];

public:

	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;

	UFUNCTION()
	void OnOwnerEndPlay(EEndPlayReason::Type EndPlayReason);

	/** Sets PeripheralVisionAngle. Updates AIPercetionSystem's stimuli listener */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="AI")
	void SetPeripheralVisionAngle(const float NewPeripheralVisionAngle);

	/** Sets HearingRange. Updates AIPercetionSystem's stimuli listener */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="AI")
	void SetHearingRange(const float NewHearingRange);

	/** Sets LOSHearingRange. Updates AIPercetionSystem's stimuli listener */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="AI")
	void SetLOSHearingRange(const float NewLOSHearingRange);

	/** Sets SightRadius. Updates AIPercetionSystem's stimuli listener */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="AI")
	void SetSightRadius(const float NewSightRadius);

	FORCEINLINE float GetPeripheralVisionAngle() const { return PeripheralVisionAngle; }
	FORCEINLINE float GetHearingRange() const { return HearingRange; }
	FORCEINLINE float GetLOSHearingRange() const { return LOSHearingRange; }
	FORCEINLINE float GetSightRadius() const { return SightRadius; }	
	FORCEINLINE float GetLoseSightRadius() const { return LoseSightRadius; }	

	void GetLocationAndDirection(FVector& Location, FVector& Direction) const;
	const AActor* GetBodyActor() const;

	FORCEINLINE const FPerceptionChannelFilter GetPerceptionFilter() const { return PerceptionFilter; }

	FGenericTeamId GetTeamIdentifier() const;
	FORCEINLINE uint32 GetListenerId() const { return PerceptionListenerId; }

	FVector GetActorLocation(const AActor* Actor) const;
	FORCEINLINE const FActorPerceptionInfo* GetActorInfo(const AActor* Actor) const { return PerceptualData.Find(Actor); }
	FORCEINLINE TActorPerceptionContainer::TIterator GetPerceptualDataIterator() { return TActorPerceptionContainer::TIterator(PerceptualData); }
	FORCEINLINE TActorPerceptionContainer::TConstIterator GetPerceptualDataConstIterator() const { return TActorPerceptionContainer::TConstIterator(PerceptualData); }

	void GetHostileActors(TArray<const AActor*>& OutActors);

	// @note will stop on first age 0 stimulus
	const FActorPerceptionInfo* GetFreshestTrace(const FAISenseId Sense) const;
	
	void SetDominantSense(ECorePerceptionTypes::Type InDominantSense);
	FORCEINLINE ECorePerceptionTypes::Type GetDominantSense() const { return DominantSense; }

	void SetShouldSee(bool bNewValue);
	void SetShouldHear(bool bNewValue);
	void SetShouldSenseDamage(bool bNewValue);	

	/** Notifies AIPerceptionSystem to update data for this "stimuli listener" */
	void RequestStimuliListenerUpdate();

	void RegisterStimulus(AActor* Source, const FAIStimulus& Stimulus);
	void ProcessStimuli();
	void AgeStimuli(const float ConstPerceptionAgingRate);
	void ForgetActor(AActor* ActorToForget);

	float GetYoungestStimulusAge(const AActor* Source) const;
	bool HasAnyActiveStimulus(const AActor* Source) const;
	bool HasActiveStimulus(const AActor* Source, FAISenseId Sense) const;

	void DrawDebugInfo(class UCanvas* Canvas);
	
protected:

	void UpdatePerceptionFilter(FAISenseId Channel, bool bNewValue);
	TActorPerceptionContainer& GetPerceptualData() { return PerceptualData; }

	/** called to clean up on owner's end play or destruction */
	virtual void CleanUp();
	
private:
	uint32 PerceptionListenerId;

	friend class UAIPerceptionSystem;

	void StoreListenerId(uint32 InListenerId) { PerceptionListenerId = InListenerId; }

};

