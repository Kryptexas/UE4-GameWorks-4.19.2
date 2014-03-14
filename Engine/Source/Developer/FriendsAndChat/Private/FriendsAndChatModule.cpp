// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	FriendsAndChatModule.cpp: Implements the FFriendsAndChatModule.
=============================================================================*/

#include "FriendsAndChatPrivatePCH.h"

/**
 * Implements the FriendsAndChat module.
 */
class FFriendsAndChatModule
	: public IFriendsAndChatModule
{
public:

	// Begin IFriendsAndChatModule interface
	virtual void Init( bool bAllowJoinGame ) OVERRIDE
	{
		FFriendsAndChatManager::Get()->Init( FriendsListNotificationDelegate );
		FFriendsMessageManager::Get()->Init( FriendsListNotificationDelegate, bAllowJoinGame );
	}

	virtual void SetUnhandledNotification( TSharedRef< FUniqueNetId > NetID ) OVERRIDE
	{
		FFriendsMessageManager::Get()->SetUnhandledNotification( NetID );
	}

	virtual FOnFriendsNotification& OnFriendsListNotification() OVERRIDE
	{
		return FriendsListNotificationDelegate;
	}
	
	virtual void CreateFriendsListWidget( TSharedPtr<const SWidget> ParentWidget, const FFriendsAndChatStyle* InStyle ) OVERRIDE
	{
		FFriendsAndChatManager::Get()->GenerateFriendsWindow( ParentWidget, InStyle );
	}

	virtual TSharedPtr< SWidget > GenerateFriendsListWidget( const struct FFriendsAndChatStyle* InStyle ) OVERRIDE
	{
		return FFriendsAndChatManager::Get()->GenerateFriendsListWidget( InStyle );
	}

	virtual void SetInSession( bool bInSession ) OVERRIDE
	{
		FFriendsAndChatManager::Get()->SetInSession( bInSession );
	}

	virtual void ClearGameInvites() OVERRIDE
	{
		FFriendsMessageManager::Get()->ClearGameInvites();
	}

	virtual int32 GetFriendCount() OVERRIDE
	{
		return FFriendsAndChatManager::Get()->GetFriendCount();
	}

	virtual void Logout() OVERRIDE
	{
		FFriendsAndChatManager::Get()->Logout();
		FFriendsMessageManager::Get()->Logout();
	}

	// End IFriendsAndChatModule interface

public:

	// Begin IModuleInterface interface
	virtual void StartupModule() OVERRIDE
	{
		// Make sure the singletons are created
 		FFriendsAndChatManager::Get();
	}

	virtual void ShutdownModule() OVERRIDE
	{
		FFriendsAndChatManager::Shutdown();
	}
	// End IModuleInterface interface

private:
	// Holds the Notification delegate
	FOnFriendsNotification FriendsListNotificationDelegate;
};

IMPLEMENT_MODULE( FFriendsAndChatModule, FriendsAndChat );
