// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "OnlineEntitlementsInterface.h"
#include "OnlineIdentityInterface.h"

/**
 * Class used to test the Mcp account creation/management process
 */
 class FTestEntitlementsInterface
 {
	/** The subsystem that was requested to be tested or the default if empty */
	const FString SubsystemName;
	/** Online services used by these tests */
	IOnlineIdentityPtr IdentityOSS;
	IOnlineEntitlementsPtr EntitlementsOSS;
	
	/** Delegates for callbacks of each test */
	FOnCreateEntitlementsGroupCompleteDelegate OnCreateEntitlementsGroupCompleteDelegate;
	FOnGrantEntitlementsCompleteDelegate OnGrantEntitlementsCompleteDelegate;
	FOnConsumeEntitlementCompleteDelegate OnConsumeEntitlementCompleteDelegate;
	FOnEnumerateEntitlementsCompleteDelegate OnEnumerateEntitlementsCompleteDelegate;
	FOnDeleteEntitlementsGroupCompleteDelegate OnDeleteEntitlementsGroupCompleteDelegate;

	/** toggles for whether to run each test */
	bool bRegisterUser;
	bool bCreateEntitlementsGroup;
	bool bEnumerateEntitlements;
	bool bGrantEntitlement;
	bool bConsumeEntitlement;
	bool bGrantEntitlements;
	bool bDeleteEntitlementsGroup;

	/** Account registered by the test that should be used for all tests */
	TSharedPtr<FUniqueNetId> UserId;
	/** Local user to run tests for */
	int32 LocalUserIdx;

	/** Hidden on purpose */
	FTestEntitlementsInterface()
		: SubsystemName()
		, LocalUserIdx(0)
	{
	}

	/**
	 * Step through the various tests that should be run and initiate the next one
	 */
	void StartNextTest();

	/**
	 * Finish/cleanup the tests
	 */
	void FinishTest();

	/**
	 * Called when the async task for creating an entitlements group has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition if any
	 * @param UserId of the user who was granted entitlements in this callback
	 */
	void OnCreateEntitlementsGroupComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId);

	/**
	 * Called when the async task for deleting an entitlements group has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition if any
	 * @param UserId of the user who was granted entitlements in this callback
	 */
	void OnDeleteEntitlementsGroupComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId);

	/**
	 * Called when the async task for enumerating entitlements has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition if any
	 * @param UserId of the user who was granted entitlements in this callback
	 */
	void OnEnumerateEntitlementsComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId);

	/**
	 * Called when the async task for granting entitlements has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition if any
	 * @param UserId of the user who was granted entitlements in this callback
	 * @param NewEntitlements new entitlements as a result of this call
	 * @param ModifiedEntitlements an array of entitlements that were changed as a result of this call
	 */
	void OnGrantEntitlementsComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId, TArray<FEntitlement>& NewEntitlements, TArray<FEntitlement>& ModifiedEntitlements);

	/**
	 * Called when the async task for consuming an entitlement has completed
	 *
	 * @param bWasSuccessful true if server was contacted and a valid result received
	 * @param Error string representing the error condition if any
	 * @param UserId of the user who was granted entitlements in this callback
	 * @param NewEntitlements new entitlements as a result of this call
	 * @param ModifiedEntitlements an array of entitlements that were changed as a result of this call
	 */
	void OnConsumeEntitlementComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId, TArray<FEntitlement>& NewEntitlements, TArray<FEntitlement>& ModifiedEntitlements);

 public:
	/**
	 * Constructor
	 *
	 */
	FTestEntitlementsInterface(const FString& InSubsystemName)
		: SubsystemName(InSubsystemName)
		, IdentityOSS(NULL)
		, EntitlementsOSS(NULL)
		, OnCreateEntitlementsGroupCompleteDelegate(FOnCreateEntitlementsGroupCompleteDelegate::CreateRaw(this, &FTestEntitlementsInterface::OnCreateEntitlementsGroupComplete))
		, OnGrantEntitlementsCompleteDelegate(FOnGrantEntitlementsCompleteDelegate::CreateRaw(this, &FTestEntitlementsInterface::OnGrantEntitlementsComplete))
		, OnConsumeEntitlementCompleteDelegate(FOnConsumeEntitlementCompleteDelegate::CreateRaw(this, &FTestEntitlementsInterface::OnConsumeEntitlementComplete))
		, OnEnumerateEntitlementsCompleteDelegate(FOnEnumerateEntitlementsCompleteDelegate::CreateRaw(this, &FTestEntitlementsInterface::OnEnumerateEntitlementsComplete))
		, OnDeleteEntitlementsGroupCompleteDelegate(FOnDeleteEntitlementsGroupCompleteDelegate::CreateRaw(this, &FTestEntitlementsInterface::OnDeleteEntitlementsGroupComplete))
		, bRegisterUser(true)
		, bCreateEntitlementsGroup(true)
		, bEnumerateEntitlements(true)
		, bGrantEntitlement(true)
		, bConsumeEntitlement(true)
		, bGrantEntitlements(true)
		, bDeleteEntitlementsGroup(true)
		, LocalUserIdx(0)
	{
	}

	/**
	 * Kicks off all of the testing process
	 */
	void Test();
 };
