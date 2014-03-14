// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryGenerator_OnCircle.generated.h"

UCLASS()	
class UEnvQueryGenerator_OnCircle : public UEnvQueryGenerator
{
	GENERATED_UCLASS_BODY()

	/** max distance of path between point and context */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Radius;

	/** items will be generated on a circle this much apart */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	float ItemSpacing;

	/** If you generate items on a piece of circle you define direction of Arc cut here */
	UPROPERTY(EditDefaultsOnly, Category=Generator, meta=(EditCondition="bDefineArc"))
	TSubclassOf<class UEnvQueryContext> ArcDirectionStart;

	/** If you generate items on a piece of circle you define direction of Arc cut here. If not set ArcDirectionStart's rotation will be used*/
	UPROPERTY(EditDefaultsOnly, Category=Generator, meta=(EditCondition="bDefineArc"))
	TSubclassOf<class UEnvQueryContext> ArcDirectionEnd;

	/** If you generate items on a piece of circle you define angle of Arc cut here */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Angle;
	
	UPROPERTY()
	float AngleRadians;

	/** context */
	UPROPERTY(EditAnywhere, Category=Generator)
	TSubclassOf<class UEnvQueryContext> CircleCenter;

	UPROPERTY(EditAnywhere, Category=Generator)
	TEnumAsByte<EEnvQueryTrace::Type> TraceType;

	UPROPERTY(EditAnywhere, Category=Generator, meta=(EditCondition="bUseNavigationRaycast"))
	TSubclassOf<class UNavigationQueryFilter> NavigationFilterClass;

	UPROPERTY()
	uint32 bUseNavigationRaycast:1;

	UPROPERTY()
	uint32 bDefineArc:1;

	virtual void PostLoad() OVERRIDE;

	void GenerateItems(struct FEnvQueryInstance& QueryInstance); 

	virtual FString GetDescriptionTitle() const OVERRIDE;
	virtual FString GetDescriptionDetails() const OVERRIDE;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR

protected:
	FVector CalcDirection(struct FEnvQueryInstance& QueryInstance) const;
};
