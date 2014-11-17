// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryTest::UEnvQueryTest(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	Cost = EEnvTestCost::Low;
	Condition = EEnvTestCondition::NoCondition;
	WeightModifier = EEnvTestWeight::None;
	Weight.Value = 1.0f;
	bWorkOnFloatValues = true;
	bDiscardFailedItems = true;
	BoolFilter.Value = true;
}

void UEnvQueryTest::NormalizeItemScores(struct FEnvQueryInstance& QueryInstance)
{
	if (WeightModifier == EEnvTestWeight::Skip)
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
		if (!QueryInstance.Items[iItem].IsValid())
		{
			continue;
		}

		const float TestValue = DetailInfo->TestResults[QueryInstance.CurrentTest];
		if (TestValue != UEnvQueryTypes::SkippedItemValue)
		{
			MinScore = FMath::Min(MinScore, TestValue);
			MaxScore = FMath::Max(MaxScore, TestValue);
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

				float WeightedScore = 0.0f;

				float& TestValue = DetailInfo->TestResults[QueryInstance.CurrentTest];
				if (TestValue != UEnvQueryTypes::SkippedItemValue)
				{
					const float ModifiedScore = FMath::Abs(DetailInfo->TestResults[QueryInstance.CurrentTest]);

					const float NormalizedScore = ModifiedScore / MaxScore;
					WeightedScore = ((WeightValue > 0) ? NormalizedScore : 1.0f - NormalizedScore) * FMath::Abs(WeightValue);
				}
				else
				{
					TestValue = 0.0f;
					WeightedScore = 0.0f;
				}

#if USE_EQS_DEBUGGER
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

			float WeightedScore = 0.0f;

			float& TestValue = DetailInfo->TestResults[QueryInstance.CurrentTest];
			if (TestValue != UEnvQueryTypes::SkippedItemValue)
			{
				const float NormalizedScore = (TestValue - MinScore) / (MaxScore - MinScore);
				const float ModifiedScore =
					(WeightModifier == EEnvTestWeight::Square) ? NormalizedScore * NormalizedScore :
					(WeightModifier == EEnvTestWeight::Inverse) ? 1.0f / FMath::Max(0.01f, NormalizedScore) :
					(WeightModifier == EEnvTestWeight::Constant) ? 1.0f :
					NormalizedScore;

				WeightedScore = ((WeightValue > 0) ? ModifiedScore : 1.0f - ModifiedScore) * FMath::Abs(WeightValue);
			}
			else
			{
				TestValue = 0.0f;
				WeightedScore = 0.0f;
			}

#if USE_EQS_DEBUGGER
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
	return QueryInstance.ItemTypeVectorCDO ?
		QueryInstance.ItemTypeVectorCDO->GetLocation(QueryInstance.RawData.GetTypedData() + QueryInstance.Items[ItemIndex].DataOffset) :
		FVector::ZeroVector;
}

FRotator UEnvQueryTest::GetItemRotation(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeVectorCDO ?
		QueryInstance.ItemTypeVectorCDO->GetRotation(QueryInstance.RawData.GetTypedData() + QueryInstance.Items[ItemIndex].DataOffset) :
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
	return UEnvQueryTypes::GetShortTypeName(this).ToString();
}

FText UEnvQueryTest::GetDescriptionDetails() const
{
	return FText::GetEmpty();
}

#if WITH_EDITOR
void UEnvQueryTest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UEnvQueryManager::NotifyAssetUpdate(NULL);
}
#endif //WITH_EDITOR

FText UEnvQueryTest::DescribeFloatTestParams() const
{
	FText ParamDesc;

	if (Condition != EEnvTestCondition::NoCondition)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Filter"), FText::FromString(UEnvQueryTypes::DescribeFloatParam(FloatFilter)));

		FText InvalidConditionDesc = LOCTEXT("InvalidCondition", "invalid condition");
		ParamDesc =
			(Condition == EEnvTestCondition::AtLeast) ? FText::Format(LOCTEXT("AtLeastFiltered", "at least {Filter}"), Args) :
			(Condition == EEnvTestCondition::UpTo) ? FText::Format(LOCTEXT("UpToFiltered", "up to {Filter}"), Args) :
			InvalidConditionDesc;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("ParmDesc"), ParamDesc);

	if (WeightModifier == EEnvTestWeight::Skip)
	{
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithDontScore", "{ParmDesc}, don't score"), Args);
	}
	else if (WeightModifier == EEnvTestWeight::Constant)
	{
		FText WeightDesc = Weight.IsNamedParam() ? FText::FromName(Weight.ParamName) : FText::Format( LOCTEXT("ConstantWeightDescPattern", "x{0}"), FText::AsNumber(FMath::Abs(Weight.Value)) );
		Args.Add(TEXT("WeightDesc"), WeightDesc);
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithConstantWeight", "{ParmDesc}, constant weight [{WeightDesc}]"), Args);
	}
	else if (Weight.IsNamedParam())
	{
		Args.Add(TEXT("WeightName"), FText::FromName(Weight.ParamName));
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithWeight", "{ParmDesc}, weight: {WeightName}"), Args);
	}
	else
	{
		FNumberFormattingOptions NumberFormattingOptions;
		NumberFormattingOptions.MaximumFractionalDigits = 2;

		Args.Add(TEXT("WeightCondition"), (Weight.Value > 0) ? LOCTEXT("Greater", "greater") : LOCTEXT("Lesser", "lesser"));
		Args.Add(TEXT("WeightValue"), FText::AsNumber(FMath::Abs(Weight.Value), &NumberFormattingOptions));
		ParamDesc = FText::Format(LOCTEXT("DescriptionPreferWeightValue", "{ParmDesc}, prefer {WeightCondition} [x{WeightValue}]"), Args);
	}

	return ParamDesc;
}

FText UEnvQueryTest::DescribeBoolTestParams(const FString& ConditionDesc) const
{
	FText ParamDesc;

	if (Condition != EEnvTestCondition::NoCondition)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ConditionDesc"), FText::FromString(ConditionDesc));

		if(BoolFilter.IsNamedParam())
		{
			Args.Add(TEXT("Filter"), FText::FromString(UEnvQueryTypes::DescribeBoolParam(BoolFilter)));
		}
		else
		{
			Args.Add(TEXT("Filter"), BoolFilter.Value ? FText::GetEmpty() : LOCTEXT("NotWithSpace", "not "));
		}
		FText InvalidConditionDesc = LOCTEXT("InvalidCondition", "invalid condition");
		ParamDesc =
			(Condition == EEnvTestCondition::Match) ?
				(BoolFilter.IsNamedParam() ?
					FText::Format(LOCTEXT("RequiresCondition", "require {ConditionDesc}: {Filter}"), Args) :
					FText::Format(LOCTEXT("RequiresFilter", "require {Filter}{ConditionDesc}%s"), Args)
				) :
			InvalidConditionDesc;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("ParmDesc"), ParamDesc);

	if (WeightModifier == EEnvTestWeight::Skip)
	{
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithDontScore", "{ParmDesc}, don't score"), Args);
	}
	else if (WeightModifier == EEnvTestWeight::Constant)
	{
		FText WeightDesc = Weight.IsNamedParam() ? FText::FromName(Weight.ParamName) : FText::Format(LOCTEXT("ConstantWeightDescPattern", "x{0}"), FText::AsNumber(FMath::Abs(Weight.Value)));
		Args.Add(TEXT("WeightDesc"), WeightDesc);
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithConstantWeight", "{ParmDesc}, constant weight [{WeightDesc}]"), Args);
	}
	else if (Weight.IsNamedParam())
	{
		Args.Add(TEXT("WeightName"), FText::FromName(Weight.ParamName));
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithWeight", "{ParmDesc}, weight: {WeightName}"), Args);
	}
	else
	{
		FNumberFormattingOptions NumberFormattingOptions;
		NumberFormattingOptions.MaximumFractionalDigits = 2;

		Args.Add(TEXT("ConditionDesc"), FText::FromString(ConditionDesc));
		Args.Add(TEXT("WeightCondition"), (Weight.Value > 0) ? FText::GetEmpty() : LOCTEXT("NotWithSpace", "not "));
		Args.Add(TEXT("WeightValue"), FText::AsNumber(FMath::Abs(Weight.Value), &NumberFormattingOptions));
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithPreference", "{ParmDesc}, prefer {WeightCondition}{ConditionDesc} [x{WeightValue}]"), Args);
	}

	return ParamDesc;
}

#undef LOCTEXT_NAMESPACE
