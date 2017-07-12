// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "GoogleHelper.h"
#include "OnlineSubsystemGooglePrivate.h"

#include "CoreDelegates.h"
#include "IOSAppDelegate.h"
#include "IOS/IOSAsyncTask.h"

@implementation FGoogleHelper

- (id) initwithClientId: (NSString*) inClientId withBasicProfile: (bool) bWithProfile
{
	self = [super init];
	if (inClientId && ([inClientId length] > 0))
	{
		[self printAuthStatus];

		GIDSignIn* signIn = [GIDSignIn sharedInstance];
		signIn.shouldFetchBasicProfile = bWithProfile;
		signIn.delegate = self;
		signIn.uiDelegate = self;
		signIn.clientID = inClientId;

		dispatch_async(dispatch_get_main_queue(), ^
		{
			// Try to automatically sign in the user
			[signIn signInSilently];
		});
	}
	else
	{
		UE_LOG(LogOnline, Error, TEXT("Google init missing clientId"));
	}

	return self;
}

-(FDelegateHandle)AddOnGoogleSignInComplete: (const FOnGoogleSignInCompleteDelegate&) Delegate
{
	_OnSignInComplete.Add(Delegate);
	return Delegate.GetHandle();
}

-(FDelegateHandle)AddOnGoogleSignOutComplete: (const FOnGoogleSignOutCompleteDelegate&) Delegate
{
	_OnSignOutComplete.Add(Delegate);
	return Delegate.GetHandle();
}

- (void) login: (NSArray*) InScopes
{
	[self printAuthStatus];

	dispatch_async(dispatch_get_main_queue(), ^
	{
		GIDSignIn* signIn = [GIDSignIn sharedInstance];
		[signIn setScopes: InScopes];//[[NSArray alloc] initWithArray: InScopes]];
		[signIn signIn];
	});
}

- (void) logout
{
	dispatch_async(dispatch_get_main_queue(), ^
	{
		GIDSignIn* signIn = [GIDSignIn sharedInstance];
		[signIn signOut];
	});
}

- (void)signIn:(GIDSignIn *)signIn didSignInForUser:(GIDGoogleUser *)user withError:(NSError *)error
{
	FGoogleSignInData SignInData;
	SignInData.ErrorStr = MoveTemp(FString([error localizedDescription]));

	UE_LOG(LogOnline, Display, TEXT("signIn GID:%p User:%p Error:%s"), signIn, user, *SignInData.ErrorStr);
	[self printAuthStatus];

	if (user)
	{
		SignInData.UserId = FString(user.userID);
		SignInData.IdToken = FString(user.authentication.idToken);
		SignInData.RealName = FString(user.profile.name);
		SignInData.FirstName = FString(user.profile.givenName);
		SignInData.LastName = FString(user.profile.familyName);
		SignInData.Email = FString(user.profile.email);
		// user.profile.imageURL
		SignInData.Response = EGoogleLoginResponse::RESPONSE_OK;

		SignInData.AccessToken = user.authentication.accessToken;
		SignInData.RefreshToken = user.authentication.refreshToken;

#if __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_10_0
		NSISO8601DateFormatter* dateFormatter = [[NSISO8601DateFormatter alloc] init];
		dateFormatter.formatOptions = NSISO8601DateFormatWithInternetDateTime | NSISO8601DateFormatWithDashSeparatorInDate | NSISO8601DateFormatWithColonSeparatorInTime | NSISO8601DateFormatWithTimeZone;
#else
		// see https://developer.apple.com/library/content/qa/qa1480/_index.html
		NSDateFormatter* dateFormatter = [[NSDateFormatter alloc] init];
		NSLocale* enUSPOSIXLocale = [NSLocale localeWithLocaleIdentifier:@"en_US_POSIX"];
		[dateFormatter setLocale:enUSPOSIXLocale];
		[dateFormatter setDateFormat:@"yyyy-MM-dd'T'HH:mm:ssZZZZZ"];
#endif

		NSString* accessTokenExpirationDate = [dateFormatter stringFromDate: user.authentication.accessTokenExpirationDate];
		NSString* idTokenExpirationDate = [dateFormatter stringFromDate: user.authentication.idTokenExpirationDate];

		if (!FDateTime::ParseIso8601(*FString(accessTokenExpirationDate), SignInData.AccessTokenExpiresIn))
		{
			UE_LOG(LogOnline, Display, TEXT("Failed to parse access token expiration time"));
		}

		if (!FDateTime::ParseIso8601(*FString(idTokenExpirationDate), SignInData.IdTokenExpiresIn))
		{
			UE_LOG(LogOnline, Display, TEXT("Failed to parse ID token expiration time"));
		}

		[dateFormatter release];

	//refreshTokensWithHandler:
	//getTokensWithHandler:
	}
	else if (error.code == kGIDSignInErrorCodeHasNoAuthInKeychain)
	{
		SignInData.Response = EGoogleLoginResponse::RESPONSE_OK;
	}
	else if (error.code == kGIDSignInErrorCodeCanceled)
	{
		SignInData.Response = EGoogleLoginResponse::RESPONSE_CANCELED;
	}
	else
	{
		SignInData.Response = EGoogleLoginResponse::RESPONSE_ERROR;
	}

	UE_LOG(LogOnline, Display, TEXT("SignIn: %s"), *SignInData.ToDebugString());

	[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
	{
		// Notify on the game thread
		_OnSignInComplete.Broadcast(SignInData);
		return true;
	}];
}

- (void)signIn:(GIDSignIn *)signIn didDisconnectWithUser:(GIDGoogleUser *)user withError:(NSError *)error
{
	FGoogleSignOutData SignOutData;
	SignOutData.ErrorStr = MoveTemp(FString([error localizedDescription]));

	UE_LOG(LogOnline, Display, TEXT("didDisconnectWithUser %p %p %s"), signIn, user, *SignOutData.ErrorStr);
	// User disconnected

	UE_LOG(LogOnline, Display, TEXT("SignOut: %s"), *SignOutData.ToDebugString());

	[FIOSAsyncTask CreateTaskWithBlock : ^ bool(void)
	{
		 // Notify on the game thread
		 _OnSignOutComplete.Broadcast(SignOutData);
		 return true;
	}];
}

- (void)signInWillDispatch:(GIDSignIn *)signIn error:(NSError *)error
{
	UE_LOG(LogOnline, Display, TEXT("signInWillDispatch %p %s"), signIn, *FString([error localizedDescription]));

	// Google flow has figured out how to proceed, any engine related "please wait" is no longer necessary
}

- (void)signIn:(GIDSignIn *)signIn presentViewController:(UIViewController *)viewController
{
	UE_LOG(LogOnline, Display, TEXT("presentViewController %p"), signIn);

	// Google has provided a view controller for us to login, we present it.
	[[IOSAppDelegate GetDelegate].IOSController
	 presentViewController:viewController animated: YES completion: nil];
}

- (void)signIn:(GIDSignIn *)signIn dismissViewController:(UIViewController *)viewController
{
	UE_LOG(LogOnline, Display, TEXT("dismissViewController %p"), signIn);

	// Dismiss the Google sign in view
	[viewController dismissViewControllerAnimated: YES completion: nil];
}

- (void)printAuthStatus
{
	GIDSignIn* signIn = [GIDSignIn sharedInstance];
	GIDGoogleUser* googleUser = [signIn currentUser];

	bool bHasAuth = [signIn hasAuthInKeychain];
	UE_LOG(LogOnline, Display, TEXT("HasAuth: %d"), bHasAuth);

	UE_LOG(LogOnline, Display, TEXT("Authentication:"));
	if (googleUser.authentication)
	{
		UE_LOG(LogOnline, Display, TEXT("- Access: %s"), *FString(googleUser.authentication.accessToken));
		UE_LOG(LogOnline, Display, TEXT("- Refresh: %s"), *FString(googleUser.authentication.refreshToken));
	}
	else
	{
		UE_LOG(LogOnline, Display, TEXT("- None"));
	}

	UE_LOG(LogOnline, Display, TEXT("Scopes:"));
	for (NSString* scope in signIn.scopes)
	{
		UE_LOG(LogOnline, Display, TEXT("- %s"), *FString(scope));
	}

	UE_LOG(LogOnline, Display, TEXT("User:"));
	if (googleUser)
	{
		UE_LOG(LogOnline, Display, TEXT("- UserId: %s RealName: %s FirstName: %s LastName: %s Email: %s"),
			   *FString(googleUser.userID),
			   *FString(googleUser.profile.name),
			   *FString(googleUser.profile.givenName),
			   *FString(googleUser.profile.familyName),
			   *FString(googleUser.profile.email));
			   //*FString(googleUser.authentication.idToken));
	}
	else
	{
		UE_LOG(LogOnline, Display, TEXT("- None"));
	}
}

@end
