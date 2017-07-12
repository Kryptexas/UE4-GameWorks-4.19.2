// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#import <GoogleSignIn/GoogleSignIn.h>
THIRD_PARTY_INCLUDES_END

enum class EGoogleLoginResponse : uint8
{
	/** Google Sign In SDK ok response */
	RESPONSE_OK = 0,
	/** Google Sign In  SDK user cancellation */
	RESPONSE_CANCELED = 1,
	/** Google Sign In  SDK error */
	RESPONSE_ERROR = 2,
};

inline const TCHAR* ToString(EGoogleLoginResponse Response)
{
	switch (Response)
	{
		case EGoogleLoginResponse::RESPONSE_OK:
			return TEXT("RESPONSE_OK");
		case EGoogleLoginResponse::RESPONSE_CANCELED:
			return TEXT("RESPONSE_CANCELED");
		case EGoogleLoginResponse::RESPONSE_ERROR:
			return TEXT("RESPONSE_ERROR");
	}

	return TEXT("");
}


struct FGoogleSignInData
{
	/** @return a string that prints useful debug information about this response */
	FString ToDebugString() const
	{
		if (Response == EGoogleLoginResponse::RESPONSE_OK)
		{
			return FString::Printf(TEXT("UserId: %s RealName: %s FirstName: %s LastName: %s Email: %s"),
								   *UserId,
								   *RealName,
								   *FirstName,
								   *LastName,
								   *Email);
		}
		else
		{
			return FString::Printf(TEXT("Response: %s Error: %s"),
								   ToString(Response),
								   *ErrorStr);
		}
	}

	/** Result of the sign in */
	EGoogleLoginResponse Response;
	/** Error response, if any */
	FString ErrorStr;

	/** Id associated with the user account provided by the online service during registration */
	FString UserId;
	/** Real name */
	FString RealName;
	/** First name */
	FString FirstName;
	/** Last name */
	FString LastName;
	/** JWT */
	FString IdToken;
	FDateTime IdTokenExpiresIn;
	/** Email address */
	FString Email;
	/** Access Token */
	FString AccessToken;
	FDateTime AccessTokenExpiresIn;
	/** Refresh Token */
	FString RefreshToken;
};

struct FGoogleSignOutData
{
	/** @return a string that prints useful debug information about this response */
	FString ToDebugString() const
	{
		return FString::Printf(TEXT("Response: %s Error: %s"),
							   ToString(Response),
							   *ErrorStr);
	}

	/** Result of the sign in */
	EGoogleLoginResponse Response;
	/** Error response, if any */
	FString ErrorStr;
};

/**
 * Delegate fired when Google sign in has completed
 *
 * @param SignInData
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoogleSignInComplete, const FGoogleSignInData& /* SignInData */);
typedef FOnGoogleSignInComplete::FDelegate FOnGoogleSignInCompleteDelegate;

/**
 * Delegate fired when Google sign out has completed
 *
 * @param SignOutData
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGoogleSignOutComplete, const FGoogleSignOutData& /* SignOutData */);
typedef FOnGoogleSignOutComplete::FDelegate FOnGoogleSignOutCompleteDelegate;

@interface FGoogleHelper : NSObject<GIDSignInDelegate, GIDSignInUIDelegate>
{
	FOnGoogleSignInComplete _OnSignInComplete;
	FOnGoogleSignOutComplete _OnSignOutComplete;
};

- (id) initwithClientId: (NSString*) inClientId withBasicProfile: (bool) bWithProfile;
- (void) login: (NSArray*) InScopes;
- (void) logout;

/** Add a listener to the token change event */
-(FDelegateHandle)AddOnGoogleSignInComplete: (const FOnGoogleSignInCompleteDelegate&) Delegate;
/** Add a listener to the user id change event */
-(FDelegateHandle)AddOnGoogleSignOutComplete: (const FOnGoogleSignOutCompleteDelegate&) Delegate;

@end
