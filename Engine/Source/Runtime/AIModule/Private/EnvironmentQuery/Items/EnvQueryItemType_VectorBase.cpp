// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"

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

FString UEnvQueryItemType_VectorBase::GetDescription(const uint8* RawData) const
{
	FVector PointLocation = GetLocation(RawData);
	return FString::Printf(TEXT("(X=%.0f,Y=%.0f,Z=%.0f)"), PointLocation.X, PointLocation.Y, PointLocation.Z);
}

FVector UEnvQueryItemType_VectorBase::GetLocation(const uint8* RawData) const
{
	return FVector::ZeroVector;
}

FRotator UEnvQueryItemType_VectorBase::GetRotation(const uint8* RawData) const
{
	return FRotator::ZeroRotator;
}
