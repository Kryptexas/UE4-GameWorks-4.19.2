// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Random.h"
#include "EnvironmentQuery/Items/EnvQueryItemType.h"

UEnvQueryTest_Random::UEnvQueryTest_Random(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::Low;

	// "Random" is completely agnostic on item type!  Any type at all is valid!
	ValidItemType = UEnvQueryItemType::StaticClass();
}

void UEnvQueryTest_Random::RunTest(FEnvQueryInstance& QueryInstance) const
{
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

	// loop through all items
	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		It.SetScore(TestPurpose, FilterType, FMath::FRand(), MinThresholdValue, MaxThresholdValue);
	}
}

FString UEnvQueryTest_Random::GetDescriptionTitle() const
{
	return Super::GetDescriptionTitle();
}

FText UEnvQueryTest_Random::GetDescriptionDetails() const
{
	return DescribeFloatTestParams();
}
