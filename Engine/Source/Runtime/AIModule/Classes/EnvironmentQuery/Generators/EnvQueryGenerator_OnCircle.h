// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_OnCircle.generated.h"

UCLASS()	
class UEnvQueryGenerator_OnCircle : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_UCLASS_BODY()

	/** max distance of path between point and context */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Radius;

	/** items will be generated on a circle this much apart */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam ItemSpacing;

	/** If you generate items on a piece of circle you define direction of Arc cut here */
	UPROPERTY(EditDefaultsOnly, Category=Generator, meta=(EditCondition="bDefineArc"))
	FEnvDirection ArcDirection;

	/** If you generate items on a piece of circle you define angle of Arc cut here */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Angle;
	
	UPROPERTY()
	float AngleRadians;

	/** context */
	UPROPERTY(EditAnywhere, Category=Generator)
	TSubclassOf<class UEnvQueryContext> CircleCenter;

	/** horizontal trace for nearest obstacle */
	UPROPERTY(EditAnywhere, Category=Generator)
	FEnvTraceData TraceData;

	UPROPERTY()
	uint32 bDefineArc:1;

	virtual void PostLoad() override;

	void GenerateItems(struct FEnvQueryInstance& QueryInstance); 

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

protected:
	FVector CalcDirection(struct FEnvQueryInstance& QueryInstance) const;
};
