// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstToggle.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstToggle : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=InterpTrackInstToggle)
	TEnumAsByte<enum ETrackToggleAction> Action;

	/** 
	 *	Position we were in last time we evaluated.
	 *	During UpdateTrack, toggles between this time and the current time will be processed.
	 */
	UPROPERTY()
	float LastUpdatePosition;

	/** Cached 'active' state for the toggleable actor before we possessed it; restored when Matinee exits */
	UPROPERTY()
	uint32 bSavedActiveState:1;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

