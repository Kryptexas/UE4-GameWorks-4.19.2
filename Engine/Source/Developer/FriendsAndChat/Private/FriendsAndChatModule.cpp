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
	virtual void Init( bool bAllowJoinGame ) override
	{
		FFriendsAndChatManager::Get()->Init( FriendsListNotificationDelegate );
		FFriendsMessageManager::Get()->Init( FriendsListNotificationDelegate, bAllowJoinGame );
	}

	virtual void SetUnhandledNotification( TSharedRef< FUniqueNetId > NetID ) override
	{
		FFriendsMessageManager::Get()->SetUnhandledNotification( NetID );
	}

	virtual FOnFriendsNotification& OnFriendsListNotification() override
	{
		return FriendsListNotificationDelegate;
	}
	
	virtual void CreateFriendsListWidget( TSharedPtr<const SWidget> ParentWidget, const FFriendsAndChatStyle* InStyle ) override
	{
		FFriendsAndChatManager::Get()->GenerateFriendsWindow( ParentWidget, InStyle );
	}

	virtual TSharedPtr< SWidget > GenerateFriendsListWidget( const struct FFriendsAndChatStyle* InStyle ) override
	{
		return FFriendsAndChatManager::Get()->GenerateFriendsListWidget( InStyle );
	}

	virtual void SetInSession( bool bInSession ) override
	{
		FFriendsAndChatManager::Get()->SetInSession( bInSession );
	}

	virtual void ClearGameInvites() override
	{
		FFriendsMessageManager::Get()->ClearGameInvites();
	}

	virtual int32 GetFriendCount() override
	{
		return FFriendsAndChatManager::Get()->GetFriendCount();
	}

	virtual void Logout() override
	{
		FFriendsAndChatManager::Get()->Logout();
		FFriendsMessageManager::Get()->Logout();
	}

	// End IFriendsAndChatModule interface

public:

	// Begin IModuleInterface interface
	virtual void StartupModule() override
	{
		// Make sure the singletons are created
 		FFriendsAndChatManager::Get();
	}

	virtual void ShutdownModule() override
	{
		FFriendsAndChatManager::Shutdown();
	}
	// End IModuleInterface interface

private:
	// Holds the Notification delegate
	FOnFriendsNotification FriendsListNotificationDelegate;
};

IMPLEMENT_MODULE( FFriendsAndChatModule, FriendsAndChat );
