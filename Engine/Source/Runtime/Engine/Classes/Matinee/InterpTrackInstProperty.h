// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstProperty.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstProperty : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** Function to call after updating the value of the color property. */
	UPROPERTY()
	class UProperty* InterpProperty;

	/** Pointer to the UObject instance that is the outer of the color property we are interpolating on, this is used to process the property update callback. */
	UPROPERTY()
	class UObject* PropertyOuterObjectInst;


	/**
	 * Retrieve the update callback from the interp property's metadata and stores it.
	 *
	 * @param InActor			Actor we are operating on.
	 * @param TrackProperty		Property we are interpolating.
	 */
	void SetupPropertyUpdateCallback(AActor* InActor, const FName& TrackPropertyName);

	/** 
	 * Attempt to call the property update callback.
	 */
	void CallPropertyUpdateCallback();

	// Begin UInterpTrackInst Instance
	virtual void TermTrackInst(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

