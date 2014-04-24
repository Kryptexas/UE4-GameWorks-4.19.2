// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InterpTrackInstFloatMaterialParam.generated.h"

UCLASS(HeaderGroup=Interpolation)
class UInterpTrackInstFloatMaterialParam : public UInterpTrackInst
{
	GENERATED_UCLASS_BODY()

	/** MIDs we're using to set the desired parameter. */
	UPROPERTY()
	TArray<class UMaterialInstanceDynamic*> MaterialInstances;

	/** Saved values for restoring state when exiting Matinee. */
	UPROPERTY()
	TArray<float> ResetFloats;

	/** Primitive components on which materials have been overridden. */
	UPROPERTY()
	TArray<struct FPrimitiveMaterialRef> PrimitiveMaterialRefs;

	/** track we are an instance of - used in the editor to propagate changes to the track's Materials array immediately */
	UPROPERTY()
	class UInterpTrackFloatMaterialParam* InstancedTrack;


	// Begin UInterpTrackInst Instance
	virtual void InitTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void TermTrackInst(UInterpTrack* Track) OVERRIDE;
	virtual void SaveActorState(UInterpTrack* Track) OVERRIDE;
	virtual void RestoreActorState(UInterpTrack* Track) OVERRIDE;
	// End UInterpTrackInst Instance
};

