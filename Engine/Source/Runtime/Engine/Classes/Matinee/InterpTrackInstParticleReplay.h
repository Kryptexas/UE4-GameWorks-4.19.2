// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstParticleReplay.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstParticleReplay : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** 
	 *	Position we were in last time we evaluated.
	 *	During UpdateTrack, events between this time and the current time will be processed.
	 */
	UPROPERTY()
	float LastUpdatePosition;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

