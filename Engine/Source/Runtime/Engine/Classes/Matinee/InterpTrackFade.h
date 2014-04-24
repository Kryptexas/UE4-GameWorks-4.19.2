// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackFade.generated.h"

UCLASS(HeaderGroup=Interpolation, meta=( DisplayName = "Fade Track" ) )
class UInterpTrackFade : public UInterpTrackFloatBase
{
	GENERATED_UCLASS_BODY()

	/** 
	 * InterpTrackFade
	 *
	 * Special float property track that controls camera fading over time.
	 * Should live in a Director group.
	 *
	 */
	UPROPERTY(EditAnywhere, Category=InterpTrackFade)
	uint32 bPersistFade:1;


	// Begin UInterpTrack interface.
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual void UpdateKeyframe(int32 KeyIndex, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	// End UInterpTrack interface.

	/** @return the amount of fading we want at the given time. */
	ENGINE_API float GetFadeAmountAtTime(float Time);
};



