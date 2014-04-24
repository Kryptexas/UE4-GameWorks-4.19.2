// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstBoolProp.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstBoolProp : public UInterpTrackInstProperty
{
	GENERATED_UCLASS_BODY()

	/** Pointer to boolean property in TrackObject. */
	void* BoolPropertyAddress;

	/** Mask that indicates which bit the boolean property actually uses of the value pointed to by BoolProp. */
	UPROPERTY(transient)
	UBoolProperty* BoolProperty;

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	bool ResetBool;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

