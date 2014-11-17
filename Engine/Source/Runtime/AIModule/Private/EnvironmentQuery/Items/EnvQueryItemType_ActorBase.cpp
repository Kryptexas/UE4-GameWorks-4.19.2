// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"

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
	bool bStored = Super::StoreInBlackboard(KeySelector, Blackboard, RawData);
	if (!bStored && KeySelector.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UObject* MyObject = GetActor(RawData);
		Blackboard->SetValueAsObject(KeySelector.GetSelectedKeyID(), MyObject);

		bStored = true;
	}

	return bStored;
}

FString UEnvQueryItemType_ActorBase::GetDescription(const uint8* RawData) const
{
	const AActor* Actor = GetActor(RawData);
	return GetNameSafe(Actor);
}

AActor* UEnvQueryItemType_ActorBase::GetActor(const uint8* RawData) const
{
	return NULL;
}
