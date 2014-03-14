// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageContext.h: Declares the FMessageContext class.
=============================================================================*/

#pragma once


/**
 * Implements a message context.
 *
 * The message context holds out-of-band information for messages that allows the
 * messaging system to route and process them correctly.
 */
class FMessageContext
	: public IMutableMessageContext
{
public:

	/**
	 * Default constructor.
	 */
	FMessageContext( )
		: Message(nullptr)
		, TypeInfo(nullptr)
	{ }

	/**
	 * Creates and initializes a new message context.
	 *
	 * @param InMessage - The message payload.
	 * @param InTypeInfo - The message's type information.
	 * @param InAttachment - The binary data to attach to the message.
	 * @param InSender - The sender's address.
	 * @param InRecipients - The message recipients.
	 * @param InScope - The message scope.
	 * @param InTimeSent - The time at which the message was sent.
	 * @param InExpiration - The message's expiration time.
	 * @param InSenderThread - The name of the thread from which the message was sent.
	 */
	FMessageContext( void* InMessage, UScriptStruct* InTypeInfo, const IMessageAttachmentPtr& InAttachment, const FMessageAddress& InSender, const TArray<FMessageAddress>& InRecipients, EMessageScope::Type InScope, const FDateTime& InTimeSent, const FDateTime& InExpiration, ENamedThreads::Type InSenderThread )
		: Attachment(InAttachment)
		, Expiration(InExpiration)
		, Message(InMessage)
		, Recipients(InRecipients)
		, Scope(InScope)
		, Sender(InSender)
		, SenderThread(InSenderThread)
		, TimeSent(InTimeSent)
		, TypeInfo(InTypeInfo)
	{ }

	/**
	 * Creates and initializes a new message from an existing message context.
	 *
	 * @param InContext - The context to initialize from.
	 * @param InForwarder - The forwarder's address.
	 * @param NewRecipients - The message recipients.
	 * @param NewScope - The forwarding scope.
	 * @param InTimeForwarded - The time at which the message was forwarded.
	 * @param InSenderThread - The name of the thread from which the message was sent.
	 */
	FMessageContext( const IMessageContextRef& InContext, const FMessageAddress& InForwarder, const TArray<FMessageAddress>& NewRecipients, EMessageScope::Type NewScope, const FDateTime& InTimeForwarded, ENamedThreads::Type InSenderThread )
		: Attachment(nullptr)
		, Forwarder(InForwarder)
		, Message(nullptr)
		, OriginalContext(InContext)
		, Recipients(NewRecipients)
		, Scope(NewScope)
		, Sender(InForwarder)
		, SenderThread(InSenderThread)
		, TimeForwarded(InTimeForwarded)
		, TypeInfo(nullptr)
	{ }

	/**
	 * Destructor.
	 */
	~FMessageContext()
	{
		if ((Message != nullptr) && TypeInfo.IsValid())
		{
			TypeInfo->DestroyScriptStruct(Message);
		}		
	}

public:

	// Begin IMessageContext interface

	virtual IMessageAttachmentPtr GetAttachment( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetAttachment();
		}

		return Attachment;
	}

	virtual const FDateTime& GetExpiration( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetExpiration();
		}

		return Expiration;
	}

	virtual const FMessageAddress& GetForwarder( ) const OVERRIDE
	{
		return Forwarder;
	}

	virtual const TMap<FName, FString>& GetHeaders( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetHeaders();
		}

		return Headers;
	}

	virtual const void* GetMessage( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetMessage();
		}

		return Message;
	}

	virtual FName GetMessageType( ) const OVERRIDE
	{
		if (IsValid())
		{
			return GetMessageTypeInfo()->GetFName();
		}
		
		return NAME_None;
	}

	virtual const TWeakObjectPtr<UScriptStruct>& GetMessageTypeInfo( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetMessageTypeInfo();
		}

		return TypeInfo;
	}

	virtual IMessageContextPtr GetOriginalContext( ) const OVERRIDE
	{
		return OriginalContext;
	}

	virtual const TArray<FMessageAddress>& GetRecipients( ) const OVERRIDE
	{
		return Recipients;
	}

	virtual EMessageScope::Type GetScope( ) const OVERRIDE
	{
		return Scope;
	}

	virtual const FMessageAddress& GetSender( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetSender();
		}

		return Sender;
	}

	virtual ENamedThreads::Type GetSenderThread( ) const OVERRIDE
	{
		return SenderThread;
	}

	virtual const FDateTime& GetTimeForwarded( ) const OVERRIDE
	{
		return TimeForwarded;
	}

	virtual const FDateTime& GetTimeSent( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->GetTimeSent();
		}

		return TimeSent;
	}

	virtual bool IsForwarded( ) const OVERRIDE
	{
		return OriginalContext.IsValid();
	}

	virtual bool IsValid( ) const OVERRIDE
	{
		if (OriginalContext.IsValid())
		{
			return OriginalContext->IsValid();
		}

		return ((Message != nullptr) && TypeInfo.IsValid(false, true));
	}

	// End IMessageContext interface

public:

	// Begin IMutableMessageContext interface

	virtual void AddRecipient( const FMessageAddress& Recipient ) OVERRIDE
	{
		Recipients.Add(Recipient);
	}

	virtual void SetAttachment( const IMessageAttachmentPtr& InAttachment ) OVERRIDE
	{
		Attachment = InAttachment;
	}

	virtual void SetMessage( void* InMessage, UScriptStruct* InTypeInfo ) OVERRIDE
	{
		Message = InMessage;
		TypeInfo = InTypeInfo;
	}

	virtual void SetExpiration( const FDateTime& InExpiration ) OVERRIDE
	{
		Expiration = InExpiration;
	}

	virtual void SetHeader( const FName& Key, const FString& Value ) OVERRIDE
	{
		Headers.Add(Key, Value);
	}

	virtual void SetScope( EMessageScope::Type InScope ) OVERRIDE
	{
		Scope = InScope;
	}

	virtual void SetSender( const FMessageAddress& InSender ) OVERRIDE
	{
		Sender = InSender;
	}

	virtual void SetTimeSent( const FDateTime& InTimeSent ) OVERRIDE
	{
		TimeSent = InTimeSent;
	}

	// Begin IMutableMessageContext interface

private:

	// Holds a pointer to attached binary data.
	IMessageAttachmentPtr Attachment;

	// Holds the expiration time.
	FDateTime Expiration;

	// Holds the address of the endpoint that forwarded this message.
	FMessageAddress Forwarder;

	// Holds the optional message headers.
	TMap<FName, FString> Headers;

	// Holds the message.
	void* Message;

	// Holds the original message context.
	IMessageContextPtr OriginalContext;

	// Holds the message recipients.
	TArray<FMessageAddress> Recipients;

	// Holds the message's scope.
	EMessageScope::Type Scope;

	// Holds the sender's identifier.
	FMessageAddress Sender;

	// Holds the name of the thread from which the message was sent.
	ENamedThreads::Type SenderThread;

	// Holds the time at which the message was forwarded.
	FDateTime TimeForwarded;

	// Holds the time at which the message was sent.
	FDateTime TimeSent;

	// Holds the message's type information.
	TWeakObjectPtr<UScriptStruct> TypeInfo;
};
