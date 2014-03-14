// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimCompress_LeastDestructive.cpp: Uses the Bitwise compressor with really light settings
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AnimationUtils.h"
#include "AnimEncoding.h"

UAnimCompress_LeastDestructive::UAnimCompress_LeastDestructive(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Description = TEXT("Least Destructive");
	TranslationCompressionFormat = ACF_None;
	RotationCompressionFormat = ACF_None;
}


void UAnimCompress_LeastDestructive::DoReduction(UAnimSequence* AnimSeq, const TArray<FBoneData>& BoneData)
{
	UAnimCompress* BitwiseCompressor = ConstructObject<UAnimCompress_BitwiseCompressOnly>( UAnimCompress_BitwiseCompressOnly::StaticClass() );
	BitwiseCompressor->RotationCompressionFormat = ACF_Float96NoW;
	BitwiseCompressor->TranslationCompressionFormat = ACF_None;
	BitwiseCompressor->Reduce(AnimSeq, false);
}
