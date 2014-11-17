// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Item.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Dot.h"

UEnvQueryTest_Dot::UEnvQueryTest_Dot(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ExecuteDelegate.BindUObject(this, &UEnvQueryTest_Dot::RunTest);

	Cost = EEnvTestCost::Low;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	LineA.DirMode = EEnvDirection::Rotation;
	LineA.Rotation = UEnvQueryContext_Querier::StaticClass();
	LineB.DirMode = EEnvDirection::TwoPoints;
	LineB.LineFrom = UEnvQueryContext_Querier::StaticClass();
	LineB.LineTo = UEnvQueryContext_Item::StaticClass();
}

void UEnvQueryTest_Dot::RunTest(struct FEnvQueryInstance& QueryInstance)
{
// 	float ThresholdValue = 0.0f;
// 	if (!QueryInstance.GetParamValue(FloatFilter, ThresholdValue, TEXT("FloatFilter")))
// 	{
// 		return;
// 	}

	float MinThresholdValue = 0.0f;
	if (!QueryInstance.GetParamValue(FloatFilterMin, MinThresholdValue, TEXT("FloatFilterMin")))
	{
		return;
	}

	float MaxThresholdValue = 0.0f;
	if (!QueryInstance.GetParamValue(FloatFilterMax, MaxThresholdValue, TEXT("FloatFilterMax")))
	{
		return;
	}

	// gather all possible directions: for contexts different than Item
	TArray<FVector> LineADirs;
	const bool bUpdateLineAPerItem = RequiresPerItemUpdates(LineA.LineFrom, LineA.LineTo, LineA.Rotation, LineA.DirMode == EEnvDirection::Rotation);
	if (!bUpdateLineAPerItem)
	{
		GatherLineDirections(LineADirs, QueryInstance, LineA.LineFrom, LineA.LineTo, LineA.Rotation, LineA.DirMode == EEnvDirection::Rotation);
		if (LineADirs.Num() == 0)
		{
			return;
		}
	}

	TArray<FVector> LineBDirs;
	const bool bUpdateLineBPerItem = RequiresPerItemUpdates(LineB.LineFrom, LineB.LineTo, LineB.Rotation, LineB.DirMode == EEnvDirection::Rotation);
	if (!bUpdateLineBPerItem)
	{
		GatherLineDirections(LineBDirs, QueryInstance, LineB.LineFrom, LineB.LineTo, LineB.Rotation, LineB.DirMode == EEnvDirection::Rotation);
		if (LineBDirs.Num() == 0)
		{
			return;
		}
	}

	// loop through all items
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		// update lines for contexts using current item
		if (bUpdateLineAPerItem || bUpdateLineBPerItem)
		{
			const FVector ItemLocation = (LineA.DirMode == EEnvDirection::Rotation && LineB.DirMode == EEnvDirection::Rotation) ? FVector::ZeroVector : GetItemLocation(QueryInstance, *It);
			const FRotator ItemRotation = (LineA.DirMode == EEnvDirection::Rotation || LineB.DirMode == EEnvDirection::Rotation) ? GetItemRotation(QueryInstance, *It) : FRotator::ZeroRotator;

			if (bUpdateLineAPerItem)
			{
				LineADirs.Reset();
				GatherLineDirections(LineADirs, QueryInstance, LineA.LineFrom, LineA.LineTo, LineA.Rotation, LineA.DirMode == EEnvDirection::Rotation, ItemLocation, ItemRotation);
			}

			if (bUpdateLineBPerItem)
			{
				LineBDirs.Reset();
				GatherLineDirections(LineBDirs, QueryInstance, LineB.LineFrom, LineB.LineTo, LineB.Rotation, LineB.DirMode == EEnvDirection::Rotation, ItemLocation, ItemRotation);
			}
		}

		// perform test for each line pair
		for (int32 LineAIndex = 0; LineAIndex < LineADirs.Num(); LineAIndex++)
		{
			for (int32 LineBIndex = 0; LineBIndex < LineBDirs.Num(); LineBIndex++)
			{
				const float DotValue = FVector::DotProduct(LineADirs[LineAIndex], LineBDirs[LineBIndex]);
				It.SetScore(TestPurpose, FilterType, DotValue, MinThresholdValue, MaxThresholdValue);
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
	
	for (int32 FromIndex = 0; FromIndex < ContextLocationFrom.Num(); FromIndex++)
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
		
		for (int32 ToIndex = 0; ToIndex < ContextLocationTo.Num(); ToIndex++)
		{
			const FVector Dir = (ContextLocationTo[ToIndex] - ContextLocationFrom[FromIndex]).SafeNormal();
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

	for (int32 RotationIndex = 0; RotationIndex < ContextRotations.Num(); RotationIndex++)
	{
		const FVector Dir = ContextRotations[RotationIndex].Vector();
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
	return FString::Printf(TEXT("%s: %s and %s"), *Super::GetDescriptionTitle(), *LineA.ToText().ToString(), *LineB.ToText().ToString());
}

FText UEnvQueryTest_Dot::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}
