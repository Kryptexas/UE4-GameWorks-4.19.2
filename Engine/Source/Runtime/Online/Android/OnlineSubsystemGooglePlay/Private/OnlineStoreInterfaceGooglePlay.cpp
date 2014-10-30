// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "OnlineAsyncTaskGooglePlayQueryInAppPurchases.h"
#include <jni.h>


////////////////////////////////////////////////////////////////////
/// FOnlineStoreGooglePlay implementation


FOnlineStoreGooglePlay::FOnlineStoreGooglePlay(FOnlineSubsystemGooglePlay* InSubsystem)
	: Subsystem( InSubsystem )
	, CurrentQueryTask( nullptr )
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::FOnlineStoreGooglePlay" ));
	
	FString ProductKey;
	GConfig->GetString(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("GooglePlayProductKey"), ProductKey, GEngineIni);

	extern void AndroidThunkCpp_Iap_SetupIapService(const FString&);
	AndroidThunkCpp_Iap_SetupIapService(ProductKey);
}


FOnlineStoreGooglePlay::~FOnlineStoreGooglePlay()
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::~FOnlineStoreGooglePlay" ));
}


bool FOnlineStoreGooglePlay::IsAllowedToMakePurchases()
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineStoreGooglePlay::IsAllowedToMakePurchases"));

	extern bool AndroidThunkCpp_Iap_IsAllowedToMakePurchases();
	return AndroidThunkCpp_Iap_IsAllowedToMakePurchases();
}


bool FOnlineStoreGooglePlay::QueryForAvailablePurchases(const TArray<FString>& ProductIds, FOnlineProductInformationReadRef& InReadObject)
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::QueryForAvailablePurchases" ));
	
	ReadObject = InReadObject;
	ReadObject->ReadState = EOnlineAsyncTaskState::InProgress;

	TArray<bool> ConsumableFlags;
	ConsumableFlags.AddZeroed(ProductIds.Num());

	CurrentQueryTask = new FOnlineAsyncTaskGooglePlayQueryInAppPurchases(
		Subsystem,
		ProductIds,
		ConsumableFlags);
	Subsystem->QueueAsyncTask(CurrentQueryTask);

	return true;
}


extern "C" void Java_com_epicgames_ue4_GooglePlayStoreHelper_nativeQueryComplete(JNIEnv* jenv, jobject thiz, jboolean bSuccess, jobjectArray productIDs, jobjectArray titles, jobjectArray descriptions, jobjectArray prices)
{
	TArray<FInAppPurchaseProductInfo> ProvidedProductInformation;

	if (jenv && bSuccess)
	{
		jsize NumProducts = jenv->GetArrayLength(productIDs);
		jsize NumTitles = jenv->GetArrayLength(titles);
		jsize NumDescriptions = jenv->GetArrayLength(descriptions);
		jsize NumPrices = jenv->GetArrayLength(prices);

		ensure((NumProducts == NumTitles) && (NumProducts == NumDescriptions) && (NumProducts == NumPrices));

		for (jsize Idx = 0; Idx < NumProducts; Idx++)
		{
			// Build the product information strings.

			FInAppPurchaseProductInfo NewProductInfo;

			jstring NextId = (jstring)jenv->GetObjectArrayElement(productIDs, Idx);
			const char* charsId = jenv->GetStringUTFChars(NextId, 0);
			NewProductInfo.Identifier = FString(UTF8_TO_TCHAR(charsId));
			jenv->ReleaseStringUTFChars(NextId, charsId);
			jenv->DeleteLocalRef(NextId);

			jstring NextTitle = (jstring)jenv->GetObjectArrayElement(titles, Idx);
			const char* charsTitle = jenv->GetStringUTFChars(NextTitle, 0);
			NewProductInfo.DisplayName = FString(UTF8_TO_TCHAR(charsTitle));
			jenv->ReleaseStringUTFChars(NextTitle, charsTitle);
			jenv->DeleteLocalRef(NextTitle);

			jstring NextDesc = (jstring)jenv->GetObjectArrayElement(descriptions, Idx);
			const char* charsDesc = jenv->GetStringUTFChars(NextDesc, 0);
			NewProductInfo.DisplayDescription = FString(UTF8_TO_TCHAR(charsDesc));
			jenv->ReleaseStringUTFChars(NextDesc, charsDesc);
			jenv->DeleteLocalRef(NextDesc);

			jstring NextPrice = (jstring)jenv->GetObjectArrayElement(prices, Idx);
			const char* charsPrice = jenv->GetStringUTFChars(NextPrice, 0);
			NewProductInfo.DisplayPrice = FString(UTF8_TO_TCHAR(charsPrice));
			jenv->ReleaseStringUTFChars(NextPrice, charsPrice);
			jenv->DeleteLocalRef(NextPrice);

			ProvidedProductInformation.Add(NewProductInfo);

			FPlatformMisc::LowLevelOutputDebugStringf(TEXT("\nProduct Identifier: %s, Name: %s, Description: %s, Price: %s\n"),
				*NewProductInfo.Identifier,
				*NewProductInfo.DisplayName,
				*NewProductInfo.DisplayDescription,
				*NewProductInfo.DisplayPrice);
		}
	}

	if (IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get())
	{
		FOnlineStoreGooglePlay* StoreInterface = (FOnlineStoreGooglePlay*)OnlineSub->GetStoreInterface().Get();
		if (StoreInterface)
		{
			StoreInterface->ProcessQueryAvailablePurchasesResults(bSuccess, ProvidedProductInformation);
		}
	}

	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("In-App Purchase query was completed  %s\n"), bSuccess ? TEXT("successfully") : TEXT("unsuccessfully"));
}


void FOnlineStoreGooglePlay::ProcessQueryAvailablePurchasesResults(bool bInSuccessful, const TArray<FInAppPurchaseProductInfo>& AvailablePurchases)
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineStoreGooglePlay::ProcessQueryAvailablePurchasesResults"));

	if (ReadObject.IsValid())
	{
		ReadObject->ReadState = bInSuccessful ? EOnlineAsyncTaskState::Done : EOnlineAsyncTaskState::Failed;
		ReadObject->ProvidedProductInformation.Insert(AvailablePurchases, 0);
	}

	CurrentQueryTask->ProcessQueryAvailablePurchasesResults(bInSuccessful);
}


bool FOnlineStoreGooglePlay::BeginPurchase(const FString& ProductId, FOnlineInAppPurchaseTransactionRef& InPurchaseStateObject)
{
	UE_LOG(LogOnline, Display, TEXT( "FOnlineStoreGooglePlay::BeginPurchase" ));
	
	bool bCreatedNewTransaction = false;
	
	if (IsAllowedToMakePurchases())
	{
		CachedPurchaseStateObject = InPurchaseStateObject;

		extern bool AndroidThunkCpp_Iap_BeginPurchase(const FString&, const bool);
		bCreatedNewTransaction = AndroidThunkCpp_Iap_BeginPurchase(ProductId, true); // TPMB - update begin purchase to use a consumable flag.
		UE_LOG(LogOnline, Display, TEXT("Created Transaction? - %s"), 
			bCreatedNewTransaction ? TEXT("Created a transaction.") : TEXT("Failed to create a transaction."));

		if (!bCreatedNewTransaction)
		{
			UE_LOG(LogOnline, Display, TEXT("FOnlineStoreGooglePlay::BeginPurchase - Could not create a new transaction."));
			CachedPurchaseStateObject->ReadState = EOnlineAsyncTaskState::Failed;
			TriggerOnInAppPurchaseCompleteDelegates(EInAppPurchaseState::Invalid, nullptr);
		}
		else
		{
			CachedPurchaseStateObject->ReadState = EOnlineAsyncTaskState::InProgress;
		}
	}
	else
	{
		UE_LOG(LogOnline, Display, TEXT("This device is not able to make purchases."));

		InPurchaseStateObject->ReadState = EOnlineAsyncTaskState::Failed;
		TriggerOnInAppPurchaseCompleteDelegates(EInAppPurchaseState::NotAllowed, nullptr);
	}


	return bCreatedNewTransaction;
}


extern "C" void Java_com_epicgames_ue4_GooglePlayStoreHelper_nativePurchaseComplete(JNIEnv* jenv, jobject thiz, jboolean bSuccess, jstring productId, jstring receiptData)
{
	if (IOnlineSubsystem* const OnlineSub = IOnlineSubsystem::Get())
	{
		// call store implementation to process query results.
		if (FOnlineStoreGooglePlay* StoreInterface = (FOnlineStoreGooglePlay*)OnlineSub->GetStoreInterface().Get())
		{
			FString ProductId, ReceiptData;
			if (bSuccess)
			{
				const char* charsId = jenv->GetStringUTFChars(productId, 0);
				ProductId = FString(UTF8_TO_TCHAR(charsId));
				jenv->ReleaseStringUTFChars(productId, charsId);

				const char* charsReceipt = jenv->GetStringUTFChars(receiptData, 0);
				ReceiptData = FString(UTF8_TO_TCHAR(charsReceipt));
				jenv->ReleaseStringUTFChars(receiptData, charsReceipt);
			}

			StoreInterface->ProcessPurchaseResult(bSuccess, ProductId, ReceiptData);

		}
	}

	FPlatformMisc::LowLevelOutputDebugStringf(TEXT("In-App Purchase was completed  %s\n"), bSuccess ? TEXT("successfully") : TEXT("unsuccessfully"));
}


void FOnlineStoreGooglePlay::ProcessPurchaseResult(bool bInSuccessful, const FString& ProductId, const FString& InReceiptData)
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineStoreGooglePlay::ProcessPurchaseResult"));

	FGooglePlayPurchaseReceipt Receipt;
	if (ProductId.Len() > 0 && InReceiptData.Len() > 0)
	{
		Receipt.Identifier = ProductId;
		Receipt.Data = InReceiptData;
	}

	if (CachedPurchaseStateObject.IsValid())
	{
		CachedPurchaseStateObject->ReadState = EOnlineAsyncTaskState::Done;
	}

	TriggerOnInAppPurchaseCompleteDelegates(bInSuccessful ? EInAppPurchaseState::Success : EInAppPurchaseState::Failed, &Receipt);
}