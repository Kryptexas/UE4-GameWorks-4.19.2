// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineAchievementsInterface.h"
#include "OnlineSubsystemTypes.h"
#include "AndroidRuntimeSettings.h"

/**
 *	IOnlineAchievements - Interface class for Achievements
 */
class FOnlineAchievementsGooglePlay : public IOnlineAchievements
{
private:

	/** Reference to the main GameCenter subsystem */
	class FOnlineSubsystemGooglePlay* AndroidSubsystem;
	
	/** hide the default constructor, we need a reference to our OSS */
	FOnlineAchievementsGooglePlay() : CurrentWriteObject(MakeShareable(new FOnlineAchievementsWrite()))
	{}

	// Android only supports loading achievements for local player. This is where they are cached.
	TArray< FOnlineAchievement > Achievements;

	// Cached achievement descriptions for an Id
	TMap< FString, FOnlineAchievementDesc > AchievementDescriptions;

public:

	// Begin IOnlineAchievements interface
	virtual void WriteAchievements(const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject, const FOnAchievementsWrittenDelegate& Delegate = FOnAchievementsWrittenDelegate()) OVERRIDE;
	virtual void QueryAchievements(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate = FOnQueryAchievementsCompleteDelegate()) OVERRIDE;
	virtual void QueryAchievementDescriptions(const FUniqueNetId& PlayerId, const FOnQueryAchievementsCompleteDelegate& Delegate = FOnQueryAchievementsCompleteDelegate()) OVERRIDE ;
	virtual EOnlineCachedResult::Type GetCachedAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement) OVERRIDE;
	virtual EOnlineCachedResult::Type GetCachedAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement>& OutAchievements) OVERRIDE;
	virtual EOnlineCachedResult::Type GetCachedAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc) OVERRIDE;
#if !UE_BUILD_SHIPPING
	virtual bool ResetAchievements( const FUniqueNetId& PlayerId ) OVERRIDE;
#endif // !UE_BUILD_SHIPPING
	// End IOnlineAchievements interface
	
	//platform specific members

	/** Sets unique ID of current logged-in player */
	void SetUniqueID(const FUniqueNetId& InUserID);
	
	/** Reset unique ID of current logged-in player */
	void ResetUniqueID();

	/**
	 * Constructor
	 *
	 * @param InSubsystem - A reference to the owning subsystem
	 */
	FOnlineAchievementsGooglePlay( class FOnlineSubsystemGooglePlay* InSubsystem );

	/**
	 * Default destructor
	 */
	virtual ~FOnlineAchievementsGooglePlay() {}


private:

	TArray<FGooglePlayAchievementMapping> AchievementConfigMap;

	TSharedPtr< FUniqueNetIdString > CurrentPlayerID;

	TSharedRef<FOnlineAchievementsWrite> CurrentWriteObject;
};

typedef TSharedPtr<FOnlineAchievementsGooglePlay, ESPMode::ThreadSafe> FOnlineAchievementsGooglePlayPtr;
