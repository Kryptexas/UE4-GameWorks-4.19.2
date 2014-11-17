// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "Perception/AISightTargetInterface.h"
#include "Perception/AISense_Sight.h"

DECLARE_CYCLE_STAT(TEXT("Perception Sense: Sight"),STAT_AI_Sense_Sight,STATGROUP_AI);

static const int32 DefaultMaxTracesPerTick = 6;

//----------------------------------------------------------------------//
// helpers
//----------------------------------------------------------------------//
FORCEINLINE_DEBUGGABLE bool CheckIsTargetInSightPie(const FPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq)
{
	if (FVector::DistSquared(Listener.CachedLocation, TargetLocation) <= SightRadiusSq) 
	{
		const FVector DirectionToTarget = (TargetLocation - Listener.CachedLocation).UnsafeNormal();
		return FVector::DotProduct(DirectionToTarget, Listener.CachedDirection) > Listener.PeripheralVisionAngleCos;
	}

	return false;
}

//----------------------------------------------------------------------//
// FAISightTarget
//----------------------------------------------------------------------//
const FAISightTarget::FTargetId FAISightTarget::InvalidTargetId = NAME_None;

FAISightTarget::FAISightTarget(AActor* InTarget, FGenericTeamId InTeamId)
	: Target(InTarget), SightTargetInterface(NULL), TeamId(InTeamId)
{
	if (InTarget)
	{
		TargetId = InTarget->GetFName();
	}
	else
	{
		TargetId = InvalidTargetId;
	}
}

//----------------------------------------------------------------------//
// UAISense_Sight
//----------------------------------------------------------------------//
UAISense_Sight::UAISense_Sight(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, MaxTracesPerTick(DefaultMaxTracesPerTick)
	, HighImportanceQueryDistanceThreshold(300.f)
	, MaxQueryImportance(60.f)
	, SightLimitQueryImportance(10.f)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		OnNewListenerDelegate.BindUObject(this, &UAISense_Sight::OnNewListenerImpl);
		OnListenerUpdateDelegate.BindUObject(this, &UAISense_Sight::OnListenerUpdateImpl);
		OnListenerRemovedDelegate.BindUObject(this, &UAISense_Sight::OnListenerRemovedImpl);
	}
}

FORCEINLINE_DEBUGGABLE float UAISense_Sight::CalcQueryImportance(const FPerceptionListener& Listener, const FVector& TargetLocation, const float SightRadiusSq) const
{
	const float DistanceSq = FVector::DistSquared(Listener.CachedLocation, TargetLocation);
	return DistanceSq <= HighImportanceDistanceSquare ? MaxQueryImportance
		: FMath::Clamp((SightLimitQueryImportance - MaxQueryImportance) / SightRadiusSq * DistanceSq + MaxQueryImportance, 0.f, MaxQueryImportance);
}

void UAISense_Sight::PostInitProperties()
{
	Super::PostInitProperties();
	HighImportanceDistanceSquare = FMath::Square(HighImportanceQueryDistanceThreshold);
}

float UAISense_Sight::Update()
{
	static const FName NAME_AILineOfSight = FName(TEXT("AILineOfSight"));

	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight);

	const UWorld* World = GEngine->GetWorldFromContextObject(GetPerceptionSystem()->GetOuter());

	if (World == NULL)
	{
		return SuspendNextUpdate;
	}

	int32 TracesCount = 0;
	static const int32 InitialInvalidItemsSize = 16;
	TArray<int32> InvalidQueries;
	TArray<FAISightTarget::FTargetId> InvalidTargets;
	InvalidQueries.Reserve(InitialInvalidItemsSize);
	InvalidTargets.Reserve(InitialInvalidItemsSize);

	AIPerception::FListenerMap& ListenersMap = *GetListeners();

	FAISightQuery* SightQuery = SightQueryQueue.GetTypedData();
	for (int32 QueryIndex = 0; QueryIndex < SightQueryQueue.Num(); ++QueryIndex, ++SightQuery)
	{
		if (TracesCount < MaxTracesPerTick)
		{
			FPerceptionListener& Listener = ListenersMap[SightQuery->ObserverId];
			ensure(Listener.Listener.IsValid());
			FAISightTarget& Target = ObservedTargets[SightQuery->TargetId];
					
			const bool bTargetValid = Target.Target.IsValid();
			const bool bListenerValid = Listener.Listener.IsValid();

			// @todo figure out what should we do if not valid
			if (bTargetValid && bListenerValid)
			{
				AActor* TargetActor = Target.Target.Get();
				const FVector TargetLocation = TargetActor->GetActorLocation();
				const float SightRadiusSq = SightQuery->bLastResult ? Listener.LoseSightRadiusSq : Listener.SightRadiusSq;

				if (CheckIsTargetInSightPie(Listener, TargetLocation, SightRadiusSq))
				{
//					UE_VLOG_SEGMENT(Listener.Listener.Get()->GetOwner(), Listener.CachedLocation, TargetLocation, FColor::Green, TEXT("%s"), *(Target.TargetId.ToString()));

					FVector OutSeenLocation(0.f);
					// do line checks
					if (Target.SightTargetInterface != NULL)
					{
						int32 NumberOfLoSChecksPerformed = 0;
						if (Target.SightTargetInterface->CanBeSeenFrom(Listener.CachedLocation, OutSeenLocation, NumberOfLoSChecksPerformed, Listener.Listener->GetBodyActor()) == true)
						{
							Listener.RegisterStimulus(TargetActor, FAIStimulus(ECorePerceptionTypes::Sight, 1.f, OutSeenLocation, Listener.CachedLocation));
							SightQuery->bLastResult = true;
						}
						else
						{
//							UE_VLOG_LOCATION(Listener.Listener.Get()->GetOwner(), TargetLocation, 25.f, FColor::Red, TEXT(""));
							Listener.RegisterStimulus(TargetActor, FAIStimulus(ECorePerceptionTypes::Sight, 0.f, TargetLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
							SightQuery->bLastResult = false;
						}

						TracesCount += NumberOfLoSChecksPerformed;
					}
					else
					{
						// we need to do tests ourselves
						/*const bool bHit = World->LineTraceTest(Listener.CachedLocation, TargetLocation
							, FCollisionQueryParams(NAME_AILineOfSight, true, Listener.Listener->GetBodyActor())
							, FCollisionObjectQueryParams(ECC_WorldStatic));*/
						FHitResult HitResult;
						const bool bHit = World->LineTraceSingle(HitResult, Listener.CachedLocation, TargetLocation
							, FCollisionQueryParams(NAME_AILineOfSight, true, Listener.Listener->GetBodyActor())
							, FCollisionObjectQueryParams(ECC_WorldStatic));

						++TracesCount;

						if (bHit == false || (HitResult.Actor.IsValid() && HitResult.Actor->IsOwnedBy(TargetActor)))
						{
							Listener.RegisterStimulus(TargetActor, FAIStimulus(ECorePerceptionTypes::Sight, 1.f, TargetLocation, Listener.CachedLocation));
							SightQuery->bLastResult = true;
						}
						else
						{
//							UE_VLOG_LOCATION(Listener.Listener.Get()->GetOwner(), TargetLocation, 25.f, FColor::Red, TEXT(""));
							Listener.RegisterStimulus(TargetActor, FAIStimulus(ECorePerceptionTypes::Sight, 0.f, TargetLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
							SightQuery->bLastResult = false;
						}
					}
				}
				else
				{
//					UE_VLOG_SEGMENT(Listener.Listener.Get()->GetOwner(), Listener.CachedLocation, TargetLocation, FColor::Red, TEXT("%s"), *(Target.TargetId.ToString()));
					Listener.RegisterStimulus(TargetActor, FAIStimulus(ECorePerceptionTypes::Sight, 0.f, TargetLocation, Listener.CachedLocation, FAIStimulus::SensingFailed));
					SightQuery->bLastResult = false;
				}

				SightQuery->Importance = CalcQueryImportance(Listener, TargetLocation, SightRadiusSq);

				// restart query
				SightQuery->Age = 0.f;
			}
			else
			{
				// put this index to "to be removed" array
				InvalidQueries.Add(QueryIndex);
				if (bTargetValid == false)
				{
					InvalidTargets.AddUnique(SightQuery->TargetId);
				}
			}
		}
		else
		{
			// age unprocessed queries so that they can advance in the queue during next sort
			SightQuery->Age += 1.f;
		}

		SightQuery->RecalcScore();
	}

	if (InvalidQueries.Num() > 0)
	{
		for (int32 Index = InvalidQueries.Num() - 1; Index >= 0; --Index)
		{
			// removing with swapping here, since queue is going to be sorted anyway
			SightQueryQueue.RemoveAtSwap(InvalidQueries[Index], 1, /*bAllowShrinking*/false);
		}

		if (InvalidTargets.Num() > 0)
		{
			for (const auto& TargetId : InvalidTargets)
			{
				// remove affected queries
				RemoveAllQueriesToTarget(TargetId, DontSort);
				// remove target itself
				ObservedTargets.Remove(TargetId);
			}

			// remove holes
			ObservedTargets.Compact();
		}
	}

	// sort Sight Queries
	SortQueries();

	//return SightQueryQueue.Num() > 0 ? 1.f/6 : FLT_MAX;
	return 0.f;
}

void UAISense_Sight::RegisterEvent(const FAISightEvent& Event)
{

}

void UAISense_Sight::RegisterSource(AActor& SourceActor) 
{
	RegisterTarget(SourceActor, Sort);
}

void UAISense_Sight::RegisterTarget(AActor& TargetActor, FQueriesOperationPostProcess PostProcess)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight);
	
	FAISightTarget* SightTarget = ObservedTargets.Find(TargetActor.GetFName());
	
	if (SightTarget == NULL)
	{
		FAISightTarget NewSightTarget(&TargetActor);

		SightTarget = &(ObservedTargets.Add(NewSightTarget.TargetId, NewSightTarget));
		SightTarget->SightTargetInterface = InterfaceCast<IAISightTargetInterface>(&TargetActor);
	}

	// set/update data
	SightTarget->TeamId = FGenericTeamId::GetTeamIdentifier(&TargetActor);
	
	// generate all pairs and add them to current Sight Queries
	bool bNewQueriesAdded = false;
	AIPerception::FListenerMap& ListenersMap = *GetListeners();
	const FVector TargetLocation = TargetActor.GetActorLocation();

	for (AIPerception::FListenerMap::TConstIterator ItListener(ListenersMap); ItListener; ++ItListener)
	{
		const FPerceptionListener& Listener = ItListener->Value;
		const IGenericTeamAgentInterface* ListenersTeamAgent = Listener.GetTeamAgent();

		// @todo add configuration here
		if (Listener.HasSense(GetSenseIndex()) && (ListenersTeamAgent == NULL || ListenersTeamAgent->GetTeamAttitudeTowards(TargetActor) == ETeamAttitude::Hostile))
		{
			// create a sight query		
			FAISightQuery SightQuery(ItListener->Key, SightTarget->TargetId);
			SightQuery.Importance = CalcQueryImportance(ItListener->Value, TargetLocation, Listener.SightRadiusSq);

			SightQueryQueue.Add(SightQuery);
			bNewQueriesAdded = true;
		}
	}

	// sort Sight Queries
	if (PostProcess == Sort && bNewQueriesAdded)
	{
		SortQueries();
		RequestImmediateUpdate();
	}
}

void UAISense_Sight::OnNewListenerImpl(const FPerceptionListener& NewListener)
{
	if (NewListener.HasSense(GetSenseIndex()) == false)
	{
		return;
	}

	bool bNewQueriesAdded = false;

	const IGenericTeamAgentInterface* ListenersTeamAgent = NewListener.GetTeamAgent();

	// create sight queries with all legal targets
	for (TMap<FName, FAISightTarget>::TConstIterator ItTarget(ObservedTargets); ItTarget; ++ItTarget)
	{
		const AActor* TargetActor = ItTarget->Value.GetTargetActor();
		if (TargetActor == NULL)
		{
			continue;
		}

		// @todo this should be configurable - some AI might want to observe Neutrals and Friendlies as well
		if (ListenersTeamAgent == NULL || ListenersTeamAgent->GetTeamAttitudeTowards(*TargetActor) == ETeamAttitude::Hostile)
		{
			// create a sight query		
			FAISightQuery SightQuery(NewListener.GetListenerId(), ItTarget->Key);
			SightQuery.Importance = CalcQueryImportance(NewListener, ItTarget->Value.GetLocationSimple(), NewListener.SightRadiusSq);

			SightQueryQueue.Add(SightQuery);
			bNewQueriesAdded = true;
		}
	}

	// sort Sight Queries
	if (bNewQueriesAdded)
	{
		SortQueries();
		RequestImmediateUpdate();
	}
}

void UAISense_Sight::OnListenerUpdateImpl(const FPerceptionListener& UpdatedListener)
{
	// first, naive implementation:
	// 1. remove all queries by this listener
	// 2. proceed as if it was a new listener
	// @todo add stats here to check how bad it is

	// remove all queries
	RemoveAllQueriesByListener(UpdatedListener, DontSort);

	// see if this listener is a Target as well
	const FAISightTarget::FTargetId AsTargetId = UpdatedListener.GetBodyActorName();
	FAISightTarget* AsTarget = ObservedTargets.Find(AsTargetId);
	if (AsTarget != NULL)
	{
		RemoveAllQueriesToTarget(AsTargetId, DontSort);
		if (AsTarget->Target.IsValid())
		{
			RegisterTarget(*(AsTarget->Target.Get()), DontSort);
		}
	}

	// act as if it was a new listener
	OnNewListenerImpl(UpdatedListener);
}

void UAISense_Sight::OnListenerRemovedImpl(const FPerceptionListener& UpdatedListener)
{
	RemoveAllQueriesByListener(UpdatedListener, DontSort);

	if (UpdatedListener.Listener.IsValid())
	{
		// see if this listener is a Target as well
		const FAISightTarget::FTargetId AsTargetId = UpdatedListener.GetBodyActorName();
		FAISightTarget* AsTarget = ObservedTargets.Find(AsTargetId);
		if (AsTarget != NULL)
		{
			RemoveAllQueriesToTarget(AsTargetId, Sort);			
		}
	}
	else
	{
		//@todo quite possible there are left over sight queries with this listener as target
	}
}

void UAISense_Sight::RemoveAllQueriesByListener(const FPerceptionListener& Listener, FQueriesOperationPostProcess PostProcess)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight);

	if (SightQueryQueue.Num() == 0)
	{
		return;
	}

	const uint32 ListenerId = Listener.GetListenerId();
	bool bQueriesRemoved = false;
	
	const FAISightQuery* SightQuery = &SightQueryQueue[SightQueryQueue.Num() - 1];
	for (int32 QueryIndex = SightQueryQueue.Num() - 1; QueryIndex >= 0 ; --QueryIndex, --SightQuery)
	{
		if (SightQuery->ObserverId == ListenerId)
		{
			SightQueryQueue.RemoveAt(QueryIndex, 1, /*bAllowShrinking=*/false);
			bQueriesRemoved = true;
		}
	}

	if (PostProcess == Sort && bQueriesRemoved)
	{
		SortQueries();
	}
}

void UAISense_Sight::RemoveAllQueriesToTarget(const FName& TargetId, FQueriesOperationPostProcess PostProcess)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_Sense_Sight);

	if (SightQueryQueue.Num() == 0)
	{
		return;
	}

	bool bQueriesRemoved = false;

	const FAISightQuery* SightQuery = &SightQueryQueue[SightQueryQueue.Num() - 1];
	for (int32 QueryIndex = SightQueryQueue.Num() - 1; QueryIndex >= 0 ; --QueryIndex, --SightQuery)
	{
		if (SightQuery->TargetId == TargetId)
		{
			SightQueryQueue.RemoveAt(QueryIndex, 1, /*bAllowShrinking=*/false);
			bQueriesRemoved = true;
		}
	}

	if (PostProcess == Sort && bQueriesRemoved)
	{
		SortQueries();
	}
}