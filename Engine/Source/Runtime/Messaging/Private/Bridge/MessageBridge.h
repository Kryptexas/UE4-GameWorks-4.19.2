// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageBridge.h: Declares the FMessageBridge class.
=============================================================================*/

#pragma once


/**
 * Implements a message bridge.
 */
class FMessageBridge
	: public TSharedFromThis<FMessageBridge, ESPMode::ThreadSafe>
	, public IMessageBridge
	, public IReceiveMessages
	, public ISendMessages
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InAddress - The address for this bridge.
	 * @param InBus - The message bus that this node is connected to.
	 * @param InSerializer - The message serializer to use.
	 * @param InTransport - The transport mechanism to use.
	 */
	FMessageBridge( const FMessageAddress InAddress, const IMessageBusRef& InBus, const ISerializeMessagesRef& InSerializer, const ITransportMessagesRef& InTransport );

	/**
	 * Destructor.
	 */
	~FMessageBridge( );

public:

	// Begin IMessageBridge interface

	virtual void Disable( ) OVERRIDE;

	virtual void Enable( ) OVERRIDE;

	virtual bool IsEnabled( ) const OVERRIDE
	{
		return Enabled;
	}

	// End IMessageBridge interface

public:

	// Begin IReceiveMessages interface

	virtual const FGuid& GetRecipientId( ) const OVERRIDE
	{
		return Id;
	}

	virtual FName GetRecipientName( const FMessageAddress& RecipientAddress ) const OVERRIDE
	{
		return *FString::Printf(TEXT("FMessageBridge %s"), *RecipientAddress.ToString(EGuidFormats::DigitsWithHyphensInParentheses));
	}

	virtual ENamedThreads::Type GetRecipientThread( ) const OVERRIDE
	{
		return ENamedThreads::AnyThread;
	}

	virtual bool IsLocal( ) const OVERRIDE
	{
		return false;
	}

	virtual void ReceiveMessage( const IMessageContextRef& Context ) OVERRIDE;

	// End IReceiveMessages interface

public:

	// Begin ISendMessages interface

	virtual FMessageAddress GetSenderAddress( ) OVERRIDE
	{
		return Address;
	}

	virtual void NotifyMessageError( const IMessageContextRef& Context, const FString& Error ) OVERRIDE;

	// End ISendMessages interface

protected:

	/**
	 * Shuts down the bridge.
	 */
	void Shutdown( );

private:

	// Callback for message bus shut downs
	void HandleMessageBusShutdown( );

	// Callback for received transport messages.
	void HandleTransportMessageReceived( FArchive& MessageData, const IMessageAttachmentPtr& Attachment, const FGuid& NodeId );

	// Callback for lost transport endpoints.
	void HandleTransportNodeLost( const FGuid& LostNodeId );

private:

	// Holds the bridge's address.
	FMessageAddress Address;

	// Holds the address book.
	FMessageAddressBook AddressBook;

	// Holds a reference to the bus that this bridge is attached to.
	IMessageBusPtr Bus;

	// Hold a flag indicating whether this endpoint is active.
	bool Enabled;

	// Holds the bridge's unique identifier (for debugging purposes).
	const FGuid Id;

	// Holds the message subscription for outbound messages.
	IMessageSubscriptionPtr MessageSubscription;

	// Holds the message serializer. 
	ISerializeMessagesPtr Serializer;

	// Holds the message transport object.
	ITransportMessagesPtr Transport;
};
