// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
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

	virtual void PostLoad() OVERRIDE;

	void GenerateItems(struct FEnvQueryInstance& QueryInstance); 

	virtual FText GetDescriptionTitle() const OVERRIDE;
	virtual FText GetDescriptionDetails() const OVERRIDE;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR

protected:
	FVector CalcDirection(struct FEnvQueryInstance& QueryInstance) const;

	DECLARE_DELEGATE_SevenParams(FRunTraceSignature, const FVector&, const FVector&, UWorld*, enum ECollisionChannel, const FCollisionQueryParams&, const FVector&, FVector&);

	void RunLineTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos);
	void RunSphereTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos);
	void RunCapsuleTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos);
	void RunBoxTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos);
};
