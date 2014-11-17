// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstAudioMaster.generated.h"

UCLASS()
class UInterpTrackInstAudioMaster : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) override;
	virtual void TermTrackInst(UInterpTrack* Track) override;
	// End UInterpTrackInst Instance
};

