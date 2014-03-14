// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryTest::UEnvQueryTest(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	Cost = EEnvTestCost::Low;
	Condition = EEnvTestCondition::NoCondition;
	WeightModifier = EEnvTestWeight::None;
	Weight.Value = 1.0f;
	bWorkOnFloatValues = true;
	BoolFilter.Value = true;
}

void UEnvQueryTest::NormalizeItemScores(struct FEnvQueryInstance& QueryInstance)
{
	if (WeightModifier == EEnvTestWeight::Flat)
	{
		return;
	}

	float WeightValue = 0.0f;
	if (!QueryInstance.GetParamValue(Weight, WeightValue, TEXT("Weight")))
	{
		return;
	}
	
	float MinScore = 0.f;
	float MaxScore = -BIG_NUMBER;

	FEnvQueryItemDetails* DetailInfo = QueryInstance.ItemDetails.GetTypedData();
	for (int32 iItem = 0; iItem < QueryInstance.Items.Num(); iItem++, DetailInfo++)
	{
		if (QueryInstance.Items[iItem].IsValid())
		{
			MinScore = FMath::Min(MinScore, DetailInfo->TestResults[QueryInstance.CurrentTest]);
			MaxScore = FMath::Max(MaxScore, DetailInfo->TestResults[QueryInstance.CurrentTest]);
		}
	}

	DetailInfo = QueryInstance.ItemDetails.GetTypedData();
	if (bNormalizeFromZero && MinScore > 0.0f)
	{
		MinScore = 0.0f;
	}

	// modifier TestWeight_Absolute is applied before normalization
	if (WeightModifier == EEnvTestWeight::Absolute)
	{
		MaxScore = FMath::Max(FMath::Abs(MinScore), FMath::Abs(MaxScore));
		MinScore = 0;

		if (MinScore != MaxScore)
		{
			for (int32 ItemIndex = 0; ItemIndex < QueryInstance.ItemDetails.Num(); ItemIndex++, DetailInfo++)
			{
				if (QueryInstance.Items[ItemIndex].IsValid() == false)
				{
					continue;
				}

				const float ModifiedScore = FMath::Abs(DetailInfo->TestResults[QueryInstance.CurrentTest]);

				const float NormalizedScore = ModifiedScore / MaxScore;
				const float WeightedScore = ((WeightValue > 0) ? NormalizedScore : 1.0f - NormalizedScore) * FMath::Abs(WeightValue);

#if EQS_STORE_PARTIAL_WEIGHTS
				DetailInfo->TestWeightedScores[QueryInstance.CurrentTest] = WeightedScore;
#endif
				QueryInstance.Items[ItemIndex].Score += WeightedScore;
			}
		}
	}
	else if (MinScore != MaxScore)
	{
		for (int32 ItemIndex = 0; ItemIndex < QueryInstance.ItemDetails.Num(); ItemIndex++, DetailInfo++)
		{
			if (QueryInstance.Items[ItemIndex].IsValid() == false)
			{
				continue;
			}

			const float NormalizedScore = (DetailInfo->TestResults[QueryInstance.CurrentTest] - MinScore) / (MaxScore - MinScore);

			const float ModifiedScore =
				(WeightModifier == EEnvTestWeight::Square) ? NormalizedScore * NormalizedScore :
				(WeightModifier == EEnvTestWeight::Inverse) ? 1.0f / FMath::Max(0.01f, NormalizedScore) :
				(WeightModifier == EEnvTestWeight::Flat) ? 1.0f :
				NormalizedScore;

			const float WeightedScore = ((WeightValue > 0) ? ModifiedScore : 1.0f - ModifiedScore) * FMath::Abs(WeightValue);

#if EQS_STORE_PARTIAL_WEIGHTS
			DetailInfo->TestWeightedScores[QueryInstance.CurrentTest] = WeightedScore;
#endif
			QueryInstance.Items[ItemIndex].Score += WeightedScore;
		}
	}
}

bool UEnvQueryTest::IsContextPerItem(TSubclassOf<class UEnvQueryContext> CheckContext) const
{
	return CheckContext == UEnvQueryContext_Item::StaticClass();
}

FVector UEnvQueryTest::GetItemLocation(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeLocationCDO ?
		QueryInstance.ItemTypeLocationCDO->GetLocation(QueryInstance.RawData.GetTypedData() + QueryInstance.Items[ItemIndex].DataOffset) :
		FVector::ZeroVector;
}

FRotator UEnvQueryTest::GetItemRotation(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeLocationCDO ?
		QueryInstance.ItemTypeLocationCDO->GetRotation(QueryInstance.RawData.GetTypedData() + QueryInstance.Items[ItemIndex].DataOffset) :
		FRotator::ZeroRotator;
}

AActor* UEnvQueryTest::GetItemActor(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeActorCDO ?
		QueryInstance.ItemTypeActorCDO->GetActor(QueryInstance.RawData.GetTypedData() + QueryInstance.Items[ItemIndex].DataOffset) :
		NULL;
}

FString UEnvQueryTest::GetDescriptionTitle() const
{
	return UEnvQueryTypes::GetShortTypeName(this);
}

FString UEnvQueryTest::GetDescriptionDetails() const
{
	return FString();
}

#if WITH_EDITOR
void UEnvQueryTest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UEnvQueryManager::NotifyAssetUpdate(NULL);
}
#endif //WITH_EDITOR

FString UEnvQueryTest::DescribeFloatTestParams() const
{
	FString ParamDesc;

	if (Condition != EEnvTestCondition::NoCondition)
	{
		FString InvalidConditionDesc("invalid condition");
		ParamDesc =
			(Condition == EEnvTestCondition::AtLeast) ? FString::Printf(TEXT("at least %s"), *UEnvQueryTypes::DescribeFloatParam(FloatFilter)) :
			(Condition == EEnvTestCondition::UpTo) ? FString::Printf(TEXT("up to %s"), *UEnvQueryTypes::DescribeFloatParam(FloatFilter)) :
			InvalidConditionDesc;

		ParamDesc += TEXT(", ");
	}

	if (WeightModifier == EEnvTestWeight::Flat)
	{
		ParamDesc += TEXT("don't score");
	}
	else if (Weight.IsNamedParam())
	{
		ParamDesc += FString::Printf(TEXT("weight: %s"), *Weight.ParamName.ToString());
	}
	else
	{
		ParamDesc += FString::Printf(TEXT("prefer %s [x%.2f]"), (Weight.Value > 0) ? TEXT("greater") : TEXT("lesser"), Weight.Value);
	}

	return ParamDesc;
}

FString UEnvQueryTest::DescribeBoolTestParams(const FString& ConditionDesc) const
{
	FString ParamDesc;

	if (Condition != EEnvTestCondition::NoCondition)
	{
		FString InvalidConditionDesc("invalid condition");
		ParamDesc =
			(Condition == EEnvTestCondition::Match) ?
				(BoolFilter.IsNamedParam() ?
					FString::Printf(TEXT("require %s: %s"), *ConditionDesc, *UEnvQueryTypes::DescribeBoolParam(BoolFilter)) :
					FString::Printf(TEXT("require %s%s"), BoolFilter.Value ? TEXT("") : TEXT("not "), *ConditionDesc)
				) :
			InvalidConditionDesc;

		ParamDesc += TEXT(", ");
	}

	if (WeightModifier == EEnvTestWeight::Flat)
	{
		ParamDesc += TEXT("don't score");
	}
	else if (Weight.IsNamedParam())
	{
		ParamDesc += FString::Printf(TEXT("weight: %s"), *Weight.ParamName.ToString());
	}
	else
	{
		ParamDesc += FString::Printf(TEXT("prefer %s%s [x%.2f]"), (Weight.Value > 0) ? TEXT("") : TEXT("not "), *ConditionDesc, Weight.Value);
	}

	return ParamDesc;
}
