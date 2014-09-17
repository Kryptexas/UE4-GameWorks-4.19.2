// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayPrediction.generated.h"

USTRUCT()
struct GAMEPLAYABILITIES_API FPredictionKey
{
	GENERATED_USTRUCT_BODY()

	typedef int16 KeyType;

	FPredictionKey()
	: Current(0), Base(0), PredictiveConnection(nullptr), bIsStale(false)
	{

	}

	bool IsValidKey() const
	{
		return Current > 0;
	}

	/**
	 *	Stale means that a prediction key may be valid but is not usable for further prediction actions.
	 *	E.g., we did predictive action A and are waiting for confirmation/rejection from server, we cannot
	 *	then create a new predictive action B that is associated on the same key.
	 */
	bool IsStale() const
	{
		return Current > 0 && bIsStale == true;
	}

	bool operator==(const FPredictionKey& Other) const
	{
		return Current == Other.Current && Base == Other.Base;
	}
	
	/** Construct a new prediction key with no dependancies */
	static FPredictionKey CreateNewPredictionKey();

	/** Create a new dependant prediction key: keep our existing base or use the current key as the base. */
	void GenerateDependantPredictionKey();

	UPROPERTY()
	int16	Current;

	UPROPERTY()
	int16	Base;

	UPROPERTY(NotReplicated)
	UPackageMap*	PredictiveConnection;

	UPROPERTY(NotReplicated)
	bool bIsStale;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

private:

	void GenerateNewPredictionKey();

	

	FPredictionKey(int32 Key)
		: Current(Key), Base(0), PredictiveConnection(nullptr)
	{

	}

	FPredictionKey(int16 InKey, int16 PreviousKey)
		: Current(InKey), Base(PreviousKey), PredictiveConnection(nullptr)
	{

	}
};

template<>
struct TStructOpsTypeTraits<FPredictionKey> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true,
		WithIdenticalViaEquality = true
	};
};

// -----------------------------------------------------------------


USTRUCT()
struct GAMEPLAYABILITIES_API FScopedPredictionKey
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FPredictionKey	Key;
};
