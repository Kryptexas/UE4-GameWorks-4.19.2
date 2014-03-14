// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineLeaderboardInterface.h"
#include "OnlineSubsystemIOSTypes.h"

class FOnlineLeaderboardsIOS : public IOnlineLeaderboards
{
private:
	class FOnlineIdentityIOS* IdentityInterface;

	class FOnlineFriendsIOS* FriendsInterface;

	NSMutableArray* UnreportedScores;

PACKAGE_SCOPE:

	FOnlineLeaderboardsIOS(FOnlineSubsystemIOS* InSubsystem);

public:

	virtual ~FOnlineLeaderboardsIOS() {};

	// Begin IOnlineLeaderboards interface
	virtual bool ReadLeaderboards(const TArray< TSharedRef<FUniqueNetId> >& Players, FOnlineLeaderboardReadRef& ReadObject) OVERRIDE;
	virtual bool ReadLeaderboardsForFriends(int32 LocalUserNum, FOnlineLeaderboardReadRef& ReadObject) OVERRIDE;
	virtual void FreeStats(FOnlineLeaderboardRead& ReadObject) OVERRIDE;
	virtual bool WriteLeaderboards(const FName& SessionName, const FUniqueNetId& Player, FOnlineLeaderboardWrite& WriteObject) OVERRIDE;
	virtual bool FlushLeaderboards(const FName& SessionName) OVERRIDE;
	virtual bool WriteOnlinePlayerRatings(const FName& SessionName, int32 LeaderboardId, const TArray<FOnlinePlayerScore>& PlayerScores) OVERRIDE;
	// End IOnlineLeaderboards interface

};

typedef TSharedPtr<FOnlineLeaderboardsIOS, ESPMode::ThreadSafe> FOnlineLeaderboardsIOSPtr;