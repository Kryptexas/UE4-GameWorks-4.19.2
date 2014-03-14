// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryItemType_ActorBase::UEnvQueryItemType_ActorBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UEnvQueryItemType_ActorBase::AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const
{
	Super::AddBlackboardFilters(KeySelector, FilterOwner);
	KeySelector.AddObjectFilter(FilterOwner, AActor::StaticClass());
}

bool UEnvQueryItemType_ActorBase::StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const
{
	bool bStored = StoreInBlackboard(KeySelector, Blackboard, RawData);
	if (!bStored && KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* MyObject = GetActor(RawData);
		Blackboard->SetValueAsObject(KeySelector.SelectedKeyID, MyObject);

		bStored = true;
	}

	return bStored;
}
