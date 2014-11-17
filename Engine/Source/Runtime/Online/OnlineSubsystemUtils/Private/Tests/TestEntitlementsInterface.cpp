// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "ModuleManager.h"
#include "TestEntitlementsInterface.h"

void FTestEntitlementsInterface::Test(UWorld* InWorld)
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(InWorld, SubsystemName.Len() ? FName(*SubsystemName, FNAME_Find) : NAME_None);

	EntitlementsOSS = OSS->GetEntitlementsInterface();
	IdentityOSS = OSS->GetIdentityInterface();
	if (IdentityOSS.IsValid())
	{
		UserId = IdentityOSS->GetUniquePlayerId(LocalUserIdx);
	}
	if (UserId.IsValid() && UserId->IsValid() &&
		EntitlementsOSS.IsValid())
	{
		// Add delegates for the various async calls
		EntitlementsOSS->AddOnQueryEntitlementsCompleteDelegate(OnQueryEntitlementsCompleteDelegate);

		// kick off next test
		StartNextTest();
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Failed to get entitlement service Mcp "));

		// done with the test
		FinishTest();
	}
}

/**
 * Step through the various tests that should be run and initiate the next one
 */
void FTestEntitlementsInterface::StartNextTest()
{
	if (bQueryEntitlements)
	{
		EntitlementsOSS->QueryEntitlements(*UserId);
	}
	else
	{
		FinishTest();
	}
}

void FTestEntitlementsInterface::FinishTest()
{
	if (EntitlementsOSS.IsValid())
	{
		// Clear delegates for the entitlements async calls
		EntitlementsOSS->ClearOnQueryEntitlementsCompleteDelegate(OnQueryEntitlementsCompleteDelegate);
	}

	delete this;
}

void FTestEntitlementsInterface::OnQueryEntitlementsComplete(bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	UE_LOG(LogOnline, Log, TEXT("enumerated entitlements. UserId=%s Result=%s Error=[%s]"), 
		*UserId.ToDebugString(), bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	// Now check em out
	TArray<TSharedRef<FOnlineEntitlement>> Entitlements;
	EntitlementsOSS->GetAllEntitlements(UserId, Entitlements);

	// kick off next test
	bQueryEntitlements = false;
	StartNextTest();
}