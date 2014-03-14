// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IFriendsAndChatModule.h: Declares the IFriendsAndChatModule interface.
=============================================================================*/

#pragma once

/**
 * Delegate type for FriendsList updated.
 */
DECLARE_MULTICAST_DELEGATE_OneParam( FOnFriendsNotification, const FFriendsAndChatMessageRef& )
DECLARE_DELEGATE_RetVal(bool, FFriendsSystemReady )

/**
 * Interface for the Friends and chat manager.
 */
class IFriendsAndChatModule
	: public IModuleInterface
{
public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IFriendsAndChatModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IFriendsAndChatModule>("FriendsAndChat");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		if (FModuleManager::Get().IsModuleLoaded("FriendsAndChat"))
		{
			return true;
		}
		return false;
	}

	/**
	 * Initialization function.
	 * @param bAllowLaunchGame - Can this app launch a game directly. CP cannot
	 */
	virtual void Init( bool bAllowLaunchGame = true ) = 0;

	/**
	 * Returns a delegate that is executed when we receive a Friends List notification.
	 * @return The delegate.
	 */
	 virtual FOnFriendsNotification& OnFriendsListNotification() = 0;

	/**
	 * Create the friends list widget.
	 * @param ParentWindow - the parent window to add the widget
	 * @param InStyle - the style to use to create the widgets
	 */
	virtual void CreateFriendsListWidget( TSharedPtr< const SWidget > ParentWidget, const struct FFriendsAndChatStyle* InStyle ) = 0;

	/**
	 * Create the a friends list widget without a container.
	 * @param InStyle - the style to use to create the widgets
	 * @return The Friends List widget
	 */
	virtual TSharedPtr< SWidget > GenerateFriendsListWidget( const struct FFriendsAndChatStyle* InStyle ) = 0;

	/**
	 * Set that we are in a session, so can send join game requests.
	 * @param bInSession - If we are in a session
	 */
	virtual void SetInSession( bool bInSession ) = 0;

	/**
	 * Callback for if a notification failed. Will resend later.
	 * @param NetID - the original caller ID
	 */
	virtual void SetUnhandledNotification( TSharedRef< FUniqueNetId > NetID ) = 0;

	/**
	 * Clear game invites.
	 */
	virtual void ClearGameInvites() = 0;

	/**
	 * Get the friends list count.
	 * @return The friends list count
	 */
	virtual int32 GetFriendCount() = 0;

	/**
	 * Log out - kill the friends list window.
	 */
	virtual void Logout() = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IFriendsAndChatModule() {}
};
