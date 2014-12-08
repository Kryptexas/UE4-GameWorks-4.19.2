// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvQueryTest.generated.h"

struct FEnvQueryInstance;
class UEnvQueryItemType;
class UEnvQueryContext;
#if WITH_EDITOR
struct FPropertyChangedEvent;
#endif // WITH_EDITOR

UCLASS(Abstract)
class AIMODULE_API UEnvQueryTest : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Number of test as defined in data asset */
	UPROPERTY()
	int32 TestOrder;

	UPROPERTY(EditDefaultsOnly, Category=Test,
		Meta=(ToolTip="The purpose of this test.  Should it be used for filtering possible results, scoring them, or both?"))
	TEnumAsByte<EEnvTestPurpose::Type> TestPurpose;
	
	UPROPERTY(EditDefaultsOnly, Category=Filter,
		Meta=(ToolTip="Does this test filter out results that are below a lower limit, above an upper limit, or both?  Or does it just look for a matching value?"))
	TEnumAsByte<EEnvTestFilterType::Type> FilterType;

	/** Boolean value that must be matched in order for scoring to occur or filtering test to pass. */
	UPROPERTY(EditDefaultsOnly, Category=Filter,
		Meta=(ToolTip="Boolean value that must be matched in order for scoring to occur or filtering test to pass."))
	FEnvBoolParam BoolFilter;

	// TODO: REMOVE, Deprecated!
	/** filter for test value; currently this is the ONLY value, because we can only filter in one direction. */
	UPROPERTY()
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
	UPROPERTY()
	TEnumAsByte<EEnvTestCondition::Type> Condition;

	// TODO: REMOVE, Deprecated!
	/** Weight modifier */
	UPROPERTY()
	TEnumAsByte<EEnvTestWeight::Type> WeightModifier;

	/** Cost of test */
	TEnumAsByte<EEnvTestCost::Type> Cost;

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

	UPROPERTY(EditDefaultsOnly, Category=Score,
		Meta=(ToolTip="The shape of the curve equation to apply to the normalized score before multiplying by Weight."))
	TEnumAsByte<EEnvTestScoreEquation::Type> ScoringEquation;

	// For now, I'm keeping "Weight".  TODO: Reconsider Weight vs. "Scoring Factor", Constant, Power, etc. once we have a final plan for what properties are available.
	/** Weight of test */
	UPROPERTY(EditDefaultsOnly, Category=Score,
		Meta=(ToolTip="The weight (factor) by which to multiply the normalized score after the scoring equation is applied.",
			  ClampMin="0.001", UIMin="0.001"))
	FEnvFloatParam Weight;

	/** Validation: item type that can be used with this test */
	TSubclassOf<UEnvQueryItemType> ValidItemType;

	void SetWorkOnFloatValues(bool bWorkOnFloats);
	FORCEINLINE bool GetWorkOnFloatValues() const { return bWorkOnFloatValues; }

	FORCEINLINE bool CanRunAsFinalCondition() const 
	{ 
		return (TestPurpose != EEnvTestPurpose::Score)							// We are filtering and...
				&& ((TestPurpose == EEnvTestPurpose::Filter)					// Either we are NOT scoring at ALL or...
					|| (ScoringEquation == EEnvTestScoreEquation::Constant));	// We are giving a constant score value for passing.
	}

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
	UPROPERTY()
	uint32 bDiscardFailedItems : 1;

	/** Function that does the actual work */
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const { checkNoEntry(); }

	/** check if test supports item type */
	bool IsSupportedItem(TSubclassOf<UEnvQueryItemType> ItemType) const;

	/** check if context needs to be updated for every item */
	bool IsContextPerItem(TSubclassOf<UEnvQueryContext> CheckContext) const;

	/** helper: get location of item */
	FVector GetItemLocation(FEnvQueryInstance& QueryInstance, int32 ItemIndex) const;

	/** helper: get location of item */
	FRotator GetItemRotation(FEnvQueryInstance& QueryInstance, int32 ItemIndex) const;

	/** helper: get actor from item */
	AActor* GetItemActor(FEnvQueryInstance& QueryInstance, int32 ItemIndex) const;

	/** normalize scores in range */
	void NormalizeItemScores(FEnvQueryInstance& QueryInstance);

	FORCEINLINE bool IsScoring() const { return (TestPurpose != EEnvTestPurpose::Filter); } // ((TestPurpose == EEnvTestPurpose::Score) || (TestPurpose == EEnvTestPurpose::FilterAndScore));
	FORCEINLINE bool IsFiltering() const { return (TestPurpose != EEnvTestPurpose::Score); } // ((TestPurpose == EEnvTestPurpose::Filter) || (TestPurpose == EEnvTestPurpose::FilterAndScore));

	/** get description of test */
	virtual FString GetDescriptionTitle() const;
	virtual FText GetDescriptionDetails() const;

	FText DescribeFloatTestParams() const;
	FText DescribeBoolTestParams(const FString& ConditionDesc) const;

	virtual void PostLoad() override;

	/** update to latest version after spawning */
	void UpdateTestVersion();

#if WITH_EDITOR && USE_EQS_DEBUGGER
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR && USE_EQS_DEBUGGER
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE bool UEnvQueryTest::IsSupportedItem(TSubclassOf<UEnvQueryItemType> ItemType) const
{
	return ItemType && (ItemType == ValidItemType || ItemType->IsChildOf(ValidItemType));
};
