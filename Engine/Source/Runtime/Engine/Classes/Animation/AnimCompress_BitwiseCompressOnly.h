// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Bitwise animation compression only; performs no key reduction.
 *
 */

#pragma once
#include "AnimCompress_BitwiseCompressOnly.generated.h"

UCLASS(MinimalAPI)
class UAnimCompress_BitwiseCompressOnly : public UAnimCompress
{
	GENERATED_UCLASS_BODY()


protected:
	// Begin UAnimCompress Interface
	virtual void DoReduction(class UAnimSequence* AnimSeq, const TArray<class FBoneData>& BoneData) OVERRIDE;
	// Begin UAnimCompress Interface
};



