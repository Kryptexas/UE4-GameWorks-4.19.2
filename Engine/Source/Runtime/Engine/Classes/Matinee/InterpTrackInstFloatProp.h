// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstFloatProp.generated.h"

UCLASS(HeaderGroup=Interpolation, MinimalAPI)
class UInterpTrackInstFloatProp : public UInterpTrackInstProperty
{
	GENERATED_UCLASS_BODY()

	/** Pointer to float property in TrackObject. */
	float* FloatProp;

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	float ResetFloat;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

