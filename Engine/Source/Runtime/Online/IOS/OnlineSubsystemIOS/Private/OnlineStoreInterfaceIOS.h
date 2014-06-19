// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineStoreInterface.h"

#include <StoreKit/SKRequest.h>
#include <StoreKit/SKError.h>
#include <StoreKit/SKProduct.h>
#include <StoreKit/SKProductsRequest.h>
#include <StoreKit/SKPaymentTransaction.h>
#include <StoreKit/SKPayment.h>
#include <StoreKit/SKPaymentQueue.h>


@interface FStoreKitHelper : NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
};

@property (nonatomic, strong) SKRequest *Request;
@property (nonatomic, strong) NSArray *AvailableProducts;

- (void) makePurchase:(NSMutableSet*) productIDs;
- (void) requestProductData:(NSMutableSet*) productIDs;
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;

@end


/**
 *	FOnlineStoreInterfaceIOS - Implementation of the online store for IOS
 */
class FOnlineStoreInterfaceIOS : public IOnlineStore, public FSelfRegisteringExec
{
private:
	bool bIsPurchasing, bIsProductRequestInFlight;

public:
	FOnlineStoreInterfaceIOS();
	virtual ~FOnlineStoreInterfaceIOS();

	// Begin IOnlineStore interface
	virtual bool QueryForAvailablePurchases() override;

	virtual bool IsAllowedToMakePurchases() override;

	virtual bool BeginPurchase(int Index) override;
	// End IOnlineStore interface

	// Begin FExec Interface
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar=*GLog ) override;
	// End FExec Interface

	/**
	 * Process a response from the StoreKit
	 */
	void ProcessProductsResponse( SKProductsResponse* Response );


	/**
	 * Micro-transaction purchase information
	 */
	struct FMicrotransactionPurchaseInfo
	{
		FMicrotransactionPurchaseInfo(FString InProductIdentifier)
		{
			Identifier = InProductIdentifier;
			bWasUpdatedByServer = false;
		}

		// The product identifier that matches the one in iTunesConnect
		FString Identifier;

		// Display name, gathered from querying the Apple Store
		FString DisplayName;

		// Display description, gathered from querying the Apple Store
		FString DisplayDescription;

		// Product price, gathered from querying the Apple Store
		FString DisplayPrice;

		// The currency type for this MT, gathered from querying the Apple Store
		FString CurrencyType;

		// flag that lets the user know if the data has been completed from the server
		bool bWasUpdatedByServer;
	};


private:
	FStoreKitHelper* StoreHelper;
	
	TArray< FMicrotransactionPurchaseInfo > AvailableProducts;

	FOnQueryForAvailablePurchasesComplete OnQueryForAvailablePurchasesCompleteDelegate;

	FOnPurchaseComplete OnPurchaseCompleteDelegate;
};

typedef TSharedPtr<FOnlineStoreInterfaceIOS, ESPMode::ThreadSafe> FOnlineStoreInterfaceIOSPtr;

