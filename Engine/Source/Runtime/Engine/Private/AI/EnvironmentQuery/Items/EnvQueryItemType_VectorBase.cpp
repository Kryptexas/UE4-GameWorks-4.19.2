// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryItemType_VectorBase::UEnvQueryItemType_VectorBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

void UEnvQueryItemType_VectorBase::AddBlackboardFilters(struct FBlackboardKeySelector& KeySelector, UObject* FilterOwner) const
{
	KeySelector.AddVectorFilter(FilterOwner);
}

bool UEnvQueryItemType_VectorBase::StoreInBlackboard(struct FBlackboardKeySelector& KeySelector, class UBlackboardComponent* Blackboard, const uint8* RawData) const
{
	bool bStored = false;
	if (KeySelector.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		const FVector MyLocation = GetLocation(RawData);
		Blackboard->SetValueAsVector(KeySelector.GetSelectedKeyID(), MyLocation);

		bStored = true;
	}

	return bStored;
}

FVector UEnvQueryItemType_VectorBase::GetLocation(const uint8* RawData) const
{
	return FVector::ZeroVector;
}

FRotator UEnvQueryItemType_VectorBase::GetRotation(const uint8* RawData) const
{
	return FRotator::ZeroRotator;
}
