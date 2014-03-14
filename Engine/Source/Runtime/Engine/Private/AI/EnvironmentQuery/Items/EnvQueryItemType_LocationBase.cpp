// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryItemType_LocationBase::UEnvQueryItemType_LocationBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UEnvQueryItemType_LocationBase::AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const
{
	KeySelector.AddVectorFilter(FilterOwner);
}

bool UEnvQueryItemType_LocationBase::StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const
{
	bool bStored = false;
	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector MyLocation = GetLocation(RawData);
		Blackboard->SetValueAsVector(KeySelector.SelectedKeyID, MyLocation);

		bStored = true;
	}

	return bStored;
}
