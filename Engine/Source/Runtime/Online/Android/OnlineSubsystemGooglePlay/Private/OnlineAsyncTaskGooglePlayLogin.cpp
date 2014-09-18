// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemGooglePlayPrivatePCH.h"
#include "OnlineAsyncTaskGooglePlayLogin.h"

FOnlineAsyncTaskGooglePlayLogin::FOnlineAsyncTaskGooglePlayLogin(
	FOnlineSubsystemGooglePlay* InSubsystem,
	int InPlayerId)
	: FOnlineAsyncTaskBasic(InSubsystem)
	, PlayerId(InPlayerId)
	, Status(gpg::AuthStatus::ERROR_NOT_AUTHORIZED)
{
}

void FOnlineAsyncTaskGooglePlayLogin::Finalize()
{
	Subsystem->GetIdentityGooglePlay()->OnLoginCompleted(PlayerId, Status);
}

void FOnlineAsyncTaskGooglePlayLogin::TriggerDelegates()
{
}

void FOnlineAsyncTaskGooglePlayLogin::OnAuthActionFinished(gpg::AuthOperation InOp, gpg::AuthStatus InStatus)
{
	if (InOp == AuthOperation::SIGN_IN)
	{
		if (InStatus == AuthStatus::ERROR_NOT_AUTHORIZED)
		{
			Subsystem->GetGameServices()->StartAuthorizationUI();
			return;
		}
		
		Status = InStatus;
		bWasSuccessful = Status == AuthStatus::VALID;
		bIsComplete = true;
	}
}



