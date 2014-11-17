// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "AndroidRuntimeSettings.h"
#include "CoreUObject.h"

FOnlineExternalUIGooglePlay::FOnlineExternalUIGooglePlay() 
{

}

bool FOnlineExternalUIGooglePlay::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, const FOnLoginUIClosedDelegate& Delegate)
{
	return false;
}

bool FOnlineExternalUIGooglePlay::ShowFriendsUI(int32 LocalUserNum)
{
	return false;
}

bool FOnlineExternalUIGooglePlay::ShowInviteUI(int32 LocalUserNum) 
{
	return false;
}

bool FOnlineExternalUIGooglePlay::ShowAchievementsUI(int32 LocalUserNum) 
{
	// Will always show the achievements UI for the current local signed-in user
	extern void AndroidThunkCpp_ShowAchievements();
	AndroidThunkCpp_ShowAchievements();
	return true;
}

bool FOnlineExternalUIGooglePlay::ShowLeaderboardUI(const FString& LeaderboardName)
{
	auto DefaultSettings = GetDefault<UAndroidRuntimeSettings>();

	for(const auto& Mapping : DefaultSettings->LeaderboardMap)
	{
		if(Mapping.Name == LeaderboardName)
		{
			extern void AndroidThunkCpp_ShowLeaderboard(const FString&);
			AndroidThunkCpp_ShowLeaderboard(Mapping.LeaderboardID);
			return true;
		}
	}
	
	return false;
}

bool FOnlineExternalUIGooglePlay::ShowWebURL(const FString& WebURL) 
{
	return false;
}

bool FOnlineExternalUIGooglePlay::ShowProfileUI( const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate )
{
	return false;
}
