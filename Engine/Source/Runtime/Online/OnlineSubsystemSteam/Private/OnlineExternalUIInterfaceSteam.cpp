// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemSteamPrivatePCH.h"
#include "OnlineExternalUIInterfaceSteam.h"
#include "OnlineSessionInterfaceSteam.h"
#include "OnlineSubsystemSteam.h"

// Other external UI possibilities in Steam
// "Players" - recently played with players
// "Community" 
// "Settings"
// "OfficialGameGroup"
// "Stats"

FString FOnlineAsyncEventSteamExternalUITriggered::ToString() const
{
	return FString::Printf(TEXT("FOnlineAsyncEventSteamExternalUITriggered bIsActive: %d"), bIsActive);
}

void FOnlineAsyncEventSteamExternalUITriggered::TriggerDelegates()
{
	FOnlineAsyncEvent::TriggerDelegates();
	IOnlineExternalUIPtr ExternalUIInterface = Subsystem->GetExternalUIInterface();
	ExternalUIInterface->TriggerOnExternalUIChangeDelegates(bIsActive);
}

bool FOnlineExternalUISteam::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly)
{
	return false;
}

bool FOnlineExternalUISteam::ShowFriendsUI(int32 LocalUserNum)
{
	SteamFriends()->ActivateGameOverlay("Friends");
	return true;
}

bool FOnlineExternalUISteam::ShowInviteUI(int32 LocalUserNum)
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
	if (Sessions.IsValid() && Sessions->HasPresenceSession())
	{
		SteamFriends()->ActivateGameOverlay("LobbyInvite");
		return true;
	}

	return false;
}

bool FOnlineExternalUISteam::ShowAchievementsUI(int32 LocalUserNum)
{
	SteamFriends()->ActivateGameOverlay("Achievements");
	return true;
}

bool FOnlineExternalUISteam::ShowWebURL(const FString& WebURL)
{
	if (!WebURL.StartsWith(TEXT("https://")))
	{
		SteamFriends()->ActivateGameOverlayToWebPage(TCHAR_TO_UTF8(*FString::Printf(TEXT("https://%s"), *WebURL)));
	}
	else
	{
		SteamFriends()->ActivateGameOverlayToWebPage(TCHAR_TO_UTF8(*WebURL));
	}
	
	return true;
}





