// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneAnimTypeID.h"

namespace Lex
{
	FString ToString(const FMovieSceneAnimTypeID& AnimTypeID)
	{
		return FString::Printf(TEXT("%#010x"), AnimTypeID.ID);
	}
}

uint64 FMovieSceneAnimTypeID::Initialize(uint64* StaticPtr, uint32 Seed)
{
	const uint64 NewHash = GenerateHash(StaticPtr, Seed);
	// Store the cached ID in the static itself
	*StaticPtr = NewHash;
	return NewHash;
}

uint64 FMovieSceneAnimTypeID::GenerateHash(void* StaticPtr, uint32 Seed)
{
	// The bitvalue of the address with the upper 32 bits hashed with the seed
	uint64 Address = (uint64)StaticPtr;
	return (uint64(HashCombine(Address >> 32, Seed)) << 32) | (Address & 0xFFFFFFFF);
}

namespace
{
	static uint64 RunningCount = 0;
	struct FUniqueTypeID : FMovieSceneAnimTypeID
	{
		FUniqueTypeID()
		{
			ID = GenerateHash(&RunningCount, ++RunningCount);
		}
	};
}

FMovieSceneAnimTypeID FMovieSceneAnimTypeID::Unique()
{
	return FUniqueTypeID();
}
