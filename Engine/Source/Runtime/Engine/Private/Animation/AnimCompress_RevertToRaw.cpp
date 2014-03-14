// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimCompress_RevertToRaw.cpp: Reverts any animation compression, restoring the animation to the raw data.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AnimationUtils.h"
#include "AnimEncoding.h"

UDEPRECATED_AnimCompress_RevertToRaw::UDEPRECATED_AnimCompress_RevertToRaw(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Description = TEXT("Revert To Raw");
	TranslationCompressionFormat = ACF_None;
	RotationCompressionFormat = ACF_None;
}

void UDEPRECATED_AnimCompress_RevertToRaw::DoReduction(UAnimSequence* AnimSeq, const TArray<FBoneData>& BoneData)
{
	// Disable "Revert To RAW" to prevent uncompressed animation data.
	// We instead compress using the Bitwise compressor, with really light settings, so it acts pretty close to "no compression at all."
	UAnimCompress* BitwiseCompressor = ConstructObject<UAnimCompress_BitwiseCompressOnly>( UAnimCompress_BitwiseCompressOnly::StaticClass() );
	BitwiseCompressor->RotationCompressionFormat = ACF_Float96NoW;
	BitwiseCompressor->TranslationCompressionFormat = ACF_None;
	BitwiseCompressor->Reduce(AnimSeq, false);
}
