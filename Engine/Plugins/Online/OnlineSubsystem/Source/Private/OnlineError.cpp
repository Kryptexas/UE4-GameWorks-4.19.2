// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OnlineError.h"
#include "OnlineSubsystemTypes.h"

//static
const FString FOnlineError::GenericErrorCode = TEXT("GenericError");

FOnlineError::FOnlineError()
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(0)
{
}

FOnlineError::FOnlineError(bool bSucceededIn)
	: bSucceeded(bSucceededIn)
	, HttpResult(0)
	, NumericErrorCode(0)
{
}

FOnlineError::FOnlineError(const FString& ErrorCodeIn)
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(0)
{
	SetFromErrorCode(ErrorCodeIn);
}

FOnlineError::FOnlineError(FString&& ErrorCodeIn)
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(0)
{
	SetFromErrorCode(MoveTemp(ErrorCodeIn));
}

FOnlineError::FOnlineError(const int32 ErrorCodeIn)
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(ErrorCodeIn)
{
	SetFromErrorCode(ErrorCodeIn);
}

void FOnlineError::SetFromErrorCode(const FString& ErrorCodeIn)
{
	ErrorCode = ErrorCodeIn;
	ErrorRaw = ErrorCodeIn;
}

void FOnlineError::SetFromErrorCode(FString&& ErrorCodeIn)
{
	ErrorCode = MoveTemp(ErrorCodeIn);
	ErrorRaw = ErrorCode;
}

void FOnlineError::SetFromErrorCode(const int32 ErrorCodeIn)
{
	NumericErrorCode = ErrorCodeIn;
	ErrorCode = FString::Printf(TEXT("0x%0.8X"), NumericErrorCode);
	ErrorRaw = ErrorCode;
}

FOnlineError::FOnlineError(const FText& ErrorMessageIn)
	: bSucceeded(false)
	, HttpResult(0)
	, NumericErrorCode(INDEX_NONE)
{
	SetFromErrorMessage(ErrorMessageIn);
}

void FOnlineError::SetFromErrorMessage(const FText& ErrorMessageIn)
{
	ErrorMessage = ErrorMessageIn;
	ErrorCode = FTextInspector::GetKey(ErrorMessageIn).Get(GenericErrorCode);
	ErrorRaw = ErrorMessageIn.ToString();
}

void FOnlineError::SetFromErrorMessage(const FText& InErrorMessage, const int32 InErrorCode)
{
	ErrorMessage = InErrorMessage;
	SetFromErrorCode(InErrorCode);
}

const TCHAR* FOnlineError::ToLogString() const
{
	if (!ErrorMessage.IsEmpty())
	{
		return *ErrorMessage.ToString();
	}
	else if (!ErrorCode.IsEmpty())
	{
		return *ErrorCode;
	}
	else if (!ErrorRaw.IsEmpty())
	{
		return *ErrorRaw;
	}
	else
	{
		return TEXT("(Empty FOnlineError)");
	}
}

EOnlineServerConnectionStatus::Type FOnlineError::GetConnectionStatusFromHttpResult() const
{
	if (bSucceeded)
	{
		return EOnlineServerConnectionStatus::Connected;
	}

	switch (HttpResult)
	{
	case 0:
		// no response means we couldn't even connect
		return EOnlineServerConnectionStatus::ConnectionDropped;
	case 401:
		// No auth at all
		return EOnlineServerConnectionStatus::InvalidUser;
	case 403:
		// Auth failure
		return EOnlineServerConnectionStatus::NotAuthorized;
	case 503:
	case 504:
		// service is not avail
		return EOnlineServerConnectionStatus::ServersTooBusy;
	case 505:
		// update to supported version
		return EOnlineServerConnectionStatus::UpdateRequired;
	case 408:
	case 501:
	case 502:
		// other bad responses (ELB, etc)
		return EOnlineServerConnectionStatus::ServiceUnavailable;
	default:
		// this is a domain specific error like a 400 or 404, this will be handled by application code
		return EOnlineServerConnectionStatus::Connected;
	}
}
