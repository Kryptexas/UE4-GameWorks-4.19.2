// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

namespace
{
	FORCEINLINE float CalcDistance3D(const FVector& PosA, const FVector& PosB)
	{
		return (PosB - PosA).Size();
	}

	FORCEINLINE float CalcDistance2D(const FVector& PosA, const FVector& PosB)
	{
		return (PosB - PosA).Size2D();
	}

	FORCEINLINE float CalcDistanceZ(const FVector& PosA, const FVector& PosB)
	{
		return PosB.Z - PosA.Z;
	}
}

UEnvQueryTest_Distance::UEnvQueryTest_Distance(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ExecuteDelegate.BindUObject(this, &UEnvQueryTest_Distance::RunTest);

	DistanceTo = UEnvQueryContext_Querier::StaticClass();
	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_LocationBase::StaticClass();
}

void UEnvQueryTest_Distance::RunTest(struct FEnvQueryInstance& QueryInstance)
{
	float ThresholdValue = 0.0f;
	if (!QueryInstance.GetParamValue(FloatFilter, ThresholdValue, TEXT("FloatFilter")))
	{
		return;
	}

	// don't support context Item here, it doesn't make any sense
	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(DistanceTo, ContextLocations))
	{
		return;
	}

	switch (TestMode)
	{
	case EEnvTestDistance::Distance3D:	
		for (FEnvQueryInstance::ItemIterator It(QueryInstance); It; ++It)
		{
			const FVector ItemLocation = GetItemLocation(QueryInstance, *It);
			for (int32 iContext = 0; iContext < ContextLocations.Num(); iContext++)
			{
				const float Distance = CalcDistance3D(ItemLocation, ContextLocations[iContext]);
				It.SetScore(Condition, Distance, ThresholdValue);
			}
		}
		break;
	case EEnvTestDistance::Distance2D:	
		for (FEnvQueryInstance::ItemIterator It(QueryInstance); It; ++It)
		{
			const FVector ItemLocation = GetItemLocation(QueryInstance, *It);
			for (int32 iContext = 0; iContext < ContextLocations.Num(); iContext++)
			{
				const float Distance = CalcDistance2D(ItemLocation, ContextLocations[iContext]);
				It.SetScore(Condition, Distance, ThresholdValue);
			}
		}
		break;
	case EEnvTestDistance::DistanceZ:	
		for (FEnvQueryInstance::ItemIterator It(QueryInstance); It; ++It)
		{
			const FVector ItemLocation = GetItemLocation(QueryInstance, *It);
			for (int32 iContext = 0; iContext < ContextLocations.Num(); iContext++)
			{
				const float Distance = CalcDistanceZ(ItemLocation, ContextLocations[iContext]);
				It.SetScore(Condition, Distance, ThresholdValue);
			}
		}
		break;
	default:
		return;
	}
}

FString UEnvQueryTest_Distance::GetDescriptionTitle() const
{
	FString ModeDesc;
	switch (TestMode)
	{
	case EEnvTestDistance::Distance3D:	ModeDesc = TEXT(""); break;
	case EEnvTestDistance::Distance2D:	ModeDesc = TEXT(" 2D"); break;
	case EEnvTestDistance::DistanceZ:	ModeDesc = TEXT(" Z"); break;
	default: break;
	}

	return FString::Printf(TEXT("%s%s: to %s"), 
		*Super::GetDescriptionTitle(), *ModeDesc,
		*UEnvQueryTypes::DescribeContext(DistanceTo));
}

FString UEnvQueryTest_Distance::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}
