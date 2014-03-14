// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineDelegateMacros.h"
#include "OnlineSubsystemTypes.h"

/**
 * Details of an entitlement
 */
struct FEntitlement
{
	/** Unique Entitlement Id associated with this entitlement */
	FString EntitlementId;
	/** Id unique to this player's instance of this entitlement */
	FString EntitlementInstanceId;
	/** Quantity of this entitlement */
	int32 Quantity;


	/**
	 * Equality operator
	 */
	bool operator==(const FEntitlement& Other) const
	{
		return Other.EntitlementId == EntitlementId &&
			Other.EntitlementInstanceId == EntitlementInstanceId &&
			Other.Quantity == Quantity;
	}

	/**
	 * Assignment operator.  Copies the other into this object.
	 */
	FEntitlement& operator=(const FEntitlement& Other)
	{
		if (this != &Other)
		{
			this->EntitlementId = Other.EntitlementId;
			this->EntitlementInstanceId = Other.EntitlementInstanceId;
			this->Quantity = Other.Quantity;
		}
		return *this;
	}

	FEntitlement(const FString& InEntitlementId, const FString& InEntitlemendInstanceId, int32 InQuantity)
		: EntitlementId(InEntitlementId)
		, EntitlementInstanceId(InEntitlemendInstanceId)
		, Quantity(InQuantity)
	{
	}

	FEntitlement()
	{
	}
};


/**
 * Delegate declaration for when an entitlements group is created
 *
 * @param bWasSuccessful true if server was contacted and a valid result received
 * @param Error string representing the error condition
 * @param UserId of the user who was granted entitlements in this callback
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnCreateEntitlementsGroupComplete, bool, const FString&, const FUniqueNetId&);
typedef FOnCreateEntitlementsGroupComplete::FDelegate FOnCreateEntitlementsGroupCompleteDelegate;

/**
 * Delegate declaration for when an entitlements group is deleted
 *
 * @param bWasSuccessful true if server was contacted and a valid result received
 * @param Error string representing the error condition
 * @param UserId of the user who was granted entitlements in this callback
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDeleteEntitlementsGroupComplete, bool, const FString&, const FUniqueNetId&);
typedef FOnDeleteEntitlementsGroupComplete::FDelegate FOnDeleteEntitlementsGroupCompleteDelegate;

/**
 * Delegate declaration for when entitlements are enumerated
 *
 * @param bWasSuccessful true if server was contacted and a valid result received
 * @param Error string representing the error condition
 * @param UserId of the user who was granted entitlements in this callback
 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnEnumerateEntitlementsComplete, bool, const FString&, const FUniqueNetId&);
typedef FOnEnumerateEntitlementsComplete::FDelegate FOnEnumerateEntitlementsCompleteDelegate;

/**
 * Delegate declaration for when an array of entitlements are granted
 *
 * @param bWasSuccessful true if server was contacted and a valid result received
 * @param Error string representing the error condition
 * @param UserId of the user who was granted entitlements in this callback
 * @param OutNewEntitlements an array of entitlements that were changed as a result of this call
 * @param OutModifiedEntitlements an array of entitlements that were changed as a result of this call
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnGrantEntitlementsComplete, bool, const FString&, const FUniqueNetId&, TArray<FEntitlement>&, TArray<FEntitlement>&);
typedef FOnGrantEntitlementsComplete::FDelegate FOnGrantEntitlementsCompleteDelegate;

/**
 * Delegate declaration for when an entitlement is consumed
 *
 * @param bWasSuccessful true if server was contacted and a valid result received
 * @param Error string representing the error condition
 * @param UserId of the user who was granted entitlements in this callback
 * @param OutNewEntitlements an array of entitlements that were changed as a result of this call
 * @param OutModifiedEntitlements an array of entitlements that were changed as a result of this call
 */
DECLARE_MULTICAST_DELEGATE_FiveParams(FOnConsumeEntitlementComplete, bool, const FString&, const FUniqueNetId&, TArray<FEntitlement>&, TArray<FEntitlement>&);
typedef FOnConsumeEntitlementComplete::FDelegate FOnConsumeEntitlementCompleteDelegate;


/**
 * Interface for granting and retrieving user entitlements
 */
class IOnlineEntitlements
{

protected:
	IOnlineEntitlements() {};

public:
	virtual ~IOnlineEntitlements() {};

	/**
	 * Checks if a user has a single entitlement in the local cache
	 *
	 * @param UserId the ID of the user to get this entitlement for
	 * @param EntitlementId the ID of the entitlement to retrieve
	 * @return true if the requested entitlement was found in the local cache for the user
	 */
	virtual bool HasEntitlement(const FUniqueNetId& UserId, const FString& EntitlementId) = 0;

	/**
	 * Checks for and retrieves a single cached entitlement for a user
	 *
	 * @param UserId the ID of the user to get this entitlement for
	 * @param EntitlementId the ID of the entitlement to retrieve
	 * @param OutEntitlement out parameter for copying the entitlement details into if found
	 * @return true if the requested entitlement was found in the local cache for the user
	 */
	virtual bool GetEntitlement(const FUniqueNetId& UserId, const FString& EntitlementId, FEntitlement& OutEntitlement) = 0;
	
	/**
	 * Gets the cached entitlement set for the requested user
	 *
	 * @param UserId the ID of the user to get entitlements for
	 * @param OutUserEntitlements out parameter to copy the user's entitlements into
	 * @return true if the cached entitlements were available for the requested user
	 */
	virtual bool GetEntitlements(const FUniqueNetId& UserId, TMap<FString, FEntitlement>& OutUserEntitlements) = 0;

	/**
	 * Contacts server and creates the entitlements group where the user's entitlements will be stored
	 *
	 * @param UserId the ID of the user to act on
	 * @return true if the operation started successfully
	 */
	virtual bool CreateEntitlementsGroup(const FUniqueNetId& UserId) = 0;

	/**
	 * Delegate instanced called when entitlements group creation has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition
	 * @param UserId of the user who was granted entitlements in this callback
	 */
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnCreateEntitlementsGroupComplete, bool, const FString&, const FUniqueNetId&);

	/**
	 * Contacts server and deletes the entitlements group and all entitlements within it
	 *
	 * @param UserId the ID of the user to act on
	 * @return true if the operation started successfully
	 */
	virtual bool DeleteEntitlementsGroup(const FUniqueNetId& UserId) = 0;
	
	/**
	 * Delegate instanced called when entitlements group deletion has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition
	 * @param UserId of the user who was granted entitlements in this callback
	 */
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnDeleteEntitlementsGroupComplete, bool, const FString&, const FUniqueNetId&);

	/**
	 * Contacts server and retrieves the list of the user's entitlements, caching them locally
	 *
	 * @param UserId the ID of the user to act on
	 * @return true if the operation started successfully
	 */
	virtual bool EnumerateEntitlements(const FUniqueNetId& UserId) = 0;
	
	/**
	 * Delegate instanced called when enumerating entitlements has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition
	 * @param UserId of the user who was granted entitlements in this callback
	 */
	DEFINE_ONLINE_DELEGATE_THREE_PARAM(OnEnumerateEntitlementsComplete, bool, const FString&, const FUniqueNetId&);

	/**
	 * Contacts server and gives an entitlement to the user
	 *
	 * @param UserId the ID of the user to give this entitlement to
	 * @param EntitlementId the entitlement to grant to this user
	 * @return true if the operation started successfully
	 */
	virtual bool GrantEntitlement(const FUniqueNetId& UserId, const FString& EntitlementId) = 0;

	/**
	 * Contacts server and gives multiple entitlements to the user
	 *
	 * @param UserId the ID of the user to give this entitlement to
	 * @param EntitlementIds the array of entitlementIds to grant to this user
	 * @return true if the operation started successfully
	 */
	virtual bool GrantEntitlements(const FUniqueNetId& UserId, const TArray<FString>& EntitlementIds) = 0;

	/**
	 * Delegate instanced called when granting entitlements has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition
	 * @param UserId of the user who was granted entitlements in this callback
	 * @param OutNewEntitlements an array of entitlements that were changed as a result of this call
	 * @param OutModifiedEntitlements an array of entitlements that were changed as a result of this call
	 */
	DEFINE_ONLINE_DELEGATE_FIVE_PARAM(OnGrantEntitlementsComplete, bool, const FString&, const FUniqueNetId&, TArray<FEntitlement>&, TArray<FEntitlement>&);

	/**
	 * Contacts server and consumes a particular entitlement from the user
	 *
	 * @param UserId the ID of the user to act on
	 * @param EntitlementId the entitlement to remove from this user
	 * @return true if the operation started successfully
	 */
	virtual bool ConsumeEntitlement(const FUniqueNetId& UserId, const FString& EntitlementId) = 0;

	/**
	 * Delegate instanced called when deleting an entitlement has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition
	 * @param UserId of the user who was granted entitlements in this callback
	 * @param OutNewEntitlements an array of entitlements that were changed as a result of this call
	 * @param OutModifiedEntitlements an array of entitlements that were changed as a result of this call
	 */
	DEFINE_ONLINE_DELEGATE_FIVE_PARAM(OnConsumeEntitlementComplete, bool, const FString&, const FUniqueNetId&, TArray<FEntitlement>&, TArray<FEntitlement>&);
};

typedef TSharedPtr<IOnlineEntitlements, ESPMode::ThreadSafe> IOnlineEntitlementsPtr;
