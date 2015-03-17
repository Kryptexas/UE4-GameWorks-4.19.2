// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstColorProp.generated.h"

UCLASS()
class UInterpTrackInstColorProp : public UInterpTrackInstProperty
{
	GENERATED_BODY()
public:
	UInterpTrackInstColorProp(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Pointer to color property in TrackObject. */
	FColor* ColorProp;

	/** Saved value for restoring state when exiting Matinee. */
	UPROPERTY()
	FColor ResetColor;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) override;
	virtual void SaveActorState(UInterpTrack* Track) override;
	virtual void RestoreActorState(UInterpTrack* Track) override;
	// End UInterpTrackInst Instance
};

