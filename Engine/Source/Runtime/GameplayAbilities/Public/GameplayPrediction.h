// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayPrediction.generated.h"

DECLARE_DELEGATE(FPredictionKeyEvent);

/**
 *	FPredictionKey is a generic way of supporting Clientside Prediction in the GameplayAbility system.
 *	A FPredictionKey is essentially an ID for indentifying predictive actions and side effects that are
 *	done on a client. UAbilitySystemComponent supports syncrhonization of the predicton key and its side effects
 *	between client and server.
 *	
 *	Essentially, anything can be associated with a PredictionKey, for example activating an Ability.
 *	The client can generates a fresh PredictionKey and sends it to the server in his ServerTryActivateAbility call.
 *	The server can confirm or reject this call (ClientActivateAbilitySucceed/Failed). 
 *	
 *	While the client is predictiving his ability, he is creating side effects (GameplayEffects, TriggeredEvents, Animations, etc).
 *	As the client predicts these side effects, he associates each one with the prediction key generated at the start of the ability
 *	activation.
 *	
 *	If the ability activation is rejected, the client can immediately revert these side effects. 
 *	If the ability activation is accepted, the client must wait until the replicated side effects are sent to the server.
 *		(the ClientActivatbleAbilitySucceed RPC will be immediately sent. Property replication may happen a few frames later).
 *		Once replication of the server created side effects is finished, the client can undo his locally predictive side effects.
 *		
 *	The main things FPredictionKey itself provides are:
 *		-Unique ID and a system for having dependant chains of Prediction Keys ("Current" and "Base" integers)
 *		-A special implementation of ::NetSerialize *** which only serializes the prediction key to the predicting client ***
 *			-This is important as it allows us to serializez prediction keys in replicated state, knowing that only clients that gave the server the prediction key will actually see them!
 *	
 */

USTRUCT()
struct GAMEPLAYABILITIES_API FPredictionKey
{
	GENERATED_USTRUCT_BODY()

	typedef int16 KeyType;

	FPredictionKey()
	: Current(0), Base(0), PredictiveConnection(nullptr), bIsStale(false)
	{

	}

	UPROPERTY()
	int16	Current;

	UPROPERTY()
	int16	Base;

	UPROPERTY(NotReplicated)
	UPackageMap*	PredictiveConnection;

	UPROPERTY(NotReplicated)
	bool bIsStale;

	/** Construct a new prediction key with no dependancies */
	static FPredictionKey CreateNewPredictionKey();

	/** Create a new dependant prediction key: keep our existing base or use the current key as the base. */
	void GenerateDependantPredictionKey();

	/** Creates new delegate called only when this key is rejected. */
	FPredictionKeyEvent& NewRejectedDelegate();

	/** Creates new delegate called only when replicated state catches up to this key. */
	FPredictionKeyEvent& NewCaughtUpDelegate();
	
	/** Add a new delegate that is called if the key is rejected or caught up to. */
	void NewRejectOrCaughtUpDelegate(FPredictionKeyEvent Event);
	
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	bool IsValidKey() const
	{
		return Current > 0;
	}

	/** Can this key be used for more predictive actions, or has it already been sent off to the server? */
	bool IsValidForMorePrediction() const
	{
		return Current > 0 && bIsStale == false;
	}

	/** Was this PredictoinKey received from a NetSerialize or created locallay? */
	bool WasReceived() const
	{
		return PredictiveConnection != nullptr;
	}

	bool DependsOn(KeyType Key)
	{
		return (Current == Key || Base == Key);
	}

	bool operator==(const FPredictionKey& Other) const
	{
		return Current == Other.Current && Base == Other.Base;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("[%d/%d]"), Current, Base);
	}

	friend uint32 GetTypeHash(const FPredictionKey& InKey)
	{
		return ((InKey.Current << 16) | InKey.Base);
	}

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

/**
 *	This is a data structure for registering delegates associated with prediction key rejection and replicated state 'catching up'.
 *	Delegates should be registered that revert side effects created with prediction keys.
 *	
 */

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
	static void NewRejectOrCaughtUpDelegate(FPredictionKey::KeyType Key, FPredictionKeyEvent NewEvent);

	static void	BroadcastRejectedDelegate(FPredictionKey::KeyType Key);
	static void	BroadcastCaughtUpDelegate(FPredictionKey::KeyType Key);

	static void Reject(FPredictionKey::KeyType Key);
	static void CatchUpTo(FPredictionKey::KeyType Key);

	static void AddDependancy(FPredictionKey::KeyType ThisKey, FPredictionKey::KeyType DependsOn);

private:

	static void CaughtUp(FPredictionKey::KeyType Key);
};
