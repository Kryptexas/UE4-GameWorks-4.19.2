// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FriendsAndChatMessage.h"
#include "IUserInfo.h"

enum class EFriendActionLevel
{
	Action,			// A normal action
	Emphasis,		// An important action to be shown with emphasis
	Critical,		// A critical action
};

// Enum holding friend action types
namespace EFriendActionType
{
	enum Type : uint8
	{
		AcceptFriendRequest,		// Accept an incoming Friend request
		IgnoreFriendRequest,		// Ignore an incoming Friend request
		RejectFriendRequest,		// Reject an incoming Friend request
		CancelFriendRequest,		// Cancel a friend request
		BlockFriend,				// Block a friend
		RemoveFriend,				// Remove a friend
		JoinGame,					// Join a Friends game
		RejectGame,					// Reject a game request
		InviteToGame,				// Invite to game
		Whisper,					// Open Whisper window
		SendFriendRequest,			// Send a friend request
		Updating,					// Updating;
		Chat,						// Chat
		MAX_None,
	};

	inline FText ToText(EFriendActionType::Type State)
	{
		switch (State)
		{
			case AcceptFriendRequest: return NSLOCTEXT("FriendsList","AcceptFriendRequest", "Accept");
			case IgnoreFriendRequest: return NSLOCTEXT("FriendsList","IgnoreFriendRequest", "Ignore");
			case RejectFriendRequest: return NSLOCTEXT("FriendsList","RejectFriendRequest", "Reject");
			case CancelFriendRequest: return NSLOCTEXT("FriendsList","CancelRequest", "Cancel");
			case BlockFriend: return NSLOCTEXT("FriendsList","BlockFriend", "Block");
			case RemoveFriend: return NSLOCTEXT("FriendsList","RemoveFriend", "Remove");
			case JoinGame: return NSLOCTEXT("FriendsList","JoingGame", "Join Game");
			case RejectGame: return NSLOCTEXT("FriendsList","RejectGame", "Reject");
			case InviteToGame: return NSLOCTEXT("FriendsList","Invite to game", "Invite to Game");
			case SendFriendRequest: return NSLOCTEXT("FriendsList","SendFriendRequst", "Send Friend Request");
			case Updating: return NSLOCTEXT("FriendsList","Updating", "Updating");
			case Chat: return NSLOCTEXT("FriendsList", "Chat", "Chat");
			case Whisper: return NSLOCTEXT("FriendsList", "Whisper", "Whisper");

			default: return FText::GetEmpty();
		}
	}

	inline EFriendActionLevel ToActionLevel(EFriendActionType::Type State)
	{
		switch (State)
		{
			case AcceptFriendRequest:
			case JoinGame:
			case SendFriendRequest:
				return EFriendActionLevel::Emphasis;
			case BlockFriend:
			case RemoveFriend:
				return EFriendActionLevel::Critical;
			default:
				return EFriendActionLevel::Action;
		}
	}
};

/**
 * Class containing the friend information - used to build the list view.
 */
class IFriendItem
	: public IUserInfo, public TSharedFromThis<IFriendItem>
{
public:

	/**
	 * Get the on-line user associated with this account.
	 *
	 * @return The online user.
	 * @see GetOnlineFriend
	 */
	virtual const TSharedPtr< FOnlineUser > GetOnlineUser() const = 0;

	/**
	 * Get the cached on-line Friend.
	 *
	 * @return The online friend.
	 * @see GetOnlineUser, SetOnlineFriend
	 */
	virtual const TSharedPtr< FOnlineFriend > GetOnlineFriend() const = 0;

	/**
	 * Get the user location.
	 * @return The user location.
	 */
	virtual const FText GetFriendLocation() const = 0;

	/**
	* Get the client name the user is logged in on
	* @return The client name
	*/
	virtual const FString GetClientName() const = 0;

	/**
	 * Get if the user is online.
	 * @return The user online state.
	 */
	virtual const bool IsOnline() const = 0;

	/**
	 * Get if the user is online and his game is joinable
	 * @return The user joinable game state.
	 */
	virtual bool IsGameJoinable() const = 0;

	/**
	 * Get if the user can join our game if we were to invite them
	 * @return True if we can invite them
	 */
	virtual bool CanInvite() const = 0;

	/**
	 * Get game session id that this friend is currently in
	 * @return The id of the game session
	 */
	virtual TSharedPtr<const FUniqueNetId> GetGameSessionId() const = 0;

	/**
	 * Obtain info needed to join a party for this friend item
	 * @return party info if available or null
	 */
	virtual TSharedPtr<IOnlinePartyJoinInfo> GetPartyJoinInfo() const = 0;

	/**
	 * Get the Unique ID.
	 * @return The Unique Net ID.
	 */
	virtual const TSharedRef<const FUniqueNetId > GetUniqueID() const = 0;

	/**
	 * Is this friend in the default list.
	 * @return The List Type.
	 */
	virtual const EFriendsDisplayLists::Type GetListType() const = 0;

	/**
	 * Set new online friend.
	 *
	 * @param InOnlineFriend The new online friend.
	 * @see GetOnlineFriend
	 */
	virtual void SetOnlineFriend( TSharedPtr< FOnlineFriend > InOnlineFriend ) = 0;

	/**
	 * Set new online user.
	 *
	 * @param InOnlineUser The new online user.
	 */
	virtual void SetOnlineUser( TSharedPtr< FOnlineUser > InOnlineFriend ) = 0;

	/**
	 * Clear updated flag.
	 */
	virtual void ClearUpdated() = 0;

	/**
	 * Check if we have been updated.
	 *
	 * @return true if updated.
	 */
	virtual bool IsUpdated() = 0;

	/** Set if pending invitation response. */
	virtual void SetPendingInvite() = 0;

	/** Set if pending invitation response. */
	virtual void SetPendingAccept() = 0;

	/** Set if pending delete. */
	virtual void SetPendingDelete() = 0;

	/** Get if pending delete. */
	virtual bool IsPendingDelete() const = 0;

	/** Get if pending invitation response. */
	virtual bool IsPendingAccepted() const = 0;

	/** Get if is from a game request. */
	virtual bool IsGameRequest() const = 0;

	virtual void SetPendingAction(EFriendActionType::Type InPendingAction)
	{
		PendingActionType = InPendingAction;
	}

	virtual EFriendActionType::Type GetPendingAction()
	{
		return PendingActionType;
	}

	/**
	 * Get the invitation status.
	 *
	 * @return The invitation status.
	 */
	virtual EInviteStatus::Type GetInviteStatus() = 0;

	IFriendItem()
		: PendingActionType(EFriendActionType::MAX_None)
	{}

private:
	EFriendActionType::Type PendingActionType;
public:

	/** Virtual destructor. */
	virtual ~IFriendItem() { }
};
