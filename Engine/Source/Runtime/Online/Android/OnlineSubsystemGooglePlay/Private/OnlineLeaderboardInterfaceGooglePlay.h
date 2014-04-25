// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineLeaderboardInterface.h"
#include "AndroidRuntimeSettings.h"

struct FOnlinePendingLeaderboardWrite
{
	FString LeaderboardName;
	uint64 Score;
};

/**
 * Interface definition for the online services leaderboard services 
 */
class FOnlineLeaderboardsGooglePlay : public IOnlineLeaderboards
{
public:
	FOnlineLeaderboardsGooglePlay(class FOnlineSubsystemGooglePlay* InSubsystem);

	virtual bool ReadLeaderboards(const TArray< TSharedRef<FUniqueNetId> >& Players, FOnlineLeaderboardReadRef& ReadObject) OVERRIDE;
	virtual bool ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject) OVERRIDE;
	virtual void FreeStats(FOnlineLeaderboardRead& ReadObject) OVERRIDE;
	virtual bool WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject) OVERRIDE;
	virtual bool FlushLeaderboards(const FName& SessionName) OVERRIDE;
	virtual bool WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores) OVERRIDE;

private:
	/**
	 * Helper function to get the platform- and game-specific leaderboard ID from the JSON config file.
	 *
	 * @param LeaderboardName the cross-platform name of the leaderboard to look up
	 * @return The unique ID for the leaderboard as specified in the config file.
	*/
	FString GetLeaderboardID(const FString& LeaderboardName);

	/** Scores are cached here in WriteLeaderboards until FlushLeaderboards is called */
	TArray<FOnlinePendingLeaderboardWrite> UnreportedScores;
};

typedef TSharedPtr<FOnlineLeaderboardsGooglePlay, ESPMode::ThreadSafe> FOnlineLeaderboardsGooglePlayPtr;
