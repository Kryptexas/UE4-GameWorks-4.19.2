// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemIOSPrivatePCH.h"
#include "OnlineStoreInterface.h"



////////////////////////////////////////////////////////////////////
/// FStoreKitHelper implementation


@implementation FStoreKitHelper
@synthesize Request;
@synthesize AvailableProducts;


- (id)init
{
	self = [super init];
	return self;
}


- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::updatedTransactions" ));
    for (SKPaymentTransaction *transaction in transactions)
    {
        switch ([transaction transactionState])
        {
            case SKPaymentTransactionStatePurchased:
                [self completeTransaction:transaction];
                break;
            case SKPaymentTransactionStateFailed:
                [self failedTransaction:transaction];
                break;
            case SKPaymentTransactionStateRestored:
                [self restoreTransaction:transaction];
				break;
            case SKPaymentTransactionStatePurchasing:
                continue;
			default:
				break;
        }
    }
}


- (void) completeTransaction: (SKPaymentTransaction *)transaction
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::completeTransaction" ));

    // Your application should implement these two methods.
    //[self recordTransaction:transaction];
    //[self provideContent:transaction.payment.productIdentifier];
 
    // Remove the transaction from the payment queue.
    [[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}


- (void) restoreTransaction: (SKPaymentTransaction *)transaction
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::restoreTransaction" ));

    //[self recordTransaction: transaction];
    //[self provideContent: transaction.originalTransaction.payment.productIdentifier];
    [[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}


- (void) failedTransaction: (SKPaymentTransaction *)transaction
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::failedTransaction" ));

    if (transaction.error.code != SKErrorPaymentCancelled) {
        // Optionally, display an error here.
    }
    [[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}


- (void) requestProductData: (NSMutableSet*) productIDs
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::requestProductData" ));

	Request = [[SKProductsRequest alloc] initWithProductIdentifiers: productIDs];
	Request.delegate = self;

	[Request start];
}


- (void) makePurchase: (NSMutableSet*) productIDs
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::makePurchase" ));

	Request = [[SKProductsRequest alloc] initWithProductIdentifiers: productIDs];
	Request.delegate = self;

	[Request start];
}


- (void)productsRequest: (SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
	UE_LOG(LogOnline, Display, TEXT( "FStoreKitHelper::didReceiveResponse" ));

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get(FName(TEXT("IOS")));
	FOnlineStoreInterfaceIOS* StoreInterface = (FOnlineStoreInterfaceIOS*)OnlineSub->GetStoreInterface().Get();
	StoreInterface->ProcessProductsResponse( response );

	[request autorelease];
}

@end



////////////////////////////////////////////////////////////////////
/// FOnlineStoreInterfaceIOS implementation


FOnlineStoreInterfaceIOS::FOnlineStoreInterfaceIOS() 
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::FOnlineStoreInterfaceIOS" ));
	StoreHelper = [[FStoreKitHelper alloc] init];
	[[SKPaymentQueue defaultQueue] addTransactionObserver:StoreHelper];

	// Gather the microtransaction data dfrom the ini's
	TArray<FString> ProductIDs;
	TArray<FString> DisplayNames;
	TArray<FString> DisplayDescriptions;
	TArray<FString> DisplayPrices;
	
	GConfig->GetArray(TEXT("OnlineSubsystemIOS.Store"), TEXT("ProductIDs"),		ProductIDs,				GEngineIni);
	GConfig->GetArray(TEXT("OnlineSubsystemIOS.Store"), TEXT("DisplayNames"),	DisplayNames,			GEngineIni);
	GConfig->GetArray(TEXT("OnlineSubsystemIOS.Store"), TEXT("DisplayDescriptions"),	DisplayDescriptions,	GEngineIni);
	GConfig->GetArray(TEXT("OnlineSubsystemIOS.Store"), TEXT("DisplayPrices"),	DisplayPrices,			GEngineIni);

	// @todo: add some sanity checking and logging to make sure the user has added the same number of entries to all of the arrays
	for(int32 ProductIndex = 0; ProductIndex < ProductIDs.Num(); ProductIndex++)
	{
		FMicrotransactionPurchaseInfo Info; 

		Info.Identifier = ProductIDs[ProductIndex];
		Info.DisplayName = DisplayNames[ProductIndex];
		Info.DisplayDescription = DisplayDescriptions[ProductIndex];
		Info.DisplayPrice = DisplayPrices[ProductIndex];

		AvailableProducts.Add( Info );
	}

	bIsPurchasing = false;
	bIsProductRequestInFlight = false;
}


FOnlineStoreInterfaceIOS::~FOnlineStoreInterfaceIOS() 
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::~FOnlineStoreInterfaceIOS" ));
	[StoreHelper release];
}


bool FOnlineStoreInterfaceIOS::QueryForAvailablePurchases()
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::QueryForAvailablePurchases" ));
	bool bSentAQueryRequest = false;

	TArray<FString> ProductIDs;
	GConfig->GetArray(TEXT("OnlineSubsystemIOS.Store"), TEXT("ProductIDs"), ProductIDs, GEngineIni);
	
	if( bIsPurchasing || bIsProductRequestInFlight )
	{
		UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::BeginPurchase - cannot make a purchase whilst one is in transaction." ));
	}
	else if (ProductIDs.Num() == 0)
	{
		UE_LOG(LogOnline, Display, TEXT( "There are no product IDs configured for Microtransactions in the engine.ini" ));
	}
	else
	{
		// autoreleased NSSet to hold IDs
		NSMutableSet* ProductSet = [NSMutableSet setWithCapacity:ProductIDs.Num()];
		for (int32 ProductIdx = 0; ProductIdx < ProductIDs.Num(); ProductIdx++)
		{
			NSString* ID = [NSString stringWithCString:TCHAR_TO_UTF8( *ProductIDs[ ProductIdx ] ) encoding:NSUTF8StringEncoding];
			// convert to NSString for the set objects
			[ProductSet addObject:ID];
		}

		[StoreHelper requestProductData:ProductSet];

		bIsProductRequestInFlight = true;

		bSentAQueryRequest = true;
	}

	return bSentAQueryRequest;
}


bool FOnlineStoreInterfaceIOS::IsAllowedToMakePurchases()
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::IsAllowedToMakePurchases" ));
	bool bCanMakePurchases = [SKPaymentQueue canMakePayments];
	return bCanMakePurchases;
}


bool FOnlineStoreInterfaceIOS::BeginPurchase(int Index)
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::BeginPurchase" ));
	bool bCreatedNewTransaction = false;
	bool bIsAValidIndex = Index >= 0 && Index < AvailableProducts.Num();
	
	if( bIsPurchasing || bIsProductRequestInFlight )
	{
		UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::BeginPurchase - cannot make a purchase whilst one is in transaction." ));
	}
	else if( !bIsAValidIndex )
	{
		UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS - Index: %i is out of range." ), Index);
	}
	else if( IsAllowedToMakePurchases() )
	{
		UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS - Making a transaction of product %i" ), Index);

		NSMutableSet* ProductSet = [NSMutableSet setWithCapacity:1];
		NSString* ID = [NSString stringWithCString:TCHAR_TO_UTF8( *AvailableProducts[Index].Identifier ) encoding:NSUTF8StringEncoding];
		// convert to NSString for the set objects
		[ProductSet addObject:ID];
				
		UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS - Making a transaction of product %i(%s) - count=%i" ), Index, *AvailableProducts[Index].Identifier, [ProductSet count]);

		[StoreHelper makePurchase:ProductSet];
		
		// Flag that we are purchasing so we can manage subsequent callbacks and transaction requests
		bIsPurchasing = true;
		
		// We have created a new transaction on ths pass.
		bCreatedNewTransaction = true;
	}
	else
	{
		UE_LOG(LogOnline, Display, TEXT( "This device is not able to make purchases." ) );
	}

	return bCreatedNewTransaction;
}


bool FOnlineStoreInterfaceIOS::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreInterfaceIOS::Exec" ));
	// Ignore any execs that don't start with MT
	if (FParse::Command(&Cmd, TEXT("MT")))
	{
		if (FParse::Command(&Cmd, TEXT("q")))
		{
			QueryForAvailablePurchases();
		}
		else
		{
			int32 Index;
			if (FParse::Value(Cmd, TEXT("PURCHASE="), Index ) )
			{
				BeginPurchase( Index );
			}
		}
		return true;
	}
	return false;
}


void FOnlineStoreInterfaceIOS::ProcessProductsResponse( SKProductsResponse* Response )
{
	if( bIsPurchasing )
	{
		bool bWasSuccessful = [Response.products count] > 0;
		if( [Response.products count] == 1 )
		{
			SKProduct* Product = [Response.products objectAtIndex:0];

			NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
			[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
			[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
			[numberFormatter setLocale:Product.priceLocale];

			UE_LOG(LogOnline, Display, TEXT("Making a purchase: Product: %s, Price: %s"), ANSI_TO_TCHAR([[Product localizedTitle] cStringUsingEncoding:NSASCIIStringEncoding]), ANSI_TO_TCHAR([[numberFormatter stringFromNumber:Product.price] cStringUsingEncoding:NSASCIIStringEncoding]));

			// now that we have recently refreshed the info, we can purchase it
			SKPayment* Payment = [SKPayment paymentWithProduct:Product];
			[[SKPaymentQueue defaultQueue] addPayment:Payment];
		}
		else if( [Response.products count] > 1 )
		{
			UE_LOG(LogOnline, Display, TEXT("Wrong number of products, [%d], in the response when trying to make a single purchase"), [Response.products count]);
		}
		else
		{
			for(NSString *invalidProduct in Response.invalidProductIdentifiers)
			{
				UE_LOG(LogOnline, Display, TEXT("Problem in iTunes connect configuration for product: %s"), ANSI_TO_TCHAR( [invalidProduct cStringUsingEncoding:NSASCIIStringEncoding]));
			}

			UE_LOG(LogOnline, Display, TEXT("Wrong number of products, [%d], in the response when trying to make a single purchase"), [Response.products count]);
		}

		TriggerOnPurchaseCompleteDelegates( bWasSuccessful );

		bIsPurchasing = false;
	}
	else if( bIsProductRequestInFlight )
	{
		bool bWasSuccessful = [Response.products count] > 0;
		if( [Response.products count] == 0 && [Response.invalidProductIdentifiers count] == 0 )
		{
			UE_LOG(LogOnline, Display, TEXT("Wrong number of products [%d] in the response when trying to make a single purchase"), [Response.products count]);
		}
	
		for( SKProduct* Product in Response.products )
		{
			NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
			[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
			[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
			[numberFormatter setLocale:Product.priceLocale];
		
			UE_LOG(LogOnline, Display, TEXT(": Product: %s, Price: %s"), ANSI_TO_TCHAR([[Product localizedTitle] cStringUsingEncoding:NSASCIIStringEncoding]), ANSI_TO_TCHAR([[numberFormatter stringFromNumber:Product.price] cStringUsingEncoding:NSASCIIStringEncoding]));
			[numberFormatter release];
		}
		
		for(NSString *invalidProduct in Response.invalidProductIdentifiers)
		{
			UE_LOG(LogOnline, Display, TEXT("Problem in iTunes connect configuration for product: %s"), ANSI_TO_TCHAR( [invalidProduct cStringUsingEncoding:NSASCIIStringEncoding]));
		}

		TriggerOnQueryForAvailablePurchasesCompleteDelegates( bWasSuccessful );

		bIsProductRequestInFlight = false;
	}
}