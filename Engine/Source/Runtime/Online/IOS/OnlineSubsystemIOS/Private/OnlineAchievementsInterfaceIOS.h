// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineAchievementsInterface.h"

/**
 *	IOnlineAchievements - Interface class for acheivements
 */
class FOnlineAchievementsIOS : public IOnlineAchievements
{
private:

	/** Reference to the main GameCenter subsystem */
	class FOnlineSubsystemIOS* IOSSubsystem;

	/** hide the default constructor, we need a reference to our OSS */
	FOnlineAchievementsIOS() {};

	// IOS only supports loading achievements for local player. This is where they are cached.
	TArray< FOnlineAchievement > Achievements;

	// Cached achievement descriptions for an Id
	TMap< FString, FOnlineAchievementDesc > AchievementDescriptions;


public:

	// Begin IOnlineAchievements interface
	virtual bool WriteAchievements( const FUniqueNetId& PlayerId, FOnlineAchievementsWriteRef& WriteObject ) OVERRIDE;
	virtual bool ReadAchievements( const FUniqueNetId& PlayerId ) OVERRIDE;
	virtual bool ReadAchievementDescriptions( const FUniqueNetId& PlayerId ) OVERRIDE;
	virtual bool GetAchievement(const FUniqueNetId& PlayerId, const FString& AchievementId, FOnlineAchievement& OutAchievement) OVERRIDE;
	virtual bool GetAchievements(const FUniqueNetId& PlayerId, TArray<FOnlineAchievement> & OutAchievements) OVERRIDE;
	virtual bool GetAchievementDescription(const FString& AchievementId, FOnlineAchievementDesc& OutAchievementDesc) OVERRIDE;
#if !UE_BUILD_SHIPPING
	virtual bool ResetAchievements( const FUniqueNetId& PlayerId ) OVERRIDE;
#endif // !UE_BUILD_SHIPPING
	// End IOnlineAchievements interface


	/**
	 * Constructor
	 *
	 * @param InSubsystem - A reference to the owning subsystem
	 */
	FOnlineAchievementsIOS( class FOnlineSubsystemIOS* InSubsystem );

	
	/**
	 * Default destructor
	 */
	virtual ~FOnlineAchievementsIOS(){}


private:

	/**
	 * Convert the name of the achievement to one which is valid with game center. I.e. grp.InAchievementName
	 *
	 * @param InAchievementName - The name we will check matches the game center requirements
	 *
	 * @return The name in a game center compatible format.
	 */
	FString NormalizeAchievementName( const FString& InAchievementName );
};

typedef TSharedPtr<FOnlineAchievementsIOS, ESPMode::ThreadSafe> FOnlineAchievementsIOSPtr;