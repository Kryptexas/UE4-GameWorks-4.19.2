// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstDirector.generated.h"

UCLASS(HeaderGroup=Interpolation, MinimalAPI)
class UInterpTrackInstDirector : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class AActor* OldViewTarget;


	// Begin UInterpTrackInst Instance
	virtual void TermTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

