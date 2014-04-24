// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstLinearColorProp.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstLinearColorProp : public UInterpTrackInstProperty
{
	GENERATED_UCLASS_BODY()

	/** Pointer to color property in TrackObject. */
	FLinearColor* ColorProp;

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	FLinearColor ResetColor;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

