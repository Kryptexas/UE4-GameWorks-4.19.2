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


struct FMicrotransactionPurchaseInfo
{
	FString Identifier;
	FString DisplayName;
	FString DisplayDescription;
	FString DisplayPrice;
	FString CurrencyType;
};


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
	virtual bool QueryForAvailablePurchases() OVERRIDE;

	virtual bool IsAllowedToMakePurchases() OVERRIDE;

	virtual bool BeginPurchase(int Index) OVERRIDE;
	// End IOnlineStore interface

	// Begin FExec Interface
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar=*GLog ) OVERRIDE;
	// End FExec Interface

	/**
	 * Process a response from the StoreKit
	 */
	void ProcessProductsResponse( SKProductsResponse* Response );

private:
	FStoreKitHelper* StoreHelper;
	
	TArray< FMicrotransactionPurchaseInfo > AvailableProducts;

	FOnQueryForAvailablePurchasesComplete OnQueryForAvailablePurchasesCompleteDelegate;

	FOnPurchaseComplete OnPurchaseCompleteDelegate;
};

typedef TSharedPtr<FOnlineStoreInterfaceIOS, ESPMode::ThreadSafe> FOnlineStoreInterfaceIOSPtr;

