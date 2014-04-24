// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstVectorProp.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstVectorProp : public UInterpTrackInstProperty
{
	GENERATED_UCLASS_BODY()

	/** Pointer to FVector property in TrackObject. */
	FVector* VectorProp;

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	FVector ResetVector;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

