// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineSubsystemTypes.h"
#include "OnlineChatInterface.h"

/**
 * Key value data that can be sent to party members
 */
typedef FOnlineKeyValuePairs<FString, FVariantData> FOnlinePartyAttrs;

/**
 * Party member user info returned by IOnlineParty interface
 */
class FOnlinePartyMember : public FOnlineUser
{
public:
	
	/**
	 * @return presence info for an online friend
	 */
	virtual const class FOnlineUserPresence& GetPresence() const = 0;
};

/**
 * Data associated with the entire party
 */
class FOnlinePartyData : public TSharedFromThis<FOnlinePartyData>
{
public:
	FOnlinePartyData() {}
	virtual ~FOnlinePartyData() {}

	/**
	 * Get an attribute from the party data
	 *
	 * @param AttrName - key for the attribute
	 * @param OutAttrValue - [out] value for the attribute if found
	 *
	 * @return true if the attribute was found
	 */
	bool GetAttribute(const FString& AttrName, FVariantData& OutAttrValue) const
	{
		bool bResult = false;

		const FVariantData* FoundValuePtr = KeyValAttrs.Find(AttrName);
		if (FoundValuePtr != nullptr)
		{
			OutAttrValue = *FoundValuePtr;
		}

		return bResult;
	}

	/**
	 * Set an attribute from the party data
	 *
	 * @param AttrName - key for the attribute
	 * @param AttrValue - value to set the attribute to
	 */
	void SetAttribute(const FString& AttrName, const FVariantData& AttrValue)
	{
		FVariantData& NewAttrValue = KeyValAttrs.FindOrAdd(AttrName);
		NewAttrValue = AttrValue;
	}

	/**
	 * Dump state about the party data for debugging
	 */
	virtual FString DumpDebug() const
	{
		FString Result;
		const FOnlinePartyAttrs& PartyAttrs = KeyValAttrs;
		for (FOnlinePartyAttrs::TConstIterator It(PartyAttrs); It; ++It)
		{
			FString Key = It.Key();
			const FVariantData& Value = It.Value();
			Result += FString::Printf(TEXT("\n%s=%s"), *Key, *Value.ToString());
		}
		return Result;
	}

	/** map of key/val attributes that represents the data */
	FOnlinePartyAttrs  KeyValAttrs;
};

/**
 * Info needed to join a party
 */
class IOnlinePartyJoinInfo
{
public:
	IOnlinePartyJoinInfo() {}
	virtual ~IOnlinePartyJoinInfo() {}

	/**
	 * @return id of the user this invite is from
	 */
	virtual TSharedRef<const FUniqueNetId> GetLeaderId() const = 0;

	/**
	* @return id of the party associated with this invite
	*/
	virtual TSharedRef<FOnlinePartyId> GetPartyId() const = 0;

	/**
	* @return the ReservationKey for this invite
	*/
	virtual FString GetReservationKey() const = 0;
};

/**
 * Permissions for joining a party
 */
namespace EPartyPermissions
{
	enum Type
	{
		/** any user that discovers the party can join */
		Public,
		/** invite required to join the party */
		Private
	};
}

/**
 * Options for configuring a new party or for updating an existing party
 */
struct FPartyConfiguration
{
	FPartyConfiguration()
		: Permissions(EPartyPermissions::Public)
		, bShouldPublishToPresence(true)
		, bCanNonLeaderPublishToPresence(false)
		, MaxMembers(5)
	{}

	/** Permission for configuring party */
	EPartyPermissions::Type Permissions;
	/** should publish info to presence*/
	bool bShouldPublishToPresence;
	/** should publish info to presence*/
	bool bCanNonLeaderPublishToPresence;
	/** clients can add whatever data they want for configuration options */
	FOnlinePartyData ClientConfigData;
	/** Maximum active members allowed. 0 means no maximum. */
	int32 MaxMembers;
	/** Human readable nickname */
	FString Nickname;
	/** Human readable description */
	FString Description;
	/** Human readable password for party. */
	FString Password;
};

namespace EPartyState
{
	enum Type
	{
		None,
		CreatePending,
		DestroyPending,
		JoinPending,
		LeavePending,
		Success,
		Failed
	};

	/** @return the stringified version of the enum passed in */
	inline const TCHAR* ToString(Type EnumVal)
	{
		switch (EnumVal)
		{
		case None:
		{
			return TEXT("None");
		}
		case CreatePending:
		{
			return TEXT("Create pending");
		}
		case DestroyPending:
		{
			return TEXT("Destroy pending");
		}
		case JoinPending:
		{
			return TEXT("Join pending");
		}
		case LeavePending:
		{
			return TEXT("Leave pending");
		}
		case Success:
		{
			return TEXT("Success");
		}
		case Failed:
		{
			return TEXT("Failed");
		}
		}
		return TEXT("");
	}
}

/**
 * Current state associated with a party
 */
class FOnlinePartyInfo : public TSharedFromThis<FOnlinePartyInfo>
{
	FOnlinePartyInfo();
public:
	FOnlinePartyInfo(const TSharedPtr<const FOnlinePartyId>& InPartyId)
		: State(EPartyState::None)
		, PartyId(InPartyId)
	{}

	virtual ~FOnlinePartyInfo() 
	{}

	/**
	 * Dump state about the party info for debugging
	 */
	virtual FString DumpState() const
	{
		return FString::Printf(TEXT("PartyInfo Id: %s Leader: %s ChatId: %s State: %s"),
			PartyId.IsValid() ? *PartyId->ToString() : TEXT("Invalid"),
			PartyLeader.IsValid() ? *PartyLeader->ToString() : TEXT("Invalid"),
			*RoomId,
			EPartyState::ToString(State));
	}

	/** The current state of the party */
	EPartyState::Type State;
	/** Current state of configuration */
	FPartyConfiguration Config;
	/** unique id of the party */
	TSharedPtr<const FOnlinePartyId> PartyId;
	/** leader of the party */
	TSharedPtr<const FUniqueNetId> PartyLeader;
	/** id of chat room associated with the party */
	FChatRoomId RoomId;
};

// Completion delegates

/**
 * Party creation async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully created new party
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnCreatePartyComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Party update async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully updated party
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnUpdatePartyComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Party destroy async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully destroyed party
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnDestroyPartyComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Not Yet Implemented
 */
DECLARE_DELEGATE_FourParams(FOnRequestInvitationComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Not sure if this is needed or desired
 */
DECLARE_DELEGATE_FiveParams(FOnSendInvitationComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*RecipientId*/, const FString& /*Error*/);
/**
 * Party join async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully joined party
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnJoinPartyComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Party leave async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully left party
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnLeavePartyComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Sending an invite to a user to join party async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully sent invite
 * @param PartyId - id associated with the party
 * @param RecipientId - id of user invite is being sent to
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FiveParams(FOnSendPartyInviteComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*RecipientId*/, const FString& /*Error*/);
/**
 * Accepting an invite to a user to join party async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully sent invite
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnAcceptPartyInviteComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Rejecting an invite to a user to join party async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully sent invite
 * @param PartyId - id associated with the party
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FourParams(FOnRejectPartyInviteComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FString& /*Error*/);
/**
 * Kicking a member of a party async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully kicked member
 * @param PartyId - id associated with the party
 * @param MemberId - id of member being kicked
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FiveParams(FOnKickPartyMemberComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/, const FString& /*Error*/);
/**
 * Promoting a member of a party async task completed callback
 *
 * @param LocalUserId - id of user that initiated the request
 * @param bWasSuccessful - true if successfully promoted member to admin
 * @param PartyId - id associated with the party
 * @param MemberId - id of member being promoted to admin
 * @param Error - string with error info if any
 */
DECLARE_DELEGATE_FiveParams(FOnPromotePartyMemberComplete, const FUniqueNetId& /*LocalUserId*/, bool /*bWasSuccessful*/, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/, const FString& /*Error*/);

// Notification delegates

/**
 * Notification when a new invite is received
 *
 * @param RecipientId - id of user that this invite is for
 * @param SenderId - id of member that sent the invite
 * @param PartyId - id associated with the party
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPartyInviteReceived, const FUniqueNetId& /*RecipientId*/, const FUniqueNetId& /*SenderId*/, const FOnlinePartyId& /*PartyId*/);
typedef FOnPartyInviteReceived::FDelegate FOnPartyInviteReceivedDelegate;

/**
 * Notification when the new invite list has changed
 *
 * @param RecipientId - id of user that this invite is for
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyInvitesChanged, const FUniqueNetId& /*RecipientId*/);
typedef FOnPartyInvitesChanged::FDelegate FOnPartyInvitesChangedDelegate;

/**
 * Notification when a party has been disbanded 
 *
 * @param PartyId - id associated with the party
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPartyDisbanded, const FOnlinePartyId& /*PartyId*/);
typedef FOnPartyDisbanded::FDelegate FOnPartyDisbandedDelegate;

/**
 * Notification when a member joins the party
 *
 * @param PartyId - id associated with the party
 * @param MemberId - id of member that joined
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPartyMemberJoined, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/);
typedef FOnPartyMemberJoined::FDelegate FOnPartyMemberJoinedDelegate;

/**
 * Notification when a member leaves the party
 *
 * @param PartyId - id associated with the party
 * @param MemberId - id of member that left
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPartyMemberLeft, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/);
typedef FOnPartyMemberLeft::FDelegate FOnPartyMemberLeftDelegate;

/**
 * Notification when a member is promoted to admin in the party
 *
 * @param PartyId - id associated with the party
 * @param MemberId - id of member that was promoted to admin
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPartyMemberPromoted, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/);
typedef FOnPartyMemberPromoted::FDelegate FOnPartyMemberPromotedDelegate;

/**
 * Notification when party data is updated
 *
 * @param PartyId - id associated with the party
 * @param PartyData - party data that was updated
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPartyDataReceived, const FOnlinePartyId& /*PartyId*/, const TSharedRef<FOnlinePartyData>& /*PartyData*/);
typedef FOnPartyDataReceived::FDelegate FOnPartyDataReceivedDelegate;

/**
 * Notification when party member data is updated
 *
 * @param PartyId - id associated with the party
 * @param MemberId - id of member that had updated data
 * @param PartyMemberData - party member data that was updated
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPartyMemberDataReceived, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/, const TSharedRef<FOnlinePartyData>& /*PartyMemberData*/);
typedef FOnPartyMemberDataReceived::FDelegate FOnPartyMemberDataReceivedDelegate;

/**
 * Notification when an invite is accepted
 *
 * @param PartyId - id associated with the party
 * @param MemberId - id of member that rejected the invite
 * @param bWasAccepted - whether or not the invite was accepted
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPartyInviteResponse, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*RejectorId*/, bool /*bWasAccepted*/);
typedef FOnPartyInviteResponse::FDelegate FOnPartyInviteResponseDelegate;

/**
 * Notification when a new reservation request is received
 *
 * @param RecipientId - id of user that this reservation request is for
 * @param SenderId - id of member that sent the request
 * @param PartyId - id associated with the party
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPartyJoinRequestReceived, const FUniqueNetId& /*RecipientId*/, const FUniqueNetId& /*SenderId*/, const FOnlinePartyId& /*PartyId*/);
typedef FOnPartyJoinRequestReceived::FDelegate FOnPartyJoinRequestReceivedDelegate;

/**
 * Interface definition for the online party services 
 * Allows for forming a party and communicating with party members
 */
class IOnlineParty
{
protected:
	IOnlineParty() {};

public:
	virtual ~IOnlineParty() {};

	/**
	 * Create a new party
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyConfig - configuration for the party (can be updated later)
	 * @param Delegate - called on completion
	 * @param UserRoomId - this forces the name of the room to be this value
	 *
	 * @return true if task was started
	 */
	virtual bool CreateParty(const FUniqueNetId& LocalUserId, const FPartyConfiguration& PartyConfig, const FOnCreatePartyComplete& Delegate = FOnCreatePartyComplete()) = 0;

	/**
	 * Update an existing party with new configuration
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyConfig - configuration for the party
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool UpdateParty(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FPartyConfiguration& PartyConfig, bool bShouldRegenerateAccessToken = false, const FOnUpdatePartyComplete& Delegate = FOnUpdatePartyComplete()) = 0;

	/**
	 * Destroy an existing party. Party members will be disbanded
	 * Only Owner/Admin can destroy the party
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool DestroyParty(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FOnDestroyPartyComplete& Delegate = FOnDestroyPartyComplete()) = 0;

	/**
	 * Join an existing party
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param RecipientId - id of the party leader
	 * @param ReservationKey - the ReservationKey for the party
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool JoinParty(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& RecipientId, const FString& ReservationKey, const FOnJoinPartyComplete& Delegate = FOnJoinPartyComplete()) = 0;

	/**
	 * Leave an existing party
	 * All existing party members notified of member leaving (see FOnPartyMemberLeft)
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool LeaveParty(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FOnLeavePartyComplete& Delegate = FOnLeavePartyComplete()) = 0;

	/**
	* Approve a request to join a party
	*
	* @param LocalUserId - user making the request
	* @param PartyId - id of an existing party
	* @param RecipientId - id of the user being invited
	*
	* @return true if task was started
	*/
	virtual bool ApproveJoinRequest(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& RecipientId, bool bIsApproved) = 0;

	/**
	* sends an request to a party leader for an invite to a party that could not otherwise be joined
	* if the player accepts the invite they will be sent the data needed to trigger a call to RequestReservation
	*
	* @param LocalUserId - user making the request
	* @param PartyId - id of an existing party
	* @param LeaderId - id of the user to request an invitation from
	* @param Delegate - called on completion
	*
	* @return true if task was started
	*/
	virtual bool RequestInvitation(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& LeaderId, const FOnRequestInvitationComplete& Delegate = FOnRequestInvitationComplete()) = 0;

	/**
	 * sends an invitation to a user that could not otherwise join a party
	 * if the player accepts the invite they will be sent the data needed to trigger a call to RequestReservation
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param RecipientId - id of the user being invited
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool SendInvitation(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& RecipientId, const FOnSendInvitationComplete& Delegate = FOnSendInvitationComplete()) = 0;

	/**
	* Accept an invite to a party
	*
	* @param LocalUserId - user making the request
	* @param PartyId - id of an existing party
	* @param Delegate - called on completion
	*
	* @return true if task was started
	*/
	virtual bool AcceptInvitation(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FOnJoinPartyComplete& Delegate = FOnJoinPartyComplete()) = 0;

	/**
	* Reject an invite to a party
	*
	* @param LocalUserId - user making the request
	* @param PartyId - id of an existing party
	*
	* @return true if task was started
	*/
	virtual bool RejectInvitation(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId) = 0;

	/**
	 * Kick a user from an existing party
	 * Only admin can kick a party member
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param MemberId - id of the user being kicked
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool KickMember(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId, const FOnKickPartyMemberComplete& Delegate = FOnKickPartyMemberComplete()) = 0;

	/**
	 * Promote a user from an existing party to be admin
	 * All existing party members notified of promoted member (see FOnPartyMemberPromoted)
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param MemberId - id of the user being promoted
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool PromoteMember(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId, const FOnPromotePartyMemberComplete& Delegate = FOnPromotePartyMemberComplete()) = 0;

	/**
	 * Set party data and broadcast to all members
	 * Only current data can be set and no history of past party data is preserved
	 * Party members notified of new data (see FOnPartyDataReceived)
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param PartyData - data to send to all party members
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool UpdatePartyData(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FOnlinePartyAttrs& PartyData) = 0;

	/**
	 * Set party data for a single party member and broadcast to all members
	 * Only current data can be set and no history of past party member data is preserved
	 * Party members notified of new data (see FOnPartyMemberDataReceived)
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param PartyMemberData - member data to send to all party members
	 * @param Delegate - called on completion
	 *
	 * @return true if task was started
	 */
	virtual bool UpdatePartyMemberData(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FOnlinePartyAttrs& PartyMemberData) = 0;


	/**
	 * Get a list of currently joined parties for the user
	 *
	 * @param LocalUserId - user making the request
	 * @param OutPartyIds - list of party ids joined by the current user
	 *
	 * @return true if entries found
	 */
	virtual bool GetJoinedParties(const FUniqueNetId& LocalUserId, TArray< TSharedRef<const FOnlinePartyId> >& OutPartyIds) const = 0;

	/**
	* Get a list of parties the user has been invited to
	*
	* @param LocalUserId - user making the request
	* @param OutPartyIds - list of party ids joined by the current user
	*
	* @return true if entries found
	*/
	virtual bool GetPartyInvites(const FUniqueNetId& LocalUserId, TArray< TSharedRef<IOnlinePartyJoinInfo> >& OutPartyInviteData) const = 0;

	/**
	* Get a list of parties the user has been invited to
	*
	* @param LocalUserId - user making the request
	* @param OutPartyIds - list of party ids joined by the current user
	*
	* @return true if entries found
	*/
	virtual bool GetPartyJoinsInProgress(const FUniqueNetId& LocalUserId, TArray< TSharedRef<IOnlinePartyJoinInfo> >& OutPartyInviteData) const = 0;

	/**
	 * Get info associated with a party
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 *
	 * @return party info or nullptr if not found
	 */
	virtual TSharedPtr<FOnlinePartyInfo> GetPartyInfo(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId) const = 0;

	/**
	 * Get list of current party members
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param OutPartyMembers - list of party members currently in the party
	 *
	 * @return true if entries found
	 */
	virtual bool GetPartyMembers(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, TArray< TSharedRef<FOnlinePartyMember> >& OutPartyMembers) const = 0;

	/**
	* Get list of users requesting to join the party
	*
	* @param LocalUserId - user making the request
	* @param PartyId - id of an existing party
	* @param OutPendingUserIdArray - list of pending party members
	*
	* @return true if entries found
	*/
	virtual bool GetPendingJoinRequests(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, TArray< TSharedRef<const FUniqueNetId> >& OutPendingUserIdArray) const = 0;

	/**
	* Get list of party members invited to the party but not yet joined
	*
	* @param LocalUserId - user making the request
	* @param PartyId - id of an existing party
	* @param OutPendingPartyMembers - list of pending party members
	*
	* @return true if entries found
	*/
	virtual bool GetPendingPartyMembers(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, TArray< TSharedRef<const FUniqueNetId> >& OutPendingPartyMembers) const = 0;

	/**
	 * Get a party member by id
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param MemberId - id of member to find
	 *
	 * @return party member info or nullptr if not found
	 */
	virtual TSharedPtr<FOnlinePartyMember> GetPartyMember(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId) const = 0;	

	/**
	 * Get current cached data associated with a party
	 * FOnPartyDataReceived notification called whenever this data changes
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 *
	 * @return party data or nullptr if not found
	 */
	virtual TSharedPtr<FOnlinePartyData> GetPartyData(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId) const = 0;

	/**
	 * Get current cached data associated with a party member
	 * FOnPartyMemberDataReceived notification called whenever this data changes
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param MemberId - id of member to find data for
	 *
	 * @return party member data or nullptr if not found
	 */
	virtual TSharedPtr<FOnlinePartyData> GetPartyMemberData(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId) const = 0;

	/**
	 * returns true is the user specified by MemberId is the leader of the party specified by PartyId
	 *
	 * @param LocalUserId - user making the request
	 * @param PartyId - id of an existing party
	 * @param MemberId - id of member to test
	 *
	 */
	virtual bool IsMemberLeader(const FUniqueNetId& LocalUserId, const FOnlinePartyId& PartyId, const FUniqueNetId& MemberId) = 0;

	/**
	 * Notification when a new invite is received
	 *
	 * @param RecipientId - id of user that this invite is for
	 * @param SenderId - id of member that sent the invite
	 * @param PartyId - id associated with the party
	 */
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnPartyInviteReceived, const FUniqueNetId& /*RecipientId*/, const FUniqueNetId& /*SenderId*/, const FOnlinePartyId& /*PartyId*/);

	/**
	 * Notification when a the invite list has changed
	 *
	 * @param RecipientId - id of user that the invites have changed for
	 */
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnPartyInvitesChanged, const FUniqueNetId& /*RecipientId*/);

	/**
	 * Notification when a party has been disbanded 
	 *
	 * @param PartyId - id associated with the party
	 */
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnPartyDisbanded, const FOnlinePartyId& /*PartyId*/);

	/**
	 * Notification when a member joins the party
	 *
	 * @param PartyId - id associated with the party
	 * @param MemberId - id of member that joined
	 */
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnPartyMemberJoined, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/);

	/**
	 * Notification when a member leaves the party
	 *
	 * @param PartyId - id associated with the party
	 * @param MemberId - id of member that left
	 */
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnPartyMemberLeft, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/);

	/**
	 * Notification when a member is promoted to admin in the party
	 *
	 * @param PartyId - id associated with the party
	 * @param MemberId - id of member that was promoted to admin
	 */
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnPartyMemberPromoted, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/);

	/**
	 * Notification when party data is updated
	 *
	 * @param PartyId - id associated with the party
	 * @param PartyData - party data that was updated
	 */
	DEFINE_ONLINE_DELEGATE_TWO_PARAM(OnPartyDataReceived, const FOnlinePartyId& /*PartyId*/, const TSharedRef<FOnlinePartyData>& /*PartyData*/);

	/**
	 * Notification when party member data is updated
	 *
	 * @param PartyId - id associated with the party
	 * @param MemberId - id of member that had updated data
	 * @param PartyMemberData - party member data that was updated
	 */
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnPartyMemberDataReceived, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*MemberId*/, const TSharedRef<FOnlinePartyData>& /*PartyMemberData*/);

	/**
	 * Notification when an invite is accepted
	 *
	 * @param PartyId - id associated with the party
	 * @param MemberId - id of member that accepted the invite
	 * @param bWasAccepted - whether or not the invite was accepted
	 */
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnPartyInviteResponse, const FOnlinePartyId& /*PartyId*/, const FUniqueNetId& /*RejectorId*/, bool /*bWasAccepted*/);

	/**
	* Notification when a new reservation request is received
	*
	* @param RecipientId - id of user that this reservation request is for
	* @param SenderId - id of member that sent the request
	* @param PartyId - id associated with the party
	*/
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnPartyJoinRequestReceived, const FUniqueNetId& /*RecipientId*/, const FUniqueNetId& /*SenderId*/, const FOnlinePartyId& /*PartyId*/);

	/**
	 * Dump out party state for all known parties
	 */
	virtual void DumpPartyState() const = 0;

};

typedef TSharedPtr<IOnlineParty, ESPMode::ThreadSafe> IOnlinePartyPtr;
