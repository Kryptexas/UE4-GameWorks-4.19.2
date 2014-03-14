// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageSubscription.h: Declares the FMessageSubscription structure.
=============================================================================*/

#pragma once


/**
 * Implements a message subscription.
 *
 * Message subscriptions are used by the message router to determine where to
 * dispatch published messages to. Message subscriptions are created per message
 * type.
 */
class FMessageSubscription
	: public IMessageSubscription
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InSubscriber - The message subscriber.
	 * @param InMessageType - The type of messages to subscribe to.
	 * @param InReceivingThread - The thread on which to receive messages on.
	 * @param InScopeRange - The message scope range to subscribe to.
	 */
	FMessageSubscription( const IReceiveMessagesRef& InSubscriber, const FName& InMessageType, const FMessageScopeRange& InScopeRange )
		: Enabled(true)
		, MessageType(InMessageType)
		, ScopeRange(InScopeRange)
		, Subscriber(InSubscriber)
	{ }

public:

	// Begin IMessageSubscription interface

	virtual void Disable( ) OVERRIDE
	{
		Enabled = false;
	}

	virtual void Enable( ) OVERRIDE
	{
		Enabled = true;
	}

	virtual FName GetMessageType( ) OVERRIDE
	{
		return MessageType;
	}

	virtual const FMessageScopeRange& GetScopeRange( ) OVERRIDE
	{
		return ScopeRange;
	}

	virtual const IReceiveMessagesWeakPtr& GetSubscriber( ) OVERRIDE
	{
		return Subscriber;
	}

	virtual bool IsEnabled( ) OVERRIDE
	{
		return Enabled;
	}

	// End IMessageSubscription interface

private:

	// Holds a flag indicating whether this subscription is enabled.
	bool Enabled;

	// Holds the type of subscribed messages.
	FName MessageType;

	// Holds the range of message scopes to subscribe to.
	FMessageScopeRange ScopeRange;

	// Holds the subscriber.
	IReceiveMessagesWeakPtr Subscriber;
};
