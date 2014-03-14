// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTest.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryTest : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Number of test as defined in data asset */
	UPROPERTY()
	int32 TestOrder;

	/** filter for test value */
	UPROPERTY(EditDefaultsOnly, Category=Filter)
	FEnvBoolParam BoolFilter;

	/** filter for test value */
	UPROPERTY(EditDefaultsOnly, Category=Filter)
	FEnvFloatParam FloatFilter;

	/** Weight of test */
	UPROPERTY(EditDefaultsOnly, Category=Weight)
	FEnvFloatParam Weight;

	/** Weight modifier */
	UPROPERTY(EditDefaultsOnly, Category=Weight)
	TEnumAsByte<EEnvTestWeight::Type> WeightModifier;

	/** Cost of test */
	TEnumAsByte<EEnvTestCost::Type> Cost;

	/** Condition for discarding item */
	UPROPERTY(EditDefaultsOnly, Category=Filter)
	TEnumAsByte<EEnvTestCondition::Type> Condition;

	/** Validation: item type that can be used with this test */
	TSubclassOf<UEnvQueryItemType> ValidItemType;

	/** Normalize test result starting from 0 */
	uint32 bNormalizeFromZero : 1;

	/** When set, test operates on float values (e.g. distance, with AtLeast, UpTo conditions),
	 *  otherwise it will accept bool values (e.g. visibility, with Equals condition) */
	UPROPERTY()
	uint32 bWorkOnFloatValues : 1;

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

	/** get description of test */
	virtual FString GetDescriptionTitle() const;
	virtual FString GetDescriptionDetails() const;

	FString DescribeFloatTestParams() const;
	FString DescribeBoolTestParams(const FString& ConditionDesc) const;

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
