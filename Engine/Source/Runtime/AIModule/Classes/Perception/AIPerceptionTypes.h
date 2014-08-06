// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AIPerceptionTypes.generated.h"

class UAIPerceptionComponent;

UENUM()
namespace ECorePerceptionTypes
{
	enum Type
	{
		Sight,
		Hearing,
		Damage,
		Touch,
		Team,	// information from teammates 
		Prediction,

		MAX UMETA(Hidden)
	};
}

typedef ECorePerceptionTypes::Type FAISenseId;

namespace AIPerception
{
	static const FAISenseId InvalidSenseIndex = FAISenseId(-1);
	static const uint32 InvalidListenerId = 0;
}

struct FPerceptionChannelFilter
{
	static const uint32 AllChannels = uint32(-1);

	uint32 AcceptedChannelsMask;

	// by default accept all
	FPerceptionChannelFilter() : AcceptedChannelsMask(AllChannels)
	{}

	FORCEINLINE FPerceptionChannelFilter& FilterOutChannel(FAISenseId Channel)
	{
		AcceptedChannelsMask &= ~(1 << Channel);
		return *this;
	}

	FORCEINLINE FPerceptionChannelFilter& AcceptChannel(FAISenseId Channel)
	{
		AcceptedChannelsMask |= (1 << Channel);
		return *this;
	}

	FORCEINLINE bool ShouldRespondToChannel(FAISenseId Channel) const
	{
		return (AcceptedChannelsMask & (1 << Channel)) != 0;
	}
};

struct AIMODULE_API FAIStimulus
{
	static const float NeverHappenedAge;

	enum FResult
	{
		SensingSucceeded,
		SensingFailed
	};

protected:
	float Age;
public:
	float Strength;
	FVector StimulusLocation;
	FVector ReceiverLocation;
	FAISenseId Type;
protected:
	uint32 bLastSensingResult:1; // currently used only for marking failed sight tests
	
public:
	FAIStimulus(FAISenseId SenseType, float StimulusStrength, const FVector& InStimulusLocation, const FVector& InReceiverLocation, FResult Result = SensingSucceeded, float StimulusAge = 0.f)
		: Age(StimulusAge), Strength(Result == SensingSucceeded ? StimulusStrength : -1.f)
		, StimulusLocation(InStimulusLocation)
		, ReceiverLocation(InReceiverLocation), Type(SenseType), bLastSensingResult(Result == SensingSucceeded)
	{}

	// default constructor
	FAIStimulus()
		: Age(NeverHappenedAge), Strength(-1.f), StimulusLocation(FAISystem::InvalidLocation)
		, ReceiverLocation(FAISystem::InvalidLocation), Type(AIPerception::InvalidSenseIndex), bLastSensingResult(false)
	{}

	FORCEINLINE float GetAge() const { return Strength > 0 ? Age : NeverHappenedAge; }
	FORCEINLINE void AgeStimulus(float ConstPerceptionAgingRate) { Age += ConstPerceptionAgingRate; }
	FORCEINLINE bool WasSuccessfullySensed() const { return bLastSensingResult; }
	FORCEINLINE void MarkNoLongerSensed() { bLastSensingResult = false; }
};

struct FPerceptionListener
{
	TWeakObjectPtr<UAIPerceptionComponent> Listener;

	FPerceptionChannelFilter Filter;

	FVector CachedLocation;
	FVector CachedDirection;

	FGenericTeamId TeamIdentifier;

	uint32 bHasStimulusToProcess : 1;

	float PeripheralVisionAngleCos;
	float HearingRangeSq; 
	float LOSHearingRangeSq;
	float SightRadiusSq;
	float LoseSightRadiusSq;

	FPerceptionListener(UAIPerceptionComponent* InListener = NULL);

	void UpdateListenerProperties(UAIPerceptionComponent* Listener);

	bool operator==(const UAIPerceptionComponent* Other) const { return Listener.Get() == Other; }
	bool operator==(const FPerceptionListener& Other) const { return Listener == Other.Listener; }

	void CacheLocation();

	void RegisterStimulus(AActor* Source, const FAIStimulus& Stimulus);

	FORCEINLINE bool HasAnyNewStimuli() const { return bHasStimulusToProcess; }
	void ProcessStimuli();

	FORCEINLINE bool HasSense(FAISenseId SenseIndex) const { return Filter.ShouldRespondToChannel(SenseIndex); }

	// used to remove "dead" listeners
	static const FPerceptionListener NullListener;

	FORCEINLINE uint32 GetListenerId() const { return ListenerId; }

	FName GetCollisionActorName() const;

private:
	uint32 ListenerId;

	friend class UAIPerceptionSystem;
	FORCEINLINE void SetListenerId(uint32 InListenerId) { ListenerId = InListenerId;}
};

namespace AIPerception
{
	typedef TMap<uint32, FPerceptionListener> FListenerMap;
}
