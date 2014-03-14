// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTest_Dot.generated.h"

UENUM()
namespace EEnvTestDot
{
	enum Type
	{
		Segment		UMETA(DisplayName="Two Points",ToolTip="Direction from location of one context to another."),
		Direction	UMETA(DisplayName="Rotation",ToolTip="Context's rotation will be used as a direction."),
	};
}

UCLASS(MinimalAPI)
class UEnvQueryTest_Dot : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

	/** line A: start context */
	UPROPERTY(EditDefaultsOnly, Category=Dot, meta=(DisplayName="Line A: From"))
	TSubclassOf<class UEnvQueryContext> LineAFrom;

	/** line A: finish context */
	UPROPERTY(EditDefaultsOnly, Category=Dot, meta=(DisplayName="Line A: To"))
	TSubclassOf<class UEnvQueryContext> LineATo;

	/** line A: direction context */
	UPROPERTY(EditDefaultsOnly, Category=Dot, meta=(DisplayName="Line A: Rotation"))
	TSubclassOf<class UEnvQueryContext> LineADirection;

	/** line B: start context */
	UPROPERTY(EditDefaultsOnly, Category=Dot, meta=(DisplayName="Line B: From"))
	TSubclassOf<class UEnvQueryContext> LineBFrom;

	/** line B: finish context */
	UPROPERTY(EditDefaultsOnly, Category=Dot, meta=(DisplayName="Line B: To"))
	TSubclassOf<class UEnvQueryContext> LineBTo;

	/** line B: direction context */
	UPROPERTY(EditDefaultsOnly, Category=Dot, meta=(DisplayName="Line B: Rotation"))
	TSubclassOf<class UEnvQueryContext> LineBDirection;

	/** defines direction of first line used by test */
	UPROPERTY(EditDefaultsOnly, Category=Dot)
	TEnumAsByte<EEnvTestDot::Type> LineA;

	/** defines direction of second line used by test */
	UPROPERTY(EditDefaultsOnly, Category=Dot)
	TEnumAsByte<EEnvTestDot::Type> LineB;

	void RunTest(struct FEnvQueryInstance& QueryInstance);

	virtual FString GetDescriptionTitle() const OVERRIDE;
	virtual FString GetDescriptionDetails() const OVERRIDE;

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
