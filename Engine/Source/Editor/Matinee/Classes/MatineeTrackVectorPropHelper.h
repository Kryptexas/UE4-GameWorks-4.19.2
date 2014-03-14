// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "MatineeTrackVectorPropHelper.generated.h"

UCLASS()
class UMatineeTrackVectorPropHelper : public UInterpTrackHelper
{
	GENERATED_UCLASS_BODY()


	/**
	 * Pops up a dialog letting user choose between a set of properties, then checks to see if that property has been bound to yet.
	 *
	 * @param PropNames		Possible property names to select from.
	 *
	 * @return true if the property selected was acceptable, false otherwise.
	 */
	virtual bool ChooseProperty(TArray<FName> &PropNames) const;

	// Begin UInterpTrackHelper Interface
	virtual	bool PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const OVERRIDE;
	virtual void  PostCreateTrack( UInterpTrack *Track, bool bDuplicatingTrack, int32 TrackIndex ) const OVERRIDE;
	// End UInterpTrackHelper Interface

	void OnCreateTrackTextEntry(const FString& ChosenText, TWeakPtr<SWindow> Window, FString* OutputString);
};

