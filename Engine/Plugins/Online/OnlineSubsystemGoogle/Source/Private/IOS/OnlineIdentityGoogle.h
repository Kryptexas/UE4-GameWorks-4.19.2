// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once
 
#include "OnlineIdentityGoogleCommon.h"
#include "OnlineSubsystemGoogleTypes.h"
#include "OnlineAccountGoogleCommon.h"
#include "OnlineSubsystemGooglePackage.h"

class FOnlineSubsystemGoogle;

/** iOS implementation of a Google user account */
class FUserOnlineAccountGoogle : public FUserOnlineAccountGoogleCommon
{
public:

	explicit FUserOnlineAccountGoogle(const FString& InUserId = FString(), const FAuthTokenGoogle& InAuthToken = FAuthTokenGoogle())
		: FUserOnlineAccountGoogleCommon(InUserId, InAuthToken)
	{
	}

	virtual ~FUserOnlineAccountGoogle()
	{
	}
};

/**
 * Google service implementation of the online identity interface
 */
class FOnlineIdentityGoogle :
	public FOnlineIdentityGoogleCommon
{

public:
	// IOnlineIdentity
	
	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) override;
	virtual bool Logout(int32 LocalUserNum) override;

	// FOnlineIdentityGoogle

	FOnlineIdentityGoogle(FOnlineSubsystemGoogle* InSubsystem);

	/**
	 * Destructor
	 */
	virtual ~FOnlineIdentityGoogle()
	{
	}
};

typedef TSharedPtr<FOnlineIdentityGoogle, ESPMode::ThreadSafe> FOnlineIdentityGooglePtr;