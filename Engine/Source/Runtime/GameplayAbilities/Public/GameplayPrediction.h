// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayPrediction.generated.h"

DECLARE_DELEGATE(FPredictionKeyEvent);

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
	bool IsValidForMorePrediction() const
	{
		return Current > 0 && bIsStale == false;
	}

	bool WasReceived() const
	{
		return PredictiveConnection != nullptr;
	}

	bool operator==(const FPredictionKey& Other) const
	{
		return Current == Other.Current && Base == Other.Base;
	}

	bool DependsOn(KeyType Key)
	{
		return (Current == Key || Base == Key);
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("[%d/%d]"), Current, Base);
	}

	friend uint32 GetTypeHash(const FPredictionKey& InKey)
	{
		return ((InKey.Current << 16) | InKey.Base);
	}
	
	/** Construct a new prediction key with no dependancies */
	static FPredictionKey CreateNewPredictionKey();

	/** Create a new dependant prediction key: keep our existing base or use the current key as the base. */
	void GenerateDependantPredictionKey();

	FPredictionKeyEvent& NewRejectedDelegate();
	FPredictionKeyEvent& NewCaughtUpDelegate();

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


struct FPredictionKeyDelegates
{

public:

	struct FDelegates
	{
	public:

		/** This delegate is called if the prediction key is associated with an action that is explicitly rejected by the server. */
		TArray<FPredictionKeyEvent>	RejectedDelegates;

		/** This delegate is called when replicated state has caught up with the prediction key. Doesnt imply rejection or acceptance. */
		TArray<FPredictionKeyEvent>	CaughtUpDelegates;
	};

	TMap<FPredictionKey::KeyType, FDelegates>	DelegateMap;

	static FPredictionKeyDelegates& Get();

	static FPredictionKeyEvent&	NewRejectedDelegate(FPredictionKey::KeyType Key);
	static FPredictionKeyEvent&	NewCaughtUpDelegate(FPredictionKey::KeyType Key);

	static void	BroadcastRejectedDelegate(FPredictionKey::KeyType Key);
	static void	BroadcastCaughtUpDelegate(FPredictionKey::KeyType Key);

	static void Reject(FPredictionKey::KeyType Key);
	static void CatchUpTo(FPredictionKey::KeyType Key);

	static void AddDependancy(FPredictionKey::KeyType ThisKey, FPredictionKey::KeyType DependsOn);

private:

	static void CaughtUp(FPredictionKey::KeyType Key);
};
