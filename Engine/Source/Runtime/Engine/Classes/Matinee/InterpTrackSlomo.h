// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackSlomo.generated.h"

UCLASS(HeaderGroup=Interpolation, meta=( DisplayName = "Slomo Track" ) )
class UInterpTrackSlomo : public UInterpTrackFloatBase
{
	GENERATED_UCLASS_BODY()


	// Begin InterpTrack interface.
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual void UpdateKeyframe(int32 KeyIndex, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	virtual void SetTrackToSensibleDefault() OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	// End InterpTrack interface.

	/** @return the slomo factor we want at the given time. */
	ENGINE_API float GetSlomoFactorAtTime(float Time);
};



