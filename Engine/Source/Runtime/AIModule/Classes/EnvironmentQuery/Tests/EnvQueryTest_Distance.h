// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_Distance.generated.h"

class UEnvQueryContext;

UENUM()
namespace EEnvTestDistance
{
	enum Type
	{
		Distance3D,
		Distance2D,
		DistanceZ,
	};
}

UCLASS()
class UEnvQueryTest_Distance : public UEnvQueryTest
{
	GENERATED_BODY()
public:
	UEnvQueryTest_Distance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** testing mode */
	UPROPERTY(EditDefaultsOnly, Category=Distance)
	TEnumAsByte<EEnvTestDistance::Type> TestMode;

	/** context */
	UPROPERTY(EditDefaultsOnly, Category=Distance)
	TSubclassOf<UEnvQueryContext> DistanceTo;

	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};
