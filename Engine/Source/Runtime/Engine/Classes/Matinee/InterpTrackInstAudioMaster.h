// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstAudioMaster.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstAudioMaster : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void TermTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

