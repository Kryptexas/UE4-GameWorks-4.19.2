// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "OnlineSubsystemUtilsPrivatePCH.h"
#include "OnlineIdentityInterface.h"
#include "OnlineUserInterface.h"
#include "ModuleManager.h"
#include "TestUserInterface.h"


FTestUserInterface::FTestUserInterface(const FString& InSubsystem)
	: OnlineSub(NULL)
	, bQueryUserInfo(true)
{
	UE_LOG(LogOnline, Display, TEXT("FTestUserInterface::FTestUserInterface"));
	SubsystemName = InSubsystem;
}


FTestUserInterface::~FTestUserInterface()
{
	UE_LOG(LogOnline, Display, TEXT("FTestUserInterface::~FTestUserInterface"));
}

void FTestUserInterface::Test(const TArray<FString>& InUserIds)
{
	UE_LOG(LogOnline, Display, TEXT("FTestUserInterface::Test"));

	OnlineSub = IOnlineSubsystem::Get(SubsystemName.Len() ? FName(*SubsystemName, FNAME_Find) : NAME_None);
	if (OnlineSub != NULL &&
		OnlineSub->GetIdentityInterface().IsValid() &&
		OnlineSub->GetUserInterface().IsValid())
	{
		// Add our delegate for the async call
		OnQueryUserInfoCompleteDelegate = FOnQueryUserInfoCompleteDelegate::CreateRaw(this, &FTestUserInterface::OnQueryUserInfoComplete);
		OnlineSub->GetUserInterface()->AddOnQueryUserInfoCompleteDelegate(0, OnQueryUserInfoCompleteDelegate);

		// list of users to query
		for (int32 Idx=0; Idx < InUserIds.Num(); Idx++)
		{
			TSharedPtr<FUniqueNetId> UserId = OnlineSub->GetIdentityInterface()->CreateUniquePlayerId(InUserIds[Idx]);
			if (UserId.IsValid())
			{
				QueryUserIds.Add(UserId.ToSharedRef());
			}
		}

		// kick off next test
		StartNextTest();
	}
	else
	{
		UE_LOG(LogOnline, Warning,
			TEXT("Failed to get user interface for %s"), *SubsystemName);
		
		FinishTest();
	}
}

void FTestUserInterface::StartNextTest()
{
	if (bQueryUserInfo)
	{
		OnlineSub->GetUserInterface()->QueryUserInfo(0, QueryUserIds);
	}
	else
	{
		FinishTest();
	}
}

void FTestUserInterface::FinishTest()
{
	if (OnlineSub != NULL &&
		OnlineSub->GetUserInterface().IsValid())
	{
		// Clear delegates for the various async calls
		OnlineSub->GetUserInterface()->ClearOnQueryUserInfoCompleteDelegate(0, OnQueryUserInfoCompleteDelegate);
	}
	delete this;
}

void FTestUserInterface::OnQueryUserInfoComplete(int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<class FUniqueNetId> >& UserIds, const FString& ErrorStr)
{
	UE_LOG(LogOnline, Log,
		TEXT("GetUserInterface() for player (%d) was success=%d"), LocalPlayer, bWasSuccessful);

	for (int32 UserIdx=0; UserIdx < UserIds.Num(); UserIdx++)
	{
		TSharedPtr<FOnlineUser> User = OnlineSub->GetUserInterface()->GetUserInfo(LocalPlayer, *UserIds[UserIdx]);
		if (User.IsValid())
		{
			UE_LOG(LogOnline, Log,
				TEXT("PlayerId=%s found"), *UserIds[UserIdx]->ToDebugString());
			UE_LOG(LogOnline, Log,
				TEXT("	DisplayName=%s"), *User->GetDisplayName());
			UE_LOG(LogOnline, Log,
				TEXT("	RealName=%s"), *User->GetRealName());
		}
		else
		{
			UE_LOG(LogOnline, Log,
				TEXT("PlayerId=%s not found"), *UserIds[UserIdx]->ToDebugString());
		}
	}

	// done
	bQueryUserInfo = false;
	StartNextTest();
}
