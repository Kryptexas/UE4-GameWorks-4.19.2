// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstEvent.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstEvent : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** 
	 *	Position we were in last time we evaluated Events. 
	 *	During UpdateTrack, events between this time and the current time will be fired. 
	 */
	UPROPERTY()
	float LastUpdatePosition;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

