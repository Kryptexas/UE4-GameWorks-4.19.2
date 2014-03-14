// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Blend Space 1D. Contains 1 axis blend 'space'
 *
 */

#pragma once

#include "AnimInterpFilter.h"
#include "BlendSpace1D.generated.h"

UCLASS(config=Engine, hidecategories=Object, dependson=UVimInstance, MinimalAPI, BlueprintType)
class UBlendSpace1D : public UBlendSpaceBase
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category=BlendSpace)
	bool bDisplayEditorVertically;
#endif

	/** Drive animation speed by blend input position **/
	UPROPERTY()
	bool bScaleAnimation;

	/** return true if all sample data is additive **/
	virtual bool IsValidAdditive() const OVERRIDE;

protected:
	// Begin UBlendSpaceBase interface
	virtual void SnapToBorder(FBlendSample& Sample) const OVERRIDE;
	virtual EBlendSpaceAxis GetAxisToScale() const OVERRIDE;
	virtual bool IsSameSamplePoint(const FVector& SamplePointA, const FVector& SamplePointB) const OVERRIDE;
	virtual void GetRawSamplesFromBlendInput(const FVector &BlendInput, TArray<FGridBlendSample> & OutBlendSamples) const OVERRIDE;
	// End UBlendSpaceBase interface

private:

	/** Returns the distance each division of the param is */
	float CalculateParamStep() const;

	/** 
	 * Calculate threshold for sample points - if within this threshold, considered to be same, so reject it
	 * this is to avoid any accidental same points of samples entered 
	 */
	float CalculateThreshold() const;
};

