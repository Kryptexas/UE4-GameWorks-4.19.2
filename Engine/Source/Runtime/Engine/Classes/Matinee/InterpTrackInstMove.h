// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstMove.generated.h"

UCLASS(HeaderGroup=Interpolation, MinimalAPI)
class UInterpTrackInstMove : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** Saved position. Used in editor for resetting when quitting Matinee. */
	UPROPERTY()
	FVector ResetLocation;

	/** Saved rotation. Used in editor for resetting when quitting Matinee. */
	UPROPERTY()
	FRotator ResetRotation;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

