// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstFade.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstFade : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackInst Instance
	virtual void TermTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

