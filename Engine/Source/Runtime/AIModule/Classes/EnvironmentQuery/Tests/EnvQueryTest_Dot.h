// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_Dot.generated.h"

UCLASS(MinimalAPI)
class UEnvQueryTest_Dot : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

	/** defines direction of first line used by test */
	UPROPERTY(EditDefaultsOnly, Category=Dot)
	FEnvDirection LineA;

	/** defines direction of second line used by test */
	UPROPERTY(EditDefaultsOnly, Category=Dot)
	FEnvDirection LineB;

	void RunTest(struct FEnvQueryInstance& QueryInstance);

	virtual FString GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

protected:

	/** helper function: gather directions from context pairs */
	void GatherLineDirections(TArray<FVector>& Directions, struct FEnvQueryInstance& QueryInstance, const FVector& ItemLocation,
		TSubclassOf<class UEnvQueryContext> LineFrom, TSubclassOf<class UEnvQueryContext> LineTo);

	/** helper function: gather directions from context */
	void GatherLineDirections(TArray<FVector>& Directions, struct FEnvQueryInstance& QueryInstance, const FRotator& ItemRotation,
		TSubclassOf<class UEnvQueryContext> LineDirection);

	/** helper function: gather directions from proper contexts */
	void GatherLineDirections(TArray<FVector>& Directions, struct FEnvQueryInstance& QueryInstance,
		TSubclassOf<class UEnvQueryContext> LineFrom, TSubclassOf<class UEnvQueryContext> LineTo, TSubclassOf<class UEnvQueryContext> LineDirection, bool bUseDirectionContext,
		const FVector& ItemLocation = FVector::ZeroVector, const FRotator& ItemRotation = FRotator::ZeroRotator);

	/** helper function: check if contexts are updated per item */
	bool RequiresPerItemUpdates(TSubclassOf<class UEnvQueryContext> LineFrom, TSubclassOf<class UEnvQueryContext> LineTo, TSubclassOf<class UEnvQueryContext> LineDirection, bool bUseDirectionContext);
};
