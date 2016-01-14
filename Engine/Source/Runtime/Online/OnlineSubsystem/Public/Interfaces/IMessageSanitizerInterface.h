// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

DECLARE_DELEGATE_OneParam(FOnMessageProcessed, const FString& /*SanitizedMessage*/);

struct FSanitizeMessage
{
	FSanitizeMessage(FString InRawMessage, FOnMessageProcessed InProcessCompleteDelegate)
		: RawMessage(InRawMessage)
		, CompletionDelegate(InProcessCompleteDelegate)
	{}

	FString RawMessage;
	FOnMessageProcessed CompletionDelegate;
};

class IMessageSanitizer
{
protected:
	IMessageSanitizer() {};

public:
	virtual ~IMessageSanitizer() {};
	virtual void SanitizeMessage(FString Message, FOnMessageProcessed CompletionDelegate) = 0;
};

typedef TSharedPtr<IMessageSanitizer, ESPMode::ThreadSafe> IMessageSanitizerPtr;

