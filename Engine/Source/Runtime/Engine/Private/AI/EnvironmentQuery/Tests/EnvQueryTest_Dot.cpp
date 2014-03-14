// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryTest_Dot::UEnvQueryTest_Dot(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ExecuteDelegate.BindUObject(this, &UEnvQueryTest_Dot::RunTest);

	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_LocationBase::StaticClass();
	LineA = EEnvTestDot::Direction;
	LineADirection = UEnvQueryContext_Querier::StaticClass();
	LineB = EEnvTestDot::Segment;
	LineBFrom = UEnvQueryContext_Querier::StaticClass();
	LineBTo = UEnvQueryContext_Item::StaticClass();
}

void UEnvQueryTest_Dot::RunTest(struct FEnvQueryInstance& QueryInstance)
{
	float ThresholdValue = 0.0f;
	if (!QueryInstance.GetParamValue(FloatFilter, ThresholdValue, TEXT("FloatFilter")))
	{
		return;
	}

	// gather all possible directions: for contexts different than Item
	TArray<FVector> LineADirs;
	const bool bUpdateLineAPerItem = RequiresPerItemUpdates(LineAFrom, LineATo, LineADirection, LineA == EEnvTestDot::Direction);
	if (!bUpdateLineAPerItem)
	{
		GatherLineDirections(LineADirs, QueryInstance, LineAFrom, LineATo, LineADirection, LineA == EEnvTestDot::Direction);
		if (LineADirs.Num() == 0)
		{
			return;
		}
	}

	TArray<FVector> LineBDirs;
	const bool bUpdateLineBPerItem = RequiresPerItemUpdates(LineBFrom, LineBTo, LineBDirection, LineB == EEnvTestDot::Direction);
	if (!bUpdateLineBPerItem)
	{
		GatherLineDirections(LineBDirs, QueryInstance, LineBFrom, LineBTo, LineBDirection, LineB == EEnvTestDot::Direction);
		if (LineBDirs.Num() == 0)
		{
			return;
		}
	}

	// loop through all items
	for (FEnvQueryInstance::ItemIterator It(QueryInstance); It; ++It)
	{
		// update lines for contexts using current item
		if (bUpdateLineAPerItem || bUpdateLineBPerItem)
		{
			const FVector ItemLocation = (LineA == EEnvTestDot::Direction && LineB == EEnvTestDot::Direction) ? FVector::ZeroVector : GetItemLocation(QueryInstance, *It);
			const FRotator ItemRotation = (LineA == EEnvTestDot::Direction || LineB == EEnvTestDot::Direction) ? GetItemRotation(QueryInstance, *It) : FRotator::ZeroRotator;

			if (bUpdateLineAPerItem)
			{
				LineADirs.Reset();
				GatherLineDirections(LineADirs, QueryInstance, LineAFrom, LineATo, LineADirection, LineA == EEnvTestDot::Direction, ItemLocation, ItemRotation);
			}

			if (bUpdateLineBPerItem)
			{
				LineBDirs.Reset();
				GatherLineDirections(LineBDirs, QueryInstance, LineBFrom, LineBTo, LineBDirection, LineB == EEnvTestDot::Direction, ItemLocation, ItemRotation);
			}
		}

		// perform test for each line pair
		for (int32 iLineA = 0; iLineA < LineADirs.Num(); iLineA++)
		{
			for (int32 iLineB = 0; iLineB < LineBDirs.Num(); iLineB++)
			{
				const float DotValue = FVector::DotProduct(LineADirs[iLineA], LineBDirs[iLineB]);
				It.SetScore(Condition, DotValue, ThresholdValue);
			}
		}
	}
}

void UEnvQueryTest_Dot::GatherLineDirections(TArray<FVector>& Directions, struct FEnvQueryInstance& QueryInstance, const FVector& ItemLocation,
	TSubclassOf<class UEnvQueryContext> LineFrom, TSubclassOf<class UEnvQueryContext> LineTo)
{
	TArray<FVector> ContextLocationFrom;
	if (IsContextPerItem(LineFrom))
	{
		ContextLocationFrom.Add(ItemLocation);
	}
	else
	{
		QueryInstance.PrepareContext(LineFrom, ContextLocationFrom);
	}
	
	for (int32 i = 0; i < ContextLocationFrom.Num(); i++)
	{
		TArray<FVector> ContextLocationTo;
		if (IsContextPerItem(LineTo))
		{
			ContextLocationTo.Add(ItemLocation);
		}
		else
		{
			QueryInstance.PrepareContext(LineTo, ContextLocationTo);
		}
		
		for (int32 i2 = 0; i2 < ContextLocationTo.Num(); i2++)
		{
			const FVector Dir = (ContextLocationTo[i2] - ContextLocationFrom[i]).SafeNormal();
			Directions.Add(Dir);
		}
	}
}

void UEnvQueryTest_Dot::GatherLineDirections(TArray<FVector>& Directions, struct FEnvQueryInstance& QueryInstance, const FRotator& ItemRotation,
	TSubclassOf<class UEnvQueryContext> LineDirection)
{
	TArray<FRotator> ContextRotations;
	if (IsContextPerItem(LineDirection))
	{
		ContextRotations.Add(ItemRotation);
	}
	else
	{
		QueryInstance.PrepareContext(LineDirection, ContextRotations);
	}

	for (int32 i = 0; i < ContextRotations.Num(); i++)
	{
		const FVector Dir = ContextRotations[i].Vector();
		Directions.Add(Dir);
	}
}

void UEnvQueryTest_Dot::GatherLineDirections(TArray<FVector>& Directions, struct FEnvQueryInstance& QueryInstance,
	TSubclassOf<class UEnvQueryContext> LineFrom, TSubclassOf<class UEnvQueryContext> LineTo, TSubclassOf<class UEnvQueryContext> LineDirection, bool bUseDirectionContext,
	const FVector& ItemLocation, const FRotator& ItemRotation)
{
	if (bUseDirectionContext)
	{
		GatherLineDirections(Directions, QueryInstance, ItemRotation, LineDirection);
	}
	else
	{
		GatherLineDirections(Directions, QueryInstance, ItemLocation, LineFrom, LineTo);
	}
}

bool UEnvQueryTest_Dot::RequiresPerItemUpdates(TSubclassOf<class UEnvQueryContext> LineFrom, TSubclassOf<class UEnvQueryContext> LineTo, TSubclassOf<class UEnvQueryContext> LineDirection, bool bUseDirectionContext)
{
	bool bRequirePerItemUpdate = false;
	if (bUseDirectionContext)
	{
		bRequirePerItemUpdate = IsContextPerItem(LineDirection);
	}
	else
	{
		bRequirePerItemUpdate = IsContextPerItem(LineFrom) || IsContextPerItem(LineTo);
	}

	return bRequirePerItemUpdate;
}

FString UEnvQueryTest_Dot::GetDescriptionTitle() const
{
	FString LineADesc = LineA == EEnvTestDot::Segment ?
		FString::Printf(TEXT("[%s - %s]"), *UEnvQueryTypes::DescribeContext(LineAFrom), *UEnvQueryTypes::DescribeContext(LineATo)) :
		FString::Printf(TEXT("[%s rotation]"), *UEnvQueryTypes::DescribeContext(LineADirection));

	FString LineBDesc = LineB == EEnvTestDot::Segment ?
		FString::Printf(TEXT("[%s - %s]"), *UEnvQueryTypes::DescribeContext(LineBFrom), *UEnvQueryTypes::DescribeContext(LineBTo)) :
		FString::Printf(TEXT("[%s rotation]"), *UEnvQueryTypes::DescribeContext(LineBDirection));

	return FString::Printf(TEXT("%s: %s and %s"), *Super::GetDescriptionTitle(), *LineADesc, *LineBDesc);
}

FString UEnvQueryTest_Dot::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}
