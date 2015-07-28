// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineDelegateMacros.h"
#include "OnlineStoreInterfaceV2.h"
#include "OnlineEntitlementsInterface.h"

/**
 * Info needed for checkout
 */
class FPurchaseCheckoutRequest
{
public:
	/**
	 * Add a offer entry for purchase
	 *
	 * @param OfferId id of offer to be purchased
	 * @param Quantity number to purchase
	 */
	void AddPurchaseOffer(const FUniqueOfferId& OfferId, int32 Quantity);

	/**
	 * Single offer entry for purchase 
	 */
	struct FPurchaseOfferEntry
	{
		FUniqueOfferId OfferId;
		int32 Quantity;
	};
	/** List of offers being purchased */
	TArray<FPurchaseOfferEntry> PurchaseOffers;
};

/**
 * State of a purchase transaction
 */
namespace EPurchaseTransactionState
{
	enum Type
	{
		/** processing has not started on the purchase */
		NotStarted,
		/** currently processing the purchase */
		Processing,
		/** purchase completed successfully */
		Purchased,
		/** purchase completed but failed */
		Failed,
		/** purchase canceled by user */
		Canceled,
		/** prior purchase that has been restored */
		Restored
	};
}

/**
 * Receipt result from checkout
 */
class FPurchaseReceipt
{
public:
	/** Unique Id for this transaction/order */
	FString TransactionId;
	/** Current state of the purchase */
	EPurchaseTransactionState::Type TransactionState;
	/** List of offers being purchased */
	TArray<FUniqueOfferId> OfferIds;
	/** The entitlements obtained from the completed purchase */
	TArray<FUniqueEntitlementId> EntitlementIds;
	/** Receipt validation data */
	FString ReceiptValidation;
};

/**
 * Info needed for code redemption
 */
class FRedeemCodeRequest
{
public:
	/** Code to redeem */
	FString Code;
};

/**
 * Delegate called when checkout process completes
 */
DECLARE_DELEGATE_FourParams(FOnPurchaseCheckoutComplete, const FUniqueNetId& /*UserId*/, bool /*bWasSuccessful*/, const TSharedRef<FPurchaseReceipt>& /*Receipt*/, const FString& /*Error*/);

/**
 * Delegate called when code redemption process completes
 */
DECLARE_DELEGATE_FourParams(FOnPurchaseRedeemCodeComplete, const FUniqueNetId& /*UserId*/, bool /*bWasSuccessful*/, const TSharedRef<FPurchaseReceipt>& /*Receipt*/, const FString& /*Error*/);

/**
 * Delegate called when query receipt process completes
 */
DECLARE_DELEGATE_ThreeParams(FOnQueryReceiptsComplete, const FUniqueNetId& /*UserId*/, bool /*bWasSuccessful*/, const FString& /*Error*/);

/**
 *	IOnlinePurchase - Interface for IAP (In App Purchases) services
 */
class IOnlinePurchase
{
public:

	/**
	 * Determine if user is allowed to purchase from store 
	 *
	 * @param UserId user initiating the request
	 *
	 * @return true if user can make a purchase
	 */
	virtual bool IsAllowedToPurchase(const FUniqueNetId& UserId) = 0;

	/**
	 * Initiate the checkout process for purchasing offers via payment
	 *
	 * @param UserId user initiating the request
	 * @param CheckoutRequest info needed for the checkout request
	 * @param Delegate completion callback
	 *
	 * @return true if user can make a purchase
	 */
	virtual bool Checkout(const FUniqueNetId& UserId, const FPurchaseCheckoutRequest& CheckoutRequest, const FOnPurchaseCheckoutComplete& Delegate = FOnPurchaseCheckoutComplete()) = 0;

	/**
	 * Initiate the checkout process for obtaining offers via code redemption
	 *
	 * @param UserId user initiating the request
	 * @param RedeemCodeRequest info needed for the redeem request
	 * @param Delegate completion callback
	 *
	 * @return true if user can make a purchase
	 */
	virtual bool RedeemCode(const FUniqueNetId& UserId, const FRedeemCodeRequest& RedeemCodeRequest, const FOnPurchaseRedeemCodeComplete& Delegate = FOnPurchaseRedeemCodeComplete()) = 0;

	/**
	 * Query for all of the user's receipts from prior purchases
	 *
	 * @param UserId user initiating the request
	 * @param Delegate completion callback
	 *
	 * @return true if user can make a purchase
	 */
	virtual bool QueryReceipts(const FUniqueNetId& UserId, const FOnQueryReceiptsComplete& Delegate = FOnQueryReceiptsComplete()) = 0;

	/**
	 * Get list of cached receipts for user (includes transactions currently being processed)
	 *
	 * @param UserId user initiating the request
	 * @param OutReceipts [out] list of receipts for the user 
	 */
	virtual void GetReceipts(const FUniqueNetId& UserId, TArray<FPurchaseReceipt>& OutReceipts) const = 0;
	
};