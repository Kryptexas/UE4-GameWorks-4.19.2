// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"

FOnlineIdentityGooglePlay::FOnlineIdentityGooglePlay(FOnlineSubsystemGooglePlay* InSubsystem)
	: bPrevLoggedIn(false)
	, bLoggedIn(false)
	, PlayerAlias("NONE")
	, MainSubsystem(InSubsystem)
	, bRegisteringUser(false)
	, bLoggingInUser(false)

{
	UE_LOG(LogOnline, Display, TEXT("FOnlineIdentityAndroid::FOnlineIdentityAndroid()"));
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityGooglePlay::GetUserAccount(const FUniqueNetId& UserId) const
{
	return nullptr;
}


TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityGooglePlay::GetAllUserAccounts() const
{
	TArray<TSharedPtr<FUserOnlineAccount> > Result;

	return Result;
}


bool FOnlineIdentityGooglePlay::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) 
{
	TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdString(TEXT("")), FString("Login not supported"));
	return false;
}


bool FOnlineIdentityGooglePlay::Logout(int32 LocalUserNum)
{
	TriggerOnLogoutCompleteDelegates(LocalUserNum, false);
	return false;
}


bool FOnlineIdentityGooglePlay::AutoLogin(int32 LocalUserNum)
{
	return false;
}


ELoginStatus::Type FOnlineIdentityGooglePlay::GetLoginStatus(int32 LocalUserNum) const
{
	ELoginStatus::Type LoginStatus = ELoginStatus::NotLoggedIn;

	if(LocalUserNum < MAX_LOCAL_PLAYERS && bLoggedIn)
	{
		UE_LOG(LogOnline, Display, TEXT("FOnlineIdentityAndroid::GetLoginStatus - Logged in"));
		LoginStatus = ELoginStatus::LoggedIn;
	}
	else
	{
		UE_LOG(LogOnline, Display, TEXT("FOnlineIdentityAndroid::GetLoginStatus - Not logged in"));
	}

	return LoginStatus;
}


TSharedPtr<FUniqueNetId> FOnlineIdentityGooglePlay::GetUniquePlayerId(int32 LocalUserNum) const
{
	TSharedPtr<FUniqueNetId> NewID = MakeShareable(new FUniqueNetIdString(""));
	return NewID;
}


TSharedPtr<FUniqueNetId> FOnlineIdentityGooglePlay::CreateUniquePlayerId(uint8* Bytes, int32 Size)
{
	if( Bytes && Size == sizeof(uint64) )
	{
		int32 StrLen = FCString::Strlen((TCHAR*)Bytes);
		if (StrLen > 0)
		{
			FString StrId((TCHAR*)Bytes);
			return MakeShareable(new FUniqueNetIdString(StrId));
		}
	}
	return NULL;
}


TSharedPtr<FUniqueNetId> FOnlineIdentityGooglePlay::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdString(Str));
}


FString FOnlineIdentityGooglePlay::GetPlayerNickname(int32 LocalUserNum) const
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineIdentityAndroid::GetPlayerNickname"));
	return PlayerAlias;
}


FString FOnlineIdentityGooglePlay::GetAuthToken(int32 LocalUserNum) const
{
	UE_LOG(LogOnline, Display, TEXT("FOnlineIdentityAndroid::GetAuthToken not implemented"));
	FString ResultToken;
	return ResultToken;
}

void FOnlineIdentityGooglePlay::Tick(float DeltaTime)
{
}
