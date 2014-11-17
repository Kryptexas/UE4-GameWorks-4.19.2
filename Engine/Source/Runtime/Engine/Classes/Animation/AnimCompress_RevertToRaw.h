// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Reverts any animation compression, restoring the animation to the raw data.
 *
 */

#pragma once
#include "Animation/AnimCompress.h"
#include "AnimCompress_RevertToRaw.generated.h"

UCLASS(deprecated)
class UDEPRECATED_AnimCompress_RevertToRaw : public UAnimCompress
{
	GENERATED_UCLASS_BODY()


protected:
	// Begin UAnimCompress Interface
	virtual void DoReduction(class UAnimSequence* AnimSeq, const TArray<class FBoneData>& BoneData) override;
	// Begin UAnimCompress Interface
};



