// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageBus.h: Declares the IMessageBus interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageBus.
 */
typedef TSharedPtr<class IMessageBus, ESPMode::ThreadSafe> IMessageBusPtr;

/**
 * Type definition for shared references to instances of IMessageBus.
 */
typedef TSharedRef<class IMessageBus, ESPMode::ThreadSafe> IMessageBusRef;

/**
 * Type definition for weak pointers to instances of IMessageBus.
 */
typedef TWeakPtr<class IMessageBus, ESPMode::ThreadSafe> IMessageBusWeakPtr;


/**
 * Delegate type for message bus shutdowns.
 */
DECLARE_MULTICAST_DELEGATE(FOnMessageBusShutdown);


/**
 * Interface for message buses.
 */
class IMessageBus
{
public:

	/**
	 * Forwards a previously received message.
	 *
	 * Messages can only be forwarded to endpoints within the same process.
	 *
	 * @param Context - The context of the message to forward.
	 * @param Recipients - The list of message recipients to forward the message to.
	 * @param NewScope - The forwarding scope.
	 * @param Delay - The deferral time.
	 * @param Forwarder - The sender that forwards the message.
	 */
	virtual void Forward( const IMessageContextRef& Context, const TArray<FMessageAddress>& Recipients, EMessageScope::Type NewScope, const FTimespan& Delay, const ISendMessagesRef& Forwarder ) = 0;

	/**
	 * Gets the message bus tracer.
	 *
	 * @return Weak pointer to the tracer object.
	 */
	virtual IMessageTracerRef GetTracer( ) = 0;

	/**
	 * Adds an interceptor for messages of the specified type.
	 *
	 * @param Interceptor - The interceptor.
	 * @param MessageType - The type of messages to intercept.
	 */
	virtual void Intercept( const IInterceptMessagesRef& Interceptor, const FName& MessageType ) = 0;

	/**
	 * Sends a message to subscribed recipients.
	 *
	 * The message is published with a default time to live of Process.
	 *
	 * @param Message - The message to publish.
	 * @param TypeInfo - The message's type information.
	 * @param Scope - The message scope.
	 * @param Delay - The delay after which to send the message.
	 * @param Expiration - The time at which the message expires.
	 * @param Publisher - The message publisher.
	 */
	virtual void Publish( void* Message, UScriptStruct* TypeInfo, EMessageScope::Type Scope, const FTimespan& Delay, const FDateTime& Expiration, const ISendMessagesRef& Publisher ) = 0;

	/**
	 * Registers a message recipient with the message bus.
	 *
	 * @param Address - The address of the recipient to register.
	 * @param Recipient - The message recipient.
	 */
	virtual void Register( const FMessageAddress& Address, const IReceiveMessagesRef& Recipient) = 0;

	/**
	 * Sends a message to multiple recipients.
	 *
	 * @param Message - The message to send.
	 * @param TypeInfo - The message's type information.
	 * @param Attachment - The binary data to attach to the message.
	 * @param Recipients - The list of message recipients.
	 * @param Delay - The delay after which to send the message.
	 * @param Expiration - The time at which the message expires.
	 * @param Sender - The message sender.
	 */
	virtual void Send( void* Message, UScriptStruct* TypeInfo, const IMessageAttachmentPtr& Attachment, const TArray<FMessageAddress>& Recipients, const FTimespan& Delay, const FDateTime& Expiration, const ISendMessagesRef& Sender ) = 0;

	/**
	 * Shuts down the message bus.
	 *
	 * @see OnShutdown
	 */
	virtual void Shutdown( ) = 0;

	/**
	 * Adds a subscription for published messages of the specified type.
	 *
	 * Subscriptions allow message consumers to receive published messages from the message bus.
	 * The returned interface can be used to query the subscription's details and its enabled state.
	 *
	 * @param Subscriber - The subscriber wishing to receive the messages.
	 * @param MessageType - The type of messages to subscribe to (NAME_All = subscribe to all message types).
	 * @param ScopeRange - The range of message scopes to include in the subscription.
	 *
	 * @return The added subscription, or NULL if the subscription failed.
	 *
	 * @see Unsubscribe
	 */
	virtual IMessageSubscriptionPtr Subscribe( const IReceiveMessagesRef& Subscriber, const FName& MessageType, const FMessageScopeRange& ScopeRange ) = 0;

	/**
	 * Removes an interceptor for messages of the specified type.
	 *
	 * @param Interceptor - The interceptor to remove.
	 * @param MessageType - The type of messages to stop intercepting.
	 */
	virtual void Unintercept( const IInterceptMessagesRef& Interceptor, const FName& MessageType ) = 0;

	/**
	 * Unregisters a message recipient from the message bus.
	 *
	 * @param Address - The address of the recipient to unregister.
	 */
	virtual void Unregister( const FMessageAddress& Address ) = 0;

	/**
	 * Cancels the specified message subscription.
	 *
	 * @param Subscriber - The subscriber wishing to stop receiving the messages.
	 * @param MessageType - The type of messages to unsubscribe from (NAME_All = all types).
	 *
	 * @see Subscribe
	 */
	virtual void Unsubscribe( const IReceiveMessagesRef& Subscriber, const FName& MessageType ) = 0;

public:

	/**
	 * Returns a delegate that is executed when the message bus is shutting down.
	 *
	 * @return The delegate.
	 *
	 * @see Shutdown
	 */
	virtual FOnMessageBusShutdown& OnShutdown( ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageBus( ) { }
};
