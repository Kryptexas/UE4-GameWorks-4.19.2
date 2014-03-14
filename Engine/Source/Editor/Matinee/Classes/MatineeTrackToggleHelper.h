// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MatineeTrackToggleHelper.generated.h"

UCLASS()
class UMatineeTrackToggleHelper : public UInterpTrackHelper
{
	GENERATED_UCLASS_BODY()


	// Begin UInterpTrackHelper Interface
	virtual	bool PreCreateKeyframe( UInterpTrack *Track, float KeyTime ) const OVERRIDE;
	virtual void  PostCreateKeyframe( UInterpTrack *Track, int32 KeyIndex ) const OVERRIDE;
	// End UInterpTrackHelper Interface

	void OnAddKeyTextEntry(const FString& ChosenText, IMatineeBase* Matinee, UInterpTrack* Track);
};

