// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
 #include "OnlineDelegateMacros.h"

/**
 * Delegate fired when a session create request has completed
 *
 * @param SessionName the name of the session this callback is for
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnQueryForAvailablePurchasesComplete, bool);
typedef FOnQueryForAvailablePurchasesComplete::FDelegate FOnQueryForAvailablePurchasesCompleteDelegate;

/**
 * Delegate fired when the online session has transitioned to the started state
 *
 * @param SessionName the name of the session the that has transitioned to started
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPurchaseComplete, bool);
typedef FOnPurchaseComplete::FDelegate FOnPurchaseCompleteDelegate;


/**
 *	IOnlineStore - Interface class for microtransactions
 */
class IOnlineStore
{
public:

	/**
	 * Search for what purchases are available
	 *
	 * @return - Whether a request was sent to check for purchases
	 */
	virtual bool QueryForAvailablePurchases() = 0;

	/**
	 * Delegate which is executed when QueryForAvailablePurchases completes
	 */
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnQueryForAvailablePurchasesComplete, bool);

	/**
	 * Check whether microtransactions can be purchased
	 *
	 * @return - Whether the device can make purchases
	 */
	virtual bool IsAllowedToMakePurchases() = 0;
	
	/**
	 * Begin a purchase transaction for the product which relates to the given index
	 *
	 * @param Index The id of the object we would like to purchase.
	 *
	 * @return - whether a purchase request was sent
	 */
	virtual bool BeginPurchase(int Index) = 0;
	
	/**
	 * Delegate which is executed when a Purchase completes
	 */
	DEFINE_ONLINE_DELEGATE_ONE_PARAM(OnPurchaseComplete, bool);
};