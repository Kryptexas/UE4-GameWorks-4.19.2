// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstFloatParticleParam.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstFloatParticleParam : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	float ResetFloat;


	// Begin UInterpTrackInst Instance
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

