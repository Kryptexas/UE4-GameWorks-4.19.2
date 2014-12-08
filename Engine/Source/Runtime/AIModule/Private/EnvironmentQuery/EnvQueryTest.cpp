// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Item.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_ActorBase.h"
#include "EnvironmentQuery/EnvQueryTest.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryTest::UEnvQueryTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	TestPurpose = EEnvTestPurpose::FilterAndScore;
	FilterType = EEnvTestFilterType::Range;
	BoolFilter.Value = true;
	bFormatUpdated = false;
	Condition = EEnvTestCondition::NoCondition;	// DEPRECATED
	Weight.Value = 1.0f;						// DEPRECATED
	WeightModifier = EEnvTestWeight::None;		// DEPRECATED
	Cost = EEnvTestCost::Low;
	ClampMinType = EEnvQueryTestClamping::None;
	ClampMaxType = EEnvQueryTestClamping::None;
	ScoringEquation = EEnvTestScoreEquation::Linear;
	
	bNormalizeFromZero = false;					// DEPRECATED
	bWorkOnFloatValues = true;
	bDiscardFailedItems = true;					// DEPRECATED
}

void UEnvQueryTest::NormalizeItemScores(FEnvQueryInstance& QueryInstance)
{
	if (!IsScoring())
	{
		return;
	}

	float WeightValue = 0.0f;
	if (!QueryInstance.GetParamValue(Weight, WeightValue, TEXT("Weight")))
	{
		return;
	}

	float MinScore = 0;
	float MaxScore = -BIG_NUMBER;

	if (ClampMinType == EEnvQueryTestClamping::FilterThreshold)
	{
		bool bSuccess = QueryInstance.GetParamValue(FloatFilterMin, MinScore, TEXT("FloatFilterMin"));
		if (!bSuccess)
		{
			UE_LOG(LogEQS, Warning, TEXT("Unable to get FloatFilterMin parameter value from EnvQueryInstance %s"), FloatFilterMin.IsNamedParam() ? *FloatFilterMin.ParamName.ToString() : TEXT("<No name specified>"));
		}
	}
	else if (ClampMinType == EEnvQueryTestClamping::SpecifiedValue)
	{
		bool bSuccess = QueryInstance.GetParamValue(ScoreClampingMin, MinScore, TEXT("ScoreClampingMin"));
		if (!bSuccess)
		{
			UE_LOG(LogEQS, Warning, TEXT("Unable to get ClampMinType parameter value from EnvQueryInstance %s"), ScoreClampingMin.IsNamedParam() ? *ScoreClampingMin.ParamName.ToString() : TEXT("<No name specified>"));
		}
	}

	if (ClampMaxType == EEnvQueryTestClamping::FilterThreshold)
	{
		bool bSuccess = QueryInstance.GetParamValue(FloatFilterMax, MaxScore, TEXT("FloatFilterMax"));
		if (!bSuccess)
		{
			UE_LOG(LogEQS, Warning, TEXT("Unable to get FloatFilterMax parameter value from EnvQueryInstance %s"), FloatFilterMax.IsNamedParam() ? *FloatFilterMax.ParamName.ToString() : TEXT("<No name specified>"));
		}
	}
	else if (ClampMaxType == EEnvQueryTestClamping::SpecifiedValue)
	{
		bool bSuccess = QueryInstance.GetParamValue(ScoreClampingMax, MaxScore, TEXT("ScoreClampingMax"));
		if (!bSuccess)
		{
			UE_LOG(LogEQS, Warning, TEXT("Unable to get ScoreClampingMax parameter value from EnvQueryInstance %s"), ScoreClampingMax.IsNamedParam() ? *ScoreClampingMax.ParamName.ToString() : TEXT("<No name specified>"));
		}
	}

	FEnvQueryItemDetails* DetailInfo = QueryInstance.ItemDetails.GetData();
	if ((ClampMinType == EEnvQueryTestClamping::None) ||
		(ClampMaxType == EEnvQueryTestClamping::None)
	   )
	{
		for (int32 ItemIndex = 0; ItemIndex < QueryInstance.Items.Num(); ItemIndex++, DetailInfo++)
		{
			if (!QueryInstance.Items[ItemIndex].IsValid())
			{
				continue;
			}

			float TestValue = DetailInfo->TestResults[QueryInstance.CurrentTest];
			if (TestValue != UEnvQueryTypes::SkippedItemValue)
			{
				if (ClampMinType == EEnvQueryTestClamping::None)
				{
					MinScore = FMath::Min(MinScore, TestValue);
				}

				if (ClampMaxType == EEnvQueryTestClamping::None)
				{
					MaxScore = FMath::Max(MaxScore, TestValue);
				}
			}
		}
	}

	DetailInfo = QueryInstance.ItemDetails.GetData();
	if (bNormalizeFromZero && MinScore > 0.0f)
	{
		MinScore = 0.0f;
	}

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
				const float ClampedScore = FMath::Clamp(TestValue, MinScore, MaxScore);
				const float NormalizedScore = (ClampedScore - MinScore) / (MaxScore - MinScore);
				// TODO? Add an option to invert the normalized score before applying an equation.
 				const float NormalizedScoreForEquation = /*bMirrorNormalizedScore ? (1.0f - NormalizedScore) :*/ NormalizedScore;
				switch (ScoringEquation)
				{
					case EEnvTestScoreEquation::Linear:
						WeightedScore = WeightValue * NormalizedScoreForEquation;
						break;

					case EEnvTestScoreEquation::InverseLinear:
					{
						// For now, we're avoiding having a separate flag for flipping the direction of the curve
						// because we don't have usage cases yet and want to avoid too complex UI.  If we decide
						// to add that flag later, we'll need to remove this option, since it should just be "mirror
						// curve" plus "Linear".
						float InverseNormalizedScore = (1.0f - NormalizedScoreForEquation);
						WeightedScore = WeightValue * InverseNormalizedScore;
						break;
					}

					case EEnvTestScoreEquation::Square:
						WeightedScore = WeightValue * (NormalizedScoreForEquation * NormalizedScoreForEquation);
						break;

					case EEnvTestScoreEquation::Constant:
						// I know, it's not "constant".  It's "Constant, or zero".  The tooltip should explain that.
						WeightedScore = (NormalizedScoreForEquation > 0) ? WeightValue : 0.0f;
						break;
						
					default:
						break;
				}
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

bool UEnvQueryTest::IsContextPerItem(TSubclassOf<UEnvQueryContext> CheckContext) const
{
	return CheckContext == UEnvQueryContext_Item::StaticClass();
}

FVector UEnvQueryTest::GetItemLocation(FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeVectorCDO ?
		QueryInstance.ItemTypeVectorCDO->GetItemLocation(QueryInstance.RawData.GetData() + QueryInstance.Items[ItemIndex].DataOffset) :
		FVector::ZeroVector;
}

FRotator UEnvQueryTest::GetItemRotation(FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeVectorCDO ?
		QueryInstance.ItemTypeVectorCDO->GetItemRotation(QueryInstance.RawData.GetData() + QueryInstance.Items[ItemIndex].DataOffset) :
		FRotator::ZeroRotator;
}

AActor* UEnvQueryTest::GetItemActor(FEnvQueryInstance& QueryInstance, int32 ItemIndex) const
{
	return QueryInstance.ItemTypeActorCDO ?
		QueryInstance.ItemTypeActorCDO->GetActor(QueryInstance.RawData.GetData() + QueryInstance.Items[ItemIndex].DataOffset) :
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

void UEnvQueryTest::PostLoad()
{
	Super::PostLoad();

	if (bFormatUpdated)
	{
		// Backwards compatibility already handled.
		return;
	}

	switch (Condition)
	{
		case EEnvTestCondition::AtLeast:
			FloatFilterMin = FloatFilter;
			FilterType = EEnvTestFilterType::Minimum;
			break;
		
		case EEnvTestCondition::UpTo:
			FloatFilterMax = FloatFilter;
			FilterType = EEnvTestFilterType::Maximum;
			break;
		
		case EEnvTestCondition::NoCondition:
			TestPurpose = EEnvTestPurpose::Score;
			break;

		case EEnvTestCondition::Match:
			FilterType = EEnvTestFilterType::Match;
			break;

		default:
			UE_LOG(LogEQS, Error, TEXT("Invalid Condition type in UEnvQueryTest::PostLoad!"));
			break;
	}

	if ((WeightModifier != EEnvTestWeight::Skip) && !Weight.IsNamedParam() && (Weight.Value == 0.f))
	{	// We ought to be skipping the test if weight is 0!  Zero makes it pointless!
		WeightModifier = EEnvTestWeight::Skip;
	}

	if (WeightModifier != EEnvTestWeight::Skip && !bWorkOnFloatValues)
	{
		// Working on booleans, so MUST use constant value!
		WeightModifier = EEnvTestWeight::Constant;
	}

	switch (WeightModifier)
	{
		case EEnvTestWeight::None:
			ScoringEquation = EEnvTestScoreEquation::Linear;
			break;

		case EEnvTestWeight::Constant:
			ScoringEquation = EEnvTestScoreEquation::Constant;
			break;

		case EEnvTestWeight::Inverse:
			ScoringEquation = EEnvTestScoreEquation::InverseLinear;
			break;

		case EEnvTestWeight::Square:
			ScoringEquation = EEnvTestScoreEquation::Square;
			break;

		case EEnvTestWeight::Absolute:
			UE_LOG(LogEQS, Warning, TEXT("Absolute weight is no longer supported!  Sticking with default weighting."));
			// falling back to Linear
			ScoringEquation = EEnvTestScoreEquation::Linear;
			break;

		case EEnvTestWeight::Skip:
			TestPurpose = EEnvTestPurpose::Filter;
			if (Condition == EEnvTestCondition::NoCondition)
			{
				UE_LOG(LogEQS, Warning, TEXT("Test was set to neither filter nor score!  Now setting to filter!"));	
			}
			break;

		default:
			UE_LOG(LogEQS, Warning, TEXT("Invalid Weight Modifier type in UEnvQueryTest::PostLoad!"));
			break;
	}

	UpdateTestVersion();
}

void UEnvQueryTest::UpdateTestVersion()
{
	bFormatUpdated = true;
}

#if WITH_EDITOR && USE_EQS_DEBUGGER
void UEnvQueryTest::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) 
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
#if USE_EQS_DEBUGGER
	UEnvQueryManager::NotifyAssetUpdate(NULL);
#endif
}
#endif //WITH_EDITOR && USE_EQS_DEBUGGER

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

	if (!IsScoring())
	{
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithDontScore", "{ParmDesc}, don't score"), Args);
	}
	else if (ScoringEquation == EEnvTestScoreEquation::Constant)
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

	if (!IsScoring())
	{
		ParamDesc = FText::Format(LOCTEXT("ParmDescWithDontScore", "{ParmDesc}, don't score"), Args);
	}
	else if (ScoringEquation == EEnvTestScoreEquation::Constant)
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

void UEnvQueryTest::SetWorkOnFloatValues(bool bWorkOnFloats)
{
	bWorkOnFloatValues = bWorkOnFloats;
	// Make sure FilterType is set to a valid value.
	if (bWorkOnFloats)
	{
		if (FilterType == EEnvTestFilterType::Match)
		{
			FilterType = EEnvTestFilterType::Range;
		}
	}
	else
	{
		if (FilterType != EEnvTestFilterType::Match)
		{
			FilterType = EEnvTestFilterType::Match;
		}

		// Scoring MUST be Constant for boolean tests.
		ScoringEquation = EEnvTestScoreEquation::Constant;
	}
}

#undef LOCTEXT_NAMESPACE
