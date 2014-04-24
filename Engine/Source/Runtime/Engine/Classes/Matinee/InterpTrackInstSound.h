// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstSound.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstSound : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	float LastUpdatePosition;

	UPROPERTY(transient)
	class UAudioComponent* PlayAudioComp;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void TermTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

