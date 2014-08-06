// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense.h"
#include "AISense_Sight.generated.h"

class IAISightTargetInterface;

namespace ESightPerceptionEventName
{
	enum Type
	{
		Undefined,
		GainedSight,
		LostSight
	};
}

USTRUCT()
struct AIMODULE_API FAISightEvent
{
	GENERATED_USTRUCT_BODY()

	typedef class UAISense_Sight FSenseClass;

	float Age;
	ESightPerceptionEventName::Type EventType;	

	UPROPERTY()
	class AActor* SeenActor;

	UPROPERTY()
	class AActor* Observer;

	FAISightEvent(){}

	FAISightEvent(class AActor* InSeenActor, class AActor* InObserver, ESightPerceptionEventName::Type InEventType)
		: Age(0.f), EventType(InEventType), SeenActor(InSeenActor), Observer(InObserver)
	{
	}
};

struct FAISightTarget
{
	typedef FName FTargetId;
	static const FTargetId InvalidTargetId;

	TWeakObjectPtr<AActor> Target;
	IAISightTargetInterface* SightTargetInterface;
	FGenericTeamId TeamId;
	FTargetId TargetId;

	FAISightTarget(class AActor* InTarget = NULL, FGenericTeamId InTeamId = FGenericTeamId::NoTeam);

	FORCEINLINE FVector GetLocationSimple() const
	{
		return Target.IsValid() ? Target->GetActorLocation() : FVector::ZeroVector;
	}
};

struct FAISightQuery
{
	uint32 ObserverId;
	FAISightTarget::FTargetId TargetId;

	float Age;
	float Score;
	float Importance;

	uint32 bLastResult : 1;

	FAISightQuery(uint32 ListenerId = AIPerception::InvalidListenerId, FAISightTarget::FTargetId Target = FAISightTarget::InvalidTargetId)
		: ObserverId(ListenerId), TargetId(Target), Age(0), Score(0), Importance(0), bLastResult(false)
	{
	}

	void RecalcScore()
	{
		Score = Age + Importance;
	}

	class FSortPredicate
	{
	public:
		FSortPredicate()
		{}

		bool operator()(const FAISightQuery& A, const FAISightQuery& B) const
		{
			return A.Score > B.Score;
		}
	};
};


UCLASS(ClassGroup=AI, config=Game)
class AIMODULE_API UAISense_Sight : public UAISense
{
	GENERATED_UCLASS_BODY()

	TMap<FName, FAISightTarget> ObservedTargets;

	TArray<FAISightQuery> SightQueryQueue;

protected:
	UPROPERTY(config)
	int32 MaxTracesPerTick;

	UPROPERTY(config)
	float HighImportanceQueryDistanceThreshold;

	float HighImportanceDistanceSquare;

	UPROPERTY(config)
	float MaxQueryImportance;

	UPROPERTY(config)
	float SightLimitQueryImportance;

public:

	virtual void PostInitProperties() override;

	FORCEINLINE static FAISenseId GetSenseIndex() { return FAISenseId(ECorePerceptionTypes::Sight); }

	void RegisterEvent(const FAISightEvent& Event);	

	virtual void RegisterSource(class AActor* SourceActor) override;
	
protected:
	virtual float Update() override;

	void OnNewListenerImpl(const FPerceptionListener& NewListener);
	void OnListenerUpdateImpl(const FPerceptionListener& UpdatedListener);
	void OnListenerRemovedImpl(const FPerceptionListener& UpdatedListener);	

	enum FQueriesOperationPostProcess
	{
		DontSort,
		Sort
	};
	void RemoveAllQueriesByListener(const FPerceptionListener& Listener, FQueriesOperationPostProcess PostProcess);
	void RemoveAllQueriesToTarget(const FName& TargetId, FQueriesOperationPostProcess PostProcess);

	void RegisterTarget(AActor* TargetActor, FQueriesOperationPostProcess PostProcess);

	FORCEINLINE void SortQueries() { SightQueryQueue.Sort(FAISightQuery::FSortPredicate()); }

	float CalcQueryImportance(const FPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq) const;
};
