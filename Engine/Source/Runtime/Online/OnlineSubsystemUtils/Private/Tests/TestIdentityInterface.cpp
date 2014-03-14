// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "ModuleManager.h"
#include "Online.h"
#include "TestIdentityInterface.h"

void FTestIdentityInterface::Test(const FOnlineAccountCredentials& InAccountCredentials)
{
	// Toggle the various tests to run
	bRunLoginTest = true;

	AccountCredentials = InAccountCredentials;

	OnlineIdentity = Online::GetIdentityInterface(SubsystemName.Len() ? FName(*SubsystemName, FNAME_Find) : NAME_None);
	if (OnlineIdentity.IsValid())
	{
		// Add delegates for the various async calls
		OnlineIdentity->AddOnLoginCompleteDelegate(LocalUserIdx, OnLoginCompleteDelegate);
		
		// kick off next test
		StartNextTest();
	}
	else
	{
		UE_LOG(LogOnline, Warning,
			TEXT("Failed to get online identity interface for %s"), *SubsystemName);

		// done with the test
		FinishTest();
	}
}

void FTestIdentityInterface::StartNextTest()
{
	if (bRunLoginTest)
	{
		// no external account credentials so use internal secret key for login
		OnlineIdentity->Login(LocalUserIdx, AccountCredentials);
	}
	else
	{
		// done with the test
		FinishTest();
	}
}

void FTestIdentityInterface::FinishTest()
{
	if (OnlineIdentity.IsValid())
	{
		// Clear delegates for the various async calls
		OnlineIdentity->ClearOnLoginCompleteDelegate(LocalUserIdx, OnLoginCompleteDelegate);
	}
	delete this;
}

void FTestIdentityInterface::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogOnline, Log, TEXT("Successful authenticated user. UserId=[%s] "), 
			*UserId.ToDebugString());

		// update user info for newly registered user
		UserInfo = OnlineIdentity->GetUserAccount(UserId);
	}
	else
	{
		UE_LOG(LogOnline, Warning, TEXT("Failed to authenticate new user. Error=[%s]"), 
			*Error);
	}
	// Mark test as done
	bRunLoginTest = false;
	// kick off next test
	StartNextTest();
}

