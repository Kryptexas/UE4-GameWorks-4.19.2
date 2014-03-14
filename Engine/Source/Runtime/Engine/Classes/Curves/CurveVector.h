// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "CurveVector.generated.h"

UCLASS(BlueprintType, MinimalAPI)
class UCurveVector : public UCurveBase
{
	GENERATED_UCLASS_BODY()

	/** Legacy keyframe data. */
	UPROPERTY()
	FInterpCurveVector VectorKeys_DEPRECATED;

	/** Keyframe data, one curve for X, Y and Z */
	UPROPERTY()
	FRichCurve FloatCurves[3];

	/** Evaluate this float curve at the specified time */
	UFUNCTION(BlueprintCallable, Category="Math|Curves")
	ENGINE_API FVector GetVectorValue(float InTime) const;

	// UObject interface
	virtual void PostLoad() OVERRIDE;

	// Begin FCurveOwnerInterface
	virtual TArray<FRichCurveEditInfoConst> GetCurves() const OVERRIDE;
	virtual TArray<FRichCurveEditInfo> GetCurves() OVERRIDE;

	/** Determine if Curve is the same */
	ENGINE_API bool operator == (const UCurveVector& Curve) const;
};

