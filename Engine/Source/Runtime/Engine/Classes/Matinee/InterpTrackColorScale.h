// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** 
 * InterpTrackColorScale
 *
 */

#pragma once
#include "InterpTrackColorScale.generated.h"

UCLASS(HeaderGroup=Interpolation, meta=( DisplayName = "Color Scale Track" ) )
class UInterpTrackColorScale : public UInterpTrackVectorBase
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrack interface.
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual void UpdateKeyframe(int32 KeyIndex, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	virtual void SetTrackToSensibleDefault() OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	// Begin UInterpTrack interface.

	/** Return the blur alpha we want at the given time. */
	ENGINE_API FVector GetColorScaleAtTime(float Time);
};



