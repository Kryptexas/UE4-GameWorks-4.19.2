// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MatineeTrackBoolPropHelper.generated.h"

UCLASS()
class UMatineeTrackBoolPropHelper : public UInterpTrackHelper
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackHelper Interface
	virtual	bool PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const OVERRIDE;
	virtual void  PostCreateTrack( UInterpTrack *Track, bool bDuplicatingTrack, int32 TrackIndex ) const OVERRIDE;
	// End UInterpTrackHelper Interface

	void OnCreateTrackTextEntry(const FString& ChosenText, TWeakPtr<SWindow> Window, FString* OutputString);
};

