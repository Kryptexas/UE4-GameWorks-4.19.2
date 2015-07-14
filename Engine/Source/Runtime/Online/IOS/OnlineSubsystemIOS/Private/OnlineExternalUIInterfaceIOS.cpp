// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

// Module includes
#include "OnlineSubsystemIOSPrivatePCH.h"
#include "OnlineIdentityInterfaceIOS.h"

FOnlineExternalUIIOS::FOnlineExternalUIIOS(FOnlineSubsystemIOS* InSubsystem)
	: Subsystem(InSubsystem)
{
	check(InSubsystem != nullptr);
}

bool FOnlineExternalUIIOS::ShowLoginUI(const int ControllerIndex, bool bShowOnlineOnly, const FOnLoginUIClosedDelegate& Delegate)
{
	FOnlineIdentityIOS* IdentityInterface = static_cast<FOnlineIdentityIOS*>(Subsystem->GetIdentityInterface().Get());
	
	check(IdentityInterface != nullptr);
	
	if (IdentityInterface->GetLocalGameCenterUser() == nullptr)
	{
		UE_LOG(LogOnline, Log, TEXT("Game Center localPlayer is null."));
		Delegate.ExecuteIfBound(nullptr, ControllerIndex);
		return true;
	}
	
	if (IdentityInterface->GetLocalGameCenterUser().isAuthenticated)
	{
		Delegate.ExecuteIfBound(IdentityInterface->GetLocalPlayerUniqueId(), ControllerIndex);
		return true;
	}
	
	// Not authenticated, set a handler
	
	// Copy the delegate so that the block can still access it when it runs.
	FOnLoginUIClosedDelegate CopiedDelegate = Delegate;
	
	// Trigger the login event on the main thread.
	dispatch_async(dispatch_get_main_queue(), ^
	{
		[IdentityInterface->GetLocalGameCenterUser() setAuthenticateHandler:(^(UIViewController* viewcontroller, NSError *error)
		{
			UE_LOG(LogOnline, Log, TEXT("Game Center authenticateHandler. viewController = %p"), viewcontroller);
			
			// The login process has completed.
			if (viewcontroller == nil)
			{
				FString ErrorMessage;
				TSharedPtr<FUniqueNetIdString> UniqueNetId;

				if (IdentityInterface->GetLocalGameCenterUser().isAuthenticated == YES)
				{
					// Trigger the completion delegate on the game thread.
					const FString PlayerId(IdentityInterface->GetLocalGameCenterUser().playerID);
					UniqueNetId = MakeShareable(new FUniqueNetIdString(PlayerId));
					UE_LOG(LogOnline, Log, TEXT("The user %s has logged into Game Center"), *PlayerId);
				}
				else
				{
					ErrorMessage = TEXT("The user could not be authenticated by Game Center");
					UE_LOG(LogOnline, Log, TEXT("%s"), *ErrorMessage);
				}

				if (error)
				{
					NSString *errstr = [error localizedDescription];
					UE_LOG(LogOnline, Warning, TEXT("Game Center login has failed. %s]"), *FString(errstr));
				}

				// Report back to the game thread whether this succeeded.
				[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
				{
					IdentityInterface->SetLocalPlayerUniqueId(UniqueNetId);
					CopiedDelegate.ExecuteIfBound(UniqueNetId, ControllerIndex);

					return true;
				}];
			}
			else
			{
				// Game Center has provided a view controller for us to login, we present it.
				[[IOSAppDelegate GetDelegate].IOSController 
					presentViewController:viewcontroller animated:YES completion:nil];
			}
		})];
	});
	
	return true;
}

bool FOnlineExternalUIIOS::ShowFriendsUI(int32 LocalUserNum)
{
	return false;
}

bool FOnlineExternalUIIOS::ShowInviteUI(int32 LocalUserNum, FName SessionMame)
{
	return false;
}

bool FOnlineExternalUIIOS::ShowAchievementsUI(int32 LocalUserNum) 
{
	// Will always show the achievements UI for the current local signed-in user
	extern CORE_API void IOSShowAchievementsUI();
	IOSShowAchievementsUI();
	return true;
}

bool FOnlineExternalUIIOS::ShowLeaderboardUI( const FString& LeaderboardName )
{
	extern CORE_API void IOSShowLeaderboardUI(const FString& CategoryName);
	IOSShowLeaderboardUI(LeaderboardName);
	return true;
}

bool FOnlineExternalUIIOS::ShowWebURL(const FString& WebURL) 
{
	return false;
}

bool FOnlineExternalUIIOS::ShowProfileUI( const FUniqueNetId& Requestor, const FUniqueNetId& Requestee, const FOnProfileUIClosedDelegate& Delegate )
{
	return false;
}

bool FOnlineExternalUIIOS::ShowAccountUpgradeUI(const FUniqueNetId& UniqueId)
{
	return false;
}
