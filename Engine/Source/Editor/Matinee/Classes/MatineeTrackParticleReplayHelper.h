// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MatineeTrackParticleReplayHelper.generated.h"

UCLASS()
class UMatineeTrackParticleReplayHelper : public UInterpTrackHelper
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackHelper Interface
	virtual	bool PreCreateKeyframe( UInterpTrack *Track, float KeyTime ) const OVERRIDE;
	virtual void  PostCreateKeyframe( UInterpTrack *Track, int32 KeyIndex ) const OVERRIDE;
	// End UInterpTrackHelper Interface
};

