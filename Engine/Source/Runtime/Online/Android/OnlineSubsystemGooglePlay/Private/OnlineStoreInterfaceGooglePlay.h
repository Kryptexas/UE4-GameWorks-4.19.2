// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineStoreInterface.h"


/**
 * The resulting state of an iap transaction
 */
namespace EInAppPurchaseResult
{
	enum Type
	{
		Succeeded = 0,
		RestoredFromServer,
		Failed,
		Cancelled,
	};
}

/**
 * Implementation of the Platform Purchase receipt. For this we provide an identifier and the encrypted data.
 */
class FGooglePlayPurchaseReceipt : public IPlatformPurchaseReceipt
{
public:

};


/**
 *	FOnlineStoreGooglePlay - Implementation of the online store for GooglePlay
 */
class FOnlineStoreGooglePlay : public IOnlineStore
{
public:
	/** C-tor */
	FOnlineStoreGooglePlay();
	/** Destructor */
	virtual ~FOnlineStoreGooglePlay();

	// Begin IOnlineStore 
	virtual bool QueryForAvailablePurchases(const TArray<FString>& ProductIDs, FOnlineProductInformationReadRef& InReadObject) override;
	virtual bool BeginPurchase(const FString& ProductId, FOnlineInAppPurchaseTransactionRef& InReadObject) override;
	virtual bool IsAllowedToMakePurchases() override;
	// End IOnlineStore 

private:

	/** Delegate fired when a query for purchases has completed, whether successful or unsuccessful */
	FOnQueryForAvailablePurchasesComplete OnQueryForAvailablePurchasesCompleteDelegate;

	/** Delegate fired when a purchase transaction has completed, whether successful or unsuccessful */
	FOnInAppPurchaseComplete OnPurchaseCompleteDelegate;
};

typedef TSharedPtr<FOnlineStoreGooglePlay, ESPMode::ThreadSafe> FOnlineStoreGooglePlayPtr;

