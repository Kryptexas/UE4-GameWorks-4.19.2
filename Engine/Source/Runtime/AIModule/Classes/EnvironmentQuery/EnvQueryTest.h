// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvQueryTest.generated.h"

UCLASS(Abstract)
class AIMODULE_API UEnvQueryTest : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Number of test as defined in data asset */
	UPROPERTY()
	int32 TestOrder;

	UPROPERTY(EditDefaultsOnly, Category=Filter)
	TEnumAsByte<EEnvTestPurpose::Type> TestPurpose;
	
	UPROPERTY(EditDefaultsOnly, Category=Filter)
	TEnumAsByte<EEnvTestFilterType::Type> FilterType;

	/** Boolean value that must be matched in order for scoring to occur or filtering test to pass. */
	UPROPERTY(EditDefaultsOnly, Category=Filter)
	FEnvBoolParam BoolFilter;

	/** filter for test value; currently this is the ONLY value, because we can only filter in one direction. */
	UPROPERTY(EditDefaultsOnly, Category=Filter)
	FEnvFloatParam FloatFilter;

	UPROPERTY(EditDefaultsOnly, Category=Filter,
		Meta=(ToolTip="Minimum limit (inclusive) of valid values for the raw test value.  Lower values will be discarded as invalid."))
	FEnvFloatParam FloatFilterMin;

	UPROPERTY(EditDefaultsOnly, Category=Filter,
		Meta=(ToolTip="Maximum limit (inclusive) of valid values for the raw test value.  Higher values will be discarded as invalid."))
	FEnvFloatParam FloatFilterMax;

	// Using this property just as a way to do file version conversion without having to up the version number!
	UPROPERTY()
	bool bFormatUpdated;

	// TODO: REMOVE, Deprecated!
	/** Condition for discarding item */
	UPROPERTY(EditDefaultsOnly, Category=DeprecatedFilt)
	TEnumAsByte<EEnvTestCondition::Type> Condition;

	// TODO: REMOVE, Deprecated!
	/** Weight modifier */
	UPROPERTY(EditDefaultsOnly, Category=Weight)
	TEnumAsByte<EEnvTestWeight::Type> WeightModifier;

	/** Cost of test */
	TEnumAsByte<EEnvTestCost::Type> Cost;

// 	UPROPERTY(EditDefaultsOnly, Category=Score, Meta=(DisplayName="Take Absolute Value of Test Result?",
// 		ToolTip="If checked, the raw value of the test result will be converted to an absolute value BEFORE clamping.  Otherwise, the raw value will reamin as-is."))
// 	bool bUseAbsoluteValueBeforeClamping;

	UPROPERTY(EditDefaultsOnly, Category=Score,
		Meta=(ToolTip="How should the lower bound for normalization of the raw test value before applying the scoring formula be determined?  Should it use the lowest value found (tested), the lower threshold for filtering, or a separate specified normalization minimum?"))
	TEnumAsByte<EEnvQueryTestClamping::Type> ClampMinType;

	UPROPERTY(EditDefaultsOnly, Category=Score,
		Meta=(ToolTip="How should the upper bound for normalization of the raw test value before applying the scoring formula be determined?  Should it use the highes value found (tested), the upper threshold for filtering, or a separate specified normalization maximum?"))
	TEnumAsByte<EEnvQueryTestClamping::Type> ClampMaxType;

	UPROPERTY(EditDefaultsOnly, Category=Score,
		Meta=(ToolTip="Minimum value to use to normalize the raw test value before applying scoring formula."))
	FEnvFloatParam ScoreClampingMin;

	UPROPERTY(EditDefaultsOnly, Category=Score,
		Meta=(ToolTip="Maximum value to use to normalize the raw test value before applying scoring formula."))
	FEnvFloatParam ScoreClampingMax;

	// TODO: Should we hide this option for boolean tests?  In those cases, presumably scoring flipped would just
	// reverse the true/false match we're looking for.
// 	UPROPERTY(EditDefaultsOnly, Category=Score,
// 		Meta=(Tooltip="If checked, reverse the normalized score by taking (1.0 - Normalized Score) before applying the curve scoring equation.  Use this if lower test values should result in a higher score."))
// 	bool bMirrorNormalizedScore;

	UPROPERTY(EditDefaultsOnly, Category=Score)
	TEnumAsByte<EEnvTestScoreEquation::Type> ScoringEquation;

	// For now, I'm keeping "Weight".  TODO: Reconsider Weight vs. "Scoring Factor", Constant, Power, etc. once we have a final plan for what properties are available.
	/** Weight of test */
	UPROPERTY(EditDefaultsOnly, Category=Score)
	FEnvFloatParam Weight;

// 	UPROPERTY(EditDefaultsOnly, Category=Score, Meta=(DisplayName="Factor"))
// 	FEnvFloatParam ScoringFactor;

// 	UPROPERTY(EditDefaultsOnly, Category=Score, Meta=(DisplayName="Constant"))
// 	FEnvFloatParam ScoringConstant;

// Only need this if we add in support for parametric equations
// 	UPROPERTY(EditDefaultsOnly, Category=Score, Meta=(DisplayName="Power"))
// 	FEnvFloatParam ScoringPower;

	/** Validation: item type that can be used with this test */
	TSubclassOf<UEnvQueryItemType> ValidItemType;

	void SetWorkOnFloatValues(bool bWorkOnFloats);
	FORCEINLINE bool GetWorkOnFloatValues() const { return bWorkOnFloatValues; }

	// TODO: REMOVE, Deprecated!
	/** Normalize test result starting from 0 */
	uint32 bNormalizeFromZero : 1;

private:
	/** When set, test operates on float values (e.g. distance, with AtLeast, UpTo conditions),
	 *  otherwise it will accept bool values (e.g. visibility, with Equals condition) */
	UPROPERTY()
	uint32 bWorkOnFloatValues : 1;

public:
	// TODO: REMOVE, Deprecated!
	/** If set, items not passing filter condition will be removed from query, otherwise they will receive score 0 */
	UPROPERTY(EditDefaultsOnly, Category=DeprecatedFilt)
	uint32 bDiscardFailedItems : 1;

	FExecuteTestSignature ExecuteDelegate;

	/** check if test supports item type */
	bool IsSupportedItem(TSubclassOf<class UEnvQueryItemType> ItemType) const;

	/** check if context needs to be updated for every item */
	bool IsContextPerItem(TSubclassOf<class UEnvQueryContext> CheckContext) const;

	/** helper: get location of item */
	FVector GetItemLocation(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const;

	/** helper: get location of item */
	FRotator GetItemRotation(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const;

	/** helper: get actor from item */
	AActor* GetItemActor(struct FEnvQueryInstance& QueryInstance, int32 ItemIndex) const;

	/** normalize scores in range */
	void NormalizeItemScores(struct FEnvQueryInstance& QueryInstance);

	FORCEINLINE bool IsScoring() const { return (TestPurpose != EEnvTestPurpose::Filter); } // ((TestPurpose == EEnvTestPurpose::Score) || (TestPurpose == EEnvTestPurpose::FilterAndScore));
	FORCEINLINE bool IsFiltering() const { return (TestPurpose != EEnvTestPurpose::Score); } // ((TestPurpose == EEnvTestPurpose::Filter) || (TestPurpose == EEnvTestPurpose::FilterAndScore));

	/** get description of test */
	virtual FString GetDescriptionTitle() const;
	virtual FText GetDescriptionDetails() const;

	FText DescribeFloatTestParams() const;
	FText DescribeBoolTestParams(const FString& ConditionDesc) const;

	virtual void PostLoad() OVERRIDE;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif //WITH_EDITOR
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE bool UEnvQueryTest::IsSupportedItem(TSubclassOf<class UEnvQueryItemType> ItemType) const
{
	return ItemType && (ItemType == ValidItemType || ItemType->IsChildOf(ValidItemType));
};
