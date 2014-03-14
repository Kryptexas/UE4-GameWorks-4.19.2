// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "MatineeTrackAnimControlHelper.generated.h"

UCLASS()
class UMatineeTrackAnimControlHelper : public UInterpTrackHelper
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackHelper Interface
	virtual	bool PreCreateTrack( UInterpGroup* Group, const UInterpTrack *TrackDef, bool bDuplicatingTrack, bool bAllowPrompts ) const OVERRIDE;
	virtual void  PostCreateTrack( UInterpTrack *Track, bool bDuplicatingTrack, int32 TrackIndex ) const OVERRIDE;
	virtual	bool PreCreateKeyframe( UInterpTrack *Track, float KeyTime ) const OVERRIDE;
	virtual void  PostCreateKeyframe( UInterpTrack *Track, int32 KeyIndex ) const OVERRIDE;
	// End UInterpTrackHelper Interface

	void OnAddKeyTextEntry(const class FAssetData& AssetData, IMatineeBase* Matinee, UInterpTrack* Track);
	void OnCreateTrackTextEntry(const FString& ChosenText, TSharedRef<SWindow> Window, FString* OutputString);
};

