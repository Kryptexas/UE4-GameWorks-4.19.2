// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineExternalUIInterface.h"
#include "OnlineSubsystemTypes.h"
#include "OnlineAsyncTaskManagerSteam.h"
#include "OnlineSubsystemSteamPackage.h"

/**
 *	Async event that notifies when the STEAM external UI has been activated
 */
class FOnlineAsyncEventSteamExternalUITriggered : public FOnlineAsyncEvent<FOnlineSubsystemSteam>
{
	/** Is the External UI activating */
	bool bIsActive;

	/** Hidden on purpose */
	FOnlineAsyncEventSteamExternalUITriggered() :
		FOnlineAsyncEvent(NULL),
		bIsActive(false)
	{
	}

public:

	FOnlineAsyncEventSteamExternalUITriggered(FOnlineSubsystemSteam* InSteamSubsystem, bool bInIsActive) :
		FOnlineAsyncEvent(InSteamSubsystem),
		bIsActive(bInIsActive)
	{
	}

	virtual ~FOnlineAsyncEventSteamExternalUITriggered()
	{
	}

	/**
	 *	Get a human readable description of task
	 */
	virtual FString ToString() const OVERRIDE;

	/**
	 *	Async task is given a chance to trigger it's delegates
	 */
	virtual void TriggerDelegates() OVERRIDE;
};

/** 
 * Implementation for the STEAM external UIs
 */
class FOnlineExternalUISteam : public IOnlineExternalUI
{
PACKAGE_SCOPE:

	/** Hidden on purpose */
	FOnlineExternalUISteam()
	{
	}

public:

	virtual ~FOnlineExternalUISteam()
	{
	}

	// IOnlineExternalUI
	virtual bool ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly = false) OVERRIDE;
	virtual bool ShowFriendsUI(int32 LocalUserNum) OVERRIDE;
	virtual bool ShowInviteUI(int32 LocalUserNum) OVERRIDE;
	virtual bool ShowAchievementsUI(int32 LocalUserNum) OVERRIDE;
	virtual bool ShowWebURL(const FString& WebURL) OVERRIDE;
};

typedef TSharedPtr<FOnlineExternalUISteam, ESPMode::ThreadSafe> FOnlineExternalUISteamPtr;

