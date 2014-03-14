// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemIOSPrivatePCH.h"

FOnlineIdentityIOS::FOnlineIdentityIOS() :
	UniqueNetId( NULL )
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::FOnlineIdentityIOS()"));

	Login( 0, FOnlineAccountCredentials() );
}

TSharedPtr<FUserOnlineAccount> FOnlineIdentityIOS::GetUserAccount(const FUniqueNetId& UserId) const
{
	// not implemented
	TSharedPtr<FUserOnlineAccount> Result;
	return Result;
}

TArray<TSharedPtr<FUserOnlineAccount> > FOnlineIdentityIOS::GetAllUserAccounts() const
{
	// not implemented
	TArray<TSharedPtr<FUserOnlineAccount> > Result;
	return Result;
}

bool FOnlineIdentityIOS::Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::Login"));

	bool bStartedLogin = false;
	
	if( GetLocalGameCenterUser() && 
		GetLocalGameCenterUser().isAuthenticated )
	{
		// Login was handled by gamecenter
		FString PlayerId( ANSI_TO_TCHAR( [GetLocalGameCenterUser().playerID cStringUsingEncoding:NSASCIIStringEncoding] ) );
		UniqueNetId = MakeShareable( new FUniqueNetIdString( PlayerId ) );
		TriggerOnLoginCompleteDelegates(LocalUserNum, true, *UniqueNetId, TEXT(""));
		bStartedLogin = true;
	}
	else if([IOSAppDelegate GetDelegate].OSVersion >= 6.0f)
	{
		bStartedLogin = true;
		dispatch_async(dispatch_get_main_queue(), ^
		{
			[GetLocalGameCenterUser() setAuthenticateHandler:(^(UIViewController* viewcontroller, NSError *error)
			{
				FString ErrorMessage;
				bool bWasSuccessful = false;

				if (!error && viewcontroller)
				{
					[[IOSAppDelegate GetDelegate].IOSController
					presentViewController:viewcontroller animated:YES completion:nil];
				}
				else
				{
					if (GetLocalGameCenterUser().isAuthenticated)
					{
						/* Perform additional tasks for the authenticated player here */
						FString PlayerId( ANSI_TO_TCHAR( [GetLocalGameCenterUser().playerID cStringUsingEncoding:NSASCIIStringEncoding] ) );
						UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::Player Authenticated >IOS6.0 - playerid: %s"), *PlayerId);
						UniqueNetId = MakeShareable( new FUniqueNetIdString( PlayerId ) );
						
						bWasSuccessful = true;
					}
					else
					{
						UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::Player NOT Authenticated >IOS6.0"));
						ErrorMessage = TEXT("Player NOT Authenticated");
					}
				}

				// Report back to the game thread whether this succeeded.
				[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
				{
					TSharedPtr<FUniqueNetIdString> UniqueIdForUser = UniqueNetId.IsValid() ? UniqueNetId : MakeShareable(new FUniqueNetIdString());
					TriggerOnLoginCompleteDelegates(LocalUserNum, bWasSuccessful, *UniqueIdForUser, *ErrorMessage);

					return true;
				}];
			})];
		});
	}
	else
	{
		UE_LOG(LogOnline, Verbose, TEXT("GameCenter login was unsuccessful"));

		// User is not currently logged into game center
		TriggerOnLoginCompleteDelegates(LocalUserNum, false, FUniqueNetIdString(), TEXT("Local player is invalid"));
	}
	
	return bStartedLogin;
}

bool FOnlineIdentityIOS::Logout(int32 LocalUserNum)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::Logout"));
	TriggerOnLogoutCompleteDelegates(LocalUserNum, true);
	return false;
}

bool FOnlineIdentityIOS::AutoLogin(int32 LocalUserNum)
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::AutoLogin"));
	
	return Login( LocalUserNum, FOnlineAccountCredentials() );
}

ELoginStatus::Type FOnlineIdentityIOS::GetLoginStatus(int32 LocalUserNum) const
{
	ELoginStatus::Type LoginStatus = ELoginStatus::NotLoggedIn;

	if(LocalUserNum < MAX_LOCAL_PLAYERS && GetLocalGameCenterUser() != NULL && GetLocalGameCenterUser().isAuthenticated == YES)
	{
		UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::GetLoginStatus - Logged in"));
		LoginStatus = ELoginStatus::LoggedIn;
	}
	else
	{
		UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::GetLoginStatus - Not logged in"));
	}

	return LoginStatus;
}

TSharedPtr<FUniqueNetId> FOnlineIdentityIOS::GetUniquePlayerId(int32 LocalUserNum) const
{
	if( UniqueNetId.IsValid() )
	{
		UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::GetUniquePlayerId %s"), *UniqueNetId->ToString());
	}

	return UniqueNetId;
}

TSharedPtr<FUniqueNetId> FOnlineIdentityIOS::CreateUniquePlayerId(uint8* Bytes, int32 Size)
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

TSharedPtr<FUniqueNetId> FOnlineIdentityIOS::CreateUniquePlayerId(const FString& Str)
{
	return MakeShareable(new FUniqueNetIdString(Str));
}

FString FOnlineIdentityIOS::GetPlayerNickname(int32 LocalUserNum) const
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::GetPlayerNickname"));


	if (LocalUserNum < MAX_LOCAL_PLAYERS && GetLocalGameCenterUser() != NULL)
	{
		NSString* PersonaName = [GetLocalGameCenterUser() alias];
		
        if (PersonaName != nil)
        {
            NSString* UserNameString = [NSString stringWithFormat:@"FOnlineIdentityIOS::GetPlayerNickname - %@", PersonaName];
            UE_LOG(LogOnline, Verbose, TEXT("%s"), ANSI_TO_TCHAR([UserNameString cStringUsingEncoding:NSASCIIStringEncoding]) );
            return FString(ANSI_TO_TCHAR([PersonaName cStringUsingEncoding:NSASCIIStringEncoding]));
        }
	}

	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::GetPlayerNickname was empty"));
	return FString();
}

FString FOnlineIdentityIOS::GetAuthToken(int32 LocalUserNum) const
{
	UE_LOG(LogOnline, Verbose, TEXT("FOnlineIdentityIOS::GetAuthToken not implemented"));
	FString ResultToken;
	return ResultToken;
}