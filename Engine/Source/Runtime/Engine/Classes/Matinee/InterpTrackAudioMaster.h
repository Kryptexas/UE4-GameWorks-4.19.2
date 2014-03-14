// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackAudioMaster.generated.h"

UCLASS(HeaderGroup=Interpolation, MinimalAPI, meta=( DisplayName = "Audio Master Track" ) )
class UInterpTrackAudioMaster : public UInterpTrackVectorBase
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrack interface.
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual void UpdateKeyframe(int32 KeyIndex, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	virtual void SetTrackToSensibleDefault() OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	// End UInterpTrack interface.

	/** Return the sound volume scale for the specified time */
	float GetVolumeScaleForTime( float Time ) const;

	/** Return the sound pitch scale for the specified time */
	float GetPitchScaleForTime( float Time ) const;

};



