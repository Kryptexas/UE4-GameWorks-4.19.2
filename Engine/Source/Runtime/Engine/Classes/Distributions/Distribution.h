// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Distribution.generated.h"

/** Lookup table for distributions. */
#if !CPP      //noexport struct
USTRUCT(noexport)
struct FDistributionLookupTable
{
	UPROPERTY()
	uint8 Op;

	UPROPERTY()
	uint8 EntryCount;

	UPROPERTY()
	uint8 EntryStride;

	UPROPERTY()
	uint8 SubEntryStride;

	UPROPERTY()
	float TimeScale;

	UPROPERTY()
	float TimeBias;

	UPROPERTY()
	TArray<float> Values;

	UPROPERTY()
	uint8 LockFlag;
};
#endif

// Base class for raw (baked out) Distribution type
#if !CPP      //noexport struct
USTRUCT(noexport)
struct FRawDistribution
{
	UPROPERTY()
	FDistributionLookupTable Table;
};
#endif

UCLASS(DefaultToInstanced, collapsecategories, hidecategories=Object, editinlinenew, abstract,MinimalAPI)
class UDistribution : public UObject, public FCurveEdInterface
{
	GENERATED_UCLASS_BODY()

};

