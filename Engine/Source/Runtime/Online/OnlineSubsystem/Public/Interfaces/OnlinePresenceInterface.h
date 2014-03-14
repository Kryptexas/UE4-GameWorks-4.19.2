// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineDelegateMacros.h"

/** Type of presence keys */
typedef FString FPresenceKey;

/** Type of presence properties - a key/value map */
typedef FOnlineKeyValuePairs<FPresenceKey, FVariantData> FPresenceProperties;

/** The default key that will update presence text in the platform's UI */
const FString DefaultPresenceKey = "RichPresence";

/**
 * Presence info for an online user returned via IOnlinePresence interface
 */
class FOnlineUserPresence
{
public:
	uint64 SessionId;
	FString PresenceStr;
	uint32 bIsOnline:1;
	uint32 bIsPlaying:1;
	uint32 bIsPlayingThisGame:1;
	uint32 bIsJoinable:1;
	uint32 bHasVoiceSupport:1;

	FPresenceProperties Presence;

	/** Constructor */
	FOnlineUserPresence()
		: SessionId(0)
		, bIsOnline(0)
		, bIsPlaying(0)
		, bIsPlayingThisGame(0)
		, bIsJoinable(0)
		, bHasVoiceSupport(0)
	{

	}
};

/**
 *	Interface class for getting and setting rich presence information.
 */
class IOnlinePresence
{
public:
	/**
	 * Delegate executed when setting presence for a user has completed.
	 *
	 * @param User The unique id of the user whose presence was set.
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error.
	 */
	DECLARE_DELEGATE_TwoParams(FOnSetCompleteDelegate, const class FUniqueNetId&, const bool);

	/**
	 * Delegate executed when the presence query request has completed.
	 *
	 * @param Users The unique ids of the users whose presence was requested.
	 * @param bWasSuccessful true if the async action completed without error, false if there was an error.
	 */
	DECLARE_DELEGATE_TwoParams(FOnQueryCompleteDelegate, const TArray<TSharedRef<FUniqueNetId>>&, const bool);

	/**
	 * Starts an async task that sets presence information for the user.
	 *
	 * @param User The unique id of the user whose presence is being set.
	 * @param Presence The collection of key/value pairs to set as the user's presence data.
	 * @param Delegate The delegate to be executed when the potentially asynchronous set operation completes.
	 */
	virtual void SetPresence(const FUniqueNetId& User, const FPresenceProperties& Presence, const FOnSetCompleteDelegate& Delegate = FOnSetCompleteDelegate()) = 0;

	/**
	 * Starts an async operation that will update the cache with presence data from all users in the Users array.
	 * On platforms that support multiple keys, this function will query all keys.
	 *
	 * @param Users The list of unique ids of the users to query for presence information.
	 * @param Delegate The delegate to be executed when the potentially asynchronous query operation completes.
	 */
	virtual void QueryPresence(const TArray<TSharedRef<FUniqueNetId>>& Users, const FOnQueryCompleteDelegate& Delegate = FOnQueryCompleteDelegate()) = 0;

	/**
	 * Gets the cached presence information for a user.
	 *
	 * @param User The unique id of the user from which to get presence information.
	 * @param OutPresence If found, a shared pointer to the cached presence data for User will be stored here.
	 * @return Whether the data was found or not.
	 */
	virtual EOnlineCachedResult::Type GetCachedPresence(const FUniqueNetId& User, TSharedPtr<FOnlineUserPresence>& OutPresence) = 0;
};
