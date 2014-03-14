// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstColorProp.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstColorProp : public UInterpTrackInstProperty
{
	GENERATED_UCLASS_BODY()

	/** Pointer to color property in TrackObject. */
	FColor* ColorProp;

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	FColor ResetColor;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

