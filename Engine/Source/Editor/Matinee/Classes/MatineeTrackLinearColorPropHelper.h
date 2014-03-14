// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MatineeTrackLinearColorPropHelper.generated.h"

UCLASS()
class UMatineeTrackLinearColorPropHelper : public UMatineeTrackVectorPropHelper
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackHelper Interface
	virtual	bool PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const OVERRIDE;
	virtual void  PostCreateTrack( UInterpTrack *Track, bool bDuplicatingTrack, int32 TrackIndex ) const OVERRIDE;
	// End UInterpTrackHelper Interface
};

