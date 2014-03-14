// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineDelegateMacros.h"

/**
 * Delegate called when the external UI is opened or closed
 *
 * @param bIsOpening state of the external UI
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnExternalUIChange, bool);
typedef FOnExternalUIChange::FDelegate FOnExternalUIChangeDelegate;

/**
 * Delegate called when the external login UI was closed,
 * and the player did not log in.
 *
 * @param ControllerIndex The controller index of the player who initiated the login UI.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoginUIClosedWithoutUser, const int);
typedef FOnLoginUIClosedWithoutUser::FDelegate FOnLoginUIClosedWithoutUserDelegate;

/**
 * Delegate called when the external login UI was closed,
 * and the player did log in.
 *
 * @param User The unique Id of the player who logged in using the UI.
 * @param ControllerIndex The controller index of the player who logged in using the UI.
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLoginUIClosedWithUser, const class FUniqueNetId&, const int);
typedef FOnLoginUIClosedWithUser::FDelegate FOnLoginUIClosedWithUserDelegate;

/** 
 * Interface definition for the online services external UIs
 * Any online service that provides extra UI overlays will implement the relevant functions
 */
class IOnlineExternalUI
{
protected:
	IOnlineExternalUI() {};

public:
	virtual ~IOnlineExternalUI() {};

	/**
	 * Displays the UI that prompts the user for their login credentials. Each
	 * platform handles the authentication of the user's data.
	 *
	 * @param bShowOnlineOnly whether to only display online enabled profiles or not
	 * @param ControllerIndex The controller that prompted showing the login UI. If the platform supports it,
	 * it will pair the signed-in user with this controller.
 	 *
	 * @return true if it was able to show the UI, false if it failed
	 */
	virtual bool ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly = false) = 0;

	/**
	 * Displays the UI that shows a user's list of friends
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return true if it was able to show the UI, false if it failed
	 */
	virtual bool ShowFriendsUI(int32 LocalUserNum) = 0;

	/**
	 *	Displays the UI that shows a user's list of friends to invite
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return true if it was able to show the UI, false if it failed
	 */
	virtual bool ShowInviteUI(int32 LocalUserNum) = 0;

	/**
	 *	Displays the UI that shows a user's list of achievements
	 *
	 * @param LocalUserNum the controller number of the associated user
	 *
	 * @return true if it was able to show the UI, false if it failed
	 */
	virtual bool ShowAchievementsUI(int32 LocalUserNum) = 0;

	/**
	 *	Displays a web page in the external UI
	 *
	 * @param WebURL fully formed web address (http://www.google.com)
	 *
	 * @return true if it was able to show the UI, false if it failed
	 */
	virtual bool ShowWebURL(const FString& WebURL) = 0;

	/**
	 * Delegate called when the external UI is opened or closed
	 *
	 * @param bIsOpening state of the external UI
	 */
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnExternalUIChange, bool);

	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnLoginUIClosedWithoutUser, const int);
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnLoginUIClosedWithUser, const class FUniqueNetId&, const int);
};

typedef TSharedPtr<IOnlineExternalUI, ESPMode::ThreadSafe> IOnlineExternalUIPtr;

