// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MatineeTrackLinearColorPropHelper.generated.h"


UCLASS()
class UMatineeTrackLinearColorPropHelper : public UMatineeTrackVectorPropHelper
{
	GENERATED_BODY()
public:
	UMatineeTrackLinearColorPropHelper(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	// UInterpTrackHelper interface

	virtual	bool PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const override;
	virtual void  PostCreateTrack( UInterpTrack *Track, bool bDuplicatingTrack, int32 TrackIndex ) const override;
};

