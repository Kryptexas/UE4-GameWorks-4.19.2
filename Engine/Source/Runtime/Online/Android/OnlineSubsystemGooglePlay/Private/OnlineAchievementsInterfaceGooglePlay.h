// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineAchievementsInterface.h"
#include "OnlineSubsystemTypes.h"
#include "AndroidRuntimeSettings.h"
#include <jni.h>

extern "C" void Java_com_epicgames_ue4_GameActivity_nativeUpdateAchievements(JNIEnv* LocalJNIEnv, jobject LocalThiz, jobjectArray Achievements);

/**
 *	IOnlineAchievements - Interface class for Achievements
 */
class FOnlineAchievementsGooglePlay : public IOnlineAchievements
{
private:

	/** Reference to the main GameCenter subsystem */
	class FOnlineSubsystemGooglePlay* AndroidSubsystem;
	
	/** hide the default constructor, we need a reference to our OSS */
	FOnlineAchievementsGooglePlay() {}

	/** Android only supports loading achievements for local player. This is where they are cached. */
	TArray< FOnlineAchievement > Achievements;

	/** Cached achievement descriptions for an Id */
	TMap< FString, FOnlineAchievementDesc > AchievementDescriptions;

	/**
	 * Hack to store context needed by the QueryAchievement callback to avoid round-tripping
	 * this data through JNI and back.
	 */
	struct FPendingAchievementQuery
	{
		FOnlineAchievementsGooglePlay* AchievementsInterface;
		FOnQueryAchievementsCompleteDelegate Delegate;
		FUniqueNetIdString PlayerID;
		bool IsQueryPending;
	};

	/** Instance of the query context data */
	static FPendingAchievementQuery PendingAchievementQuery;

	/**
	 * Native function called from Java when we get the achievement load result callback.
	 * Converts the Google Play achievement IDs to the names specified by the game's mapping.
	 *
	 * @param LocalJNIEnv the current JNI environment
	 * @param LocalThiz instance of the Java GameActivity class
	 * @param Achievements an array of JavaAchievement objects holding the data just queried from the backend
	 */
	friend void Java_com_epicgames_ue4_GameActivity_nativeUpdateAchievements(JNIEnv* LocalJNIEnv, jobject LocalThiz, jobjectArray Achievements);

public:

	// Begin IOnlineAchievements interface
	virtual void WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate = FOnAchievementsWrittenDelegate()) override;
	virtual void QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate = FOnQueryAchievementsCompleteDelegate()) override;
	virtual void QueryAchievementDescriptions(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate = FOnQueryAchievementsCompleteDelegate()) override ;
	virtual EOnlineCachedResult::Type GetCachedAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement) override;
	virtual EOnlineCachedResult::Type GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements) override;
	virtual EOnlineCachedResult::Type GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc) override;
#if !UE_BUILD_SHIPPING
	virtual bool ResetAchievements( const FUniqueNetId& PlayerId ) override;
#endif // !UE_BUILD_SHIPPING
	// End IOnlineAchievements interface

	/**
	 * Constructor
	 *
	 * @param InSubsystem - A reference to the owning subsystem
	 */
	FOnlineAchievementsGooglePlay( class FOnlineSubsystemGooglePlay* InSubsystem );
};

typedef TSharedPtr<FOnlineAchievementsGooglePlay, ESPMode::ThreadSafe> FOnlineAchievementsGooglePlayPtr;
