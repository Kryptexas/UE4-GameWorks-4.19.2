// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"


////////////////////////////////////////////////////////////////////
/// FOnlineStoreGooglePlay implementation


FOnlineStoreGooglePlay::FOnlineStoreGooglePlay() 
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::FOnlineStoreGooglePlay" ));

}


FOnlineStoreGooglePlay::~FOnlineStoreGooglePlay() 
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::~FOnlineStoreGooglePlay" ));

}


bool FOnlineStoreGooglePlay::QueryForAvailablePurchases(const TArray<FString>& ProductIDs, FOnlineProductInformationReadRef& InReadObject)
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::QueryForAvailablePurchases" ));
	bool bSentAQueryRequest = false;

	return bSentAQueryRequest;
}


bool FOnlineStoreGooglePlay::BeginPurchase(const FString& ProductId, FOnlineInAppPurchaseTransactionRef& InPurchaseStateObject)
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::BeginPurchase" ));
	bool bCreatedNewTransaction = false;

	return bCreatedNewTransaction;
}


bool FOnlineStoreGooglePlay::IsAllowedToMakePurchases()
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::IsAllowedToMakePurchases" ));
	bool bCanMakePurchases = false;

	return bCanMakePurchases;
}