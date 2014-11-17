// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineExternalUIInterface.h"
#include "OnlineSubsystemIOSTypes.h"

class FOnlineExternalUIIOS : public IOnlineExternalUI
{
PACKAGE_SCOPE:

	FOnlineExternalUIIOS();

public:

	// Begin IOnlineExternalUI interface
	virtual bool ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, const FOnLoginUIClosedDelegate& Delegate) OVERRIDE;
	virtual bool ShowFriendsUI(int32 LocalUserNum) OVERRIDE;
	virtual bool ShowInviteUI(int32 LocalUserNum) OVERRIDE;
	virtual bool ShowAchievementsUI(int32 LocalUserNum) OVERRIDE;
	virtual bool ShowLeaderboardUI(const FString& LeaderboardName) OVERRIDE;
	virtual bool ShowWebURL(const FString& WebURL) OVERRIDE;
	virtual bool ShowProfileUI(const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate) OVERRIDE;
	// End IOnlineExternalUI interface
};

typedef TSharedPtr<FOnlineExternalUIIOS, ESPMode::ThreadSafe> FOnlineExternalUIIOSPtr;