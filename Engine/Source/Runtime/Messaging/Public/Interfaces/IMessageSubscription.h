// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageSubscription.h: Declares the IMessageSubscription interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageSubscription.
 */
typedef TSharedPtr<class IMessageSubscription, ESPMode::ThreadSafe> IMessageSubscriptionPtr;

/**
 * Type definition for shared references to instances of IMessageSubscription.
 */
typedef TSharedRef<class IMessageSubscription, ESPMode::ThreadSafe> IMessageSubscriptionRef;


/**
 * Interface for message subscriptions.
 */
class IMessageSubscription
{
public:

	/**
	 * Disables the subscription.
	 */
	virtual void Disable( ) = 0;

	/**
	 * Enables the subscription.
	 */
	virtual void Enable( ) = 0;

	/**
	 * Gets the type of subscribed messages.
	 *
	 * @return Message type.
	 */
	virtual FName GetMessageType( ) = 0;

	/**
	 * Gets the range of subscribed message scopes.
	 *
	 * @return Message scope range.
	 */
	virtual const FMessageScopeRange& GetScopeRange( ) = 0;

	/**
	 * Gets the subscriber.
	 *
	 * @return The subscriber.
	 */
	virtual const IReceiveMessagesWeakPtr& GetSubscriber( ) = 0;

	/**
	 * Checks whether the subscription is enabled.
	 *
	 * @return true if the subscription is enabled, false otherwise.
	 */
	virtual bool IsEnabled( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageSubscription( ) { }
};
