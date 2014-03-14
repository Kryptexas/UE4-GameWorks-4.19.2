// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "ModuleManager.h"
#include "Online.h"
#include "TestEntitlementsInterface.h"

void FTestEntitlementsInterface::Test()
{
	IOnlineSubsystem* OSS = (IOnlineSubsystem*)IOnlineSubsystem::Get();

	EntitlementsOSS = Online::GetEntitlementsInterface(SubsystemName.Len() ? FName(*SubsystemName, FNAME_Find) : NAME_None);
	IdentityOSS = Online::GetIdentityInterface(SubsystemName.Len() ? FName(*SubsystemName, FNAME_Find) : NAME_None);
	if (IdentityOSS.IsValid())
	{
		UserId = IdentityOSS->GetUniquePlayerId(LocalUserIdx);
	}
	if (UserId.IsValid() && UserId->IsValid() &&
		EntitlementsOSS.IsValid())
	{
		// Add delegates for the various async calls
		EntitlementsOSS->AddOnCreateEntitlementsGroupCompleteDelegate(OnCreateEntitlementsGroupCompleteDelegate);
		EntitlementsOSS->AddOnGrantEntitlementsCompleteDelegate(OnGrantEntitlementsCompleteDelegate);
		EntitlementsOSS->AddOnConsumeEntitlementCompleteDelegate(OnConsumeEntitlementCompleteDelegate);
		EntitlementsOSS->AddOnEnumerateEntitlementsCompleteDelegate(OnEnumerateEntitlementsCompleteDelegate);
		EntitlementsOSS->AddOnDeleteEntitlementsGroupCompleteDelegate(OnDeleteEntitlementsGroupCompleteDelegate);

		// kick off next test
		StartNextTest();
	}
	else
	{
		UE_LOG(LogOnline, Warning,
			TEXT("Failed to get account creation service Mcp "));

		// done with the test
		FinishTest();
	}
}

/**
 * Step through the various tests that should be run and initiate the next one
 */
void FTestEntitlementsInterface::StartNextTest()
{
	// Always register user first, because this user will be used for all subsequent calls
	if (bCreateEntitlementsGroup)
	{
		EntitlementsOSS->CreateEntitlementsGroup(*UserId);
	}
	else if (bGrantEntitlement)
	{
		EntitlementsOSS->GrantEntitlement(*UserId, TEXT("entitlement1"));
	}
	else if (bGrantEntitlements)
	{
		TArray<FString> EntitlementIds;
		EntitlementIds.Add(TEXT("entitlement1"));
		EntitlementIds.Add(TEXT("entitlement2"));
		EntitlementIds.Add(TEXT("entitlement2"));
		EntitlementIds.Add(TEXT("entitlement3"));
		// Add 4 of entitlement4 (should add quantity passed in here)
		EntitlementIds.Add(TEXT("entitlement4"));
		EntitlementIds.Add(TEXT("entitlement4"));
		EntitlementIds.Add(TEXT("entitlement4"));
		EntitlementIds.Add(TEXT("entitlement4"));
		EntitlementsOSS->GrantEntitlements(*UserId, EntitlementIds);
	}
	else if (bConsumeEntitlement)
	{
		EntitlementsOSS->ConsumeEntitlement(*UserId, TEXT("entitlement4"));
	}
	else if (bEnumerateEntitlements)
	{
		EntitlementsOSS->EnumerateEntitlements(*UserId);
	}
	else if (bDeleteEntitlementsGroup)
	{
		EntitlementsOSS->DeleteEntitlementsGroup(*UserId);
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
		EntitlementsOSS->ClearOnCreateEntitlementsGroupCompleteDelegate(OnCreateEntitlementsGroupCompleteDelegate);
		EntitlementsOSS->ClearOnGrantEntitlementsCompleteDelegate(OnGrantEntitlementsCompleteDelegate);
		EntitlementsOSS->ClearOnConsumeEntitlementCompleteDelegate(OnConsumeEntitlementCompleteDelegate);
		EntitlementsOSS->ClearOnEnumerateEntitlementsCompleteDelegate(OnEnumerateEntitlementsCompleteDelegate);
		EntitlementsOSS->ClearOnDeleteEntitlementsGroupCompleteDelegate(OnDeleteEntitlementsGroupCompleteDelegate);

		if (bDeleteEntitlementsGroup == true && UserId.IsValid())
		{
			// If we haven't already tried, then kick off a request to clear the profile, but don't wait around to see if it worked
			// Not strictly needed, but nice to cleanup when possible
			EntitlementsOSS->DeleteEntitlementsGroup(*UserId);
		}
	}

	delete this;
}

void FTestEntitlementsInterface::OnCreateEntitlementsGroupComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId)
{
	UE_LOG(LogOnline, Log, TEXT("created entitlements group. Result=%s Error=[%s]"), 
		bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	// done with this part of the test
	bCreateEntitlementsGroup = false;

	// kick off next test
	if (bWasSuccessful)
	{
		StartNextTest();
	}
	else
	{
		FinishTest();
	}
}

void FTestEntitlementsInterface::OnGrantEntitlementsComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId, TArray<FEntitlement>& NewEntitlements, TArray<FEntitlement>& ModifiedEntitlements)
{
	UE_LOG(LogOnline, Log, TEXT("granted entitlements. Result=%s Error=[%s]"), 
		bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	// Same callback for single and plural grants, single is run first so turn it off first
	if (bGrantEntitlement == true)
	{
		// @todo check the returned new/modified in addition to getting the full cache to check
		// Check that the cached entitlements are correct after granting 1
		TMap<FString, FEntitlement> Entitlements;
		EntitlementsOSS->GetEntitlements(UserId, Entitlements);
		if (Entitlements.Num() != 1)
		{
			UE_LOG(LogOnline, Error, TEXT("Incorrect number of entitlement1 found after granting 1 entitlement: %d"), Entitlements.Num());
			bWasSuccessful = false;
		}
		else
		{
			FEntitlement* TestEntitlement;
			TestEntitlement = Entitlements.Find("entitlement1");
			if(TestEntitlement == NULL)
			{
				UE_LOG(LogOnline, Error, TEXT("Unable to find entitlement1 after granting 1 entitlement"));
				bWasSuccessful = false;
			}
			else
			{
				if(TestEntitlement->Quantity != 1)
				{
					UE_LOG(LogOnline, Error, TEXT("Incorrect quantity [%d] for entitlement1 after granting 1 entitlement"), TestEntitlement->Quantity);
					bWasSuccessful = false;
				}
			}
		}

		bGrantEntitlement = false;
	}
	else
	{
		// Check on the plural entitlement granting results in local cache
		TMap<FString, FEntitlement> Entitlements;
		EntitlementsOSS->GetEntitlements(UserId, Entitlements);
		if (Entitlements.Num() != 4)
		{
			UE_LOG(LogOnline, Error, TEXT("Incorrect number of entitlements found after granting multiple entitlement: %d"), Entitlements.Num());
			bWasSuccessful = false;
		}
		else
		{
			FEntitlement* TestEntitlement;
			TestEntitlement = Entitlements.Find("entitlement4");
			if(TestEntitlement == NULL)
			{
				UE_LOG(LogOnline, Error, TEXT("Unable to find entitlement4 after granting multiple entitlement"));
				bWasSuccessful = false;
			}
			else
			{
				if(TestEntitlement->Quantity != 4)
				{
					UE_LOG(LogOnline, Error, TEXT("Incorrect quantity [%d] for entitlement4 after granting multiple entitlement"), TestEntitlement->Quantity);
					bWasSuccessful = false;
				}
			}
		}

		bGrantEntitlements = false;
	}

	// kick off next test
	if (bWasSuccessful)
	{
		StartNextTest();
	}
	else
	{
		FinishTest();
	}
}

void FTestEntitlementsInterface::OnConsumeEntitlementComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId, TArray<FEntitlement>& NewEntitlements, TArray<FEntitlement>& ModifiedEntitlements)
{
	UE_LOG(LogOnline, Log, TEXT("consumed entitlement. Result=%s Error=[%s]"), 
		bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	// @todo check the returned new/modified in addition to getting the full cache to check
	// Check on the entitlement consumption results in local cache
	TMap<FString, FEntitlement> Entitlements;
	EntitlementsOSS->GetEntitlements(UserId, Entitlements);
	if (Entitlements.Num() != 4)
	{
		UE_LOG(LogOnline, Error, TEXT("Incorrect number of entitlements found after granting multiple entitlement: %d"), Entitlements.Num());
		bWasSuccessful = false;
	}
	else
	{
		FEntitlement* TestEntitlement;
		TestEntitlement = Entitlements.Find("entitlement4");
		if(TestEntitlement == NULL)
		{
			UE_LOG(LogOnline, Error, TEXT("Unable to find entitlement4 after granting multiple entitlement"));
			bWasSuccessful = false;
		}
		else
		{
			if(TestEntitlement->Quantity != 3)
			{
				UE_LOG(LogOnline, Error, TEXT("Incorrect quantity [%d] for entitlement4 after granting multiple entitlement"), TestEntitlement->Quantity);
				bWasSuccessful = false;
			}
		}
	}

	// done with this part of the test
	bConsumeEntitlement = false;

	// kick off next test
	if (bWasSuccessful)
	{
		StartNextTest();
	}
	else
	{
		FinishTest();
	}
}

void FTestEntitlementsInterface::OnEnumerateEntitlementsComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId)
{
	UE_LOG(LogOnline, Log, TEXT("enumerated entitlements. Result=%s Error=[%s]"), 
		bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	// Now check em out
	TMap<FString, FEntitlement> Entitlements;
	EntitlementsOSS->GetEntitlements(UserId, Entitlements);
	if (Entitlements.Num() != 4)
	{
		UE_LOG(LogOnline, Error, TEXT("Incorrect number of entitlements found: %d"), Entitlements.Num());
		return;
	}
	FEntitlement* TestEntitlement;
	TestEntitlement = Entitlements.Find("entitlement4");
	if(TestEntitlement == NULL)
	{
		UE_LOG(LogOnline, Error, TEXT("Unable to find entitlement4 after enumerating entitlements"));
		return;
	}
	if(TestEntitlement->Quantity != 3)
	{
		// Should be 3 because we add 4 then consume 1
		UE_LOG(LogOnline, Error, TEXT("Incorrect quantity [%d] for entitlement4 after enumerating entitlements"), TestEntitlement->Quantity);
	}

	// done with this part of the test
	bEnumerateEntitlements = false;

	// kick off next test
	if (bWasSuccessful)
	{
		StartNextTest();
	}
	else
	{
		FinishTest();
	}
}

void FTestEntitlementsInterface::OnDeleteEntitlementsGroupComplete(bool bWasSuccessful, const FString& Error, const FUniqueNetId& UserId)
{
	UE_LOG(LogOnline, Log, TEXT("deleting entitlements group. Result=%s Error=[%s]"), 
		bWasSuccessful ? TEXT("true") : TEXT("false"), *Error);

	// done with this part of the test
	bDeleteEntitlementsGroup = false;

	// kick off next test
	if (bWasSuccessful)
	{
		StartNextTest();
	}
	else
	{
		FinishTest();
	}
}