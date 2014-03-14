// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessageBus.h: Declares the FMessageBus class.
=============================================================================*/

#pragma once


/**
 * Implements a message bus.
 */
class FMessageBus
	: public TSharedFromThis<FMessageBus, ESPMode::ThreadSafe>
	, public IMessageBus
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InRecipientAuthorizer - An optional recipient authorizer.
	 */
	FMessageBus( const IAuthorizeMessageRecipientsPtr& InRecipientAuthorizer );

	/**
	 * Destructor
	 */
	~FMessageBus( );

public:

	// Begin IMessageBus interface

	virtual void Forward( const IMessageContextRef& Context, const TArray<FMessageAddress>& Recipients, EMessageScope::Type ForwardingScope, const FTimespan& Delay, const ISendMessagesRef& Forwarder ) OVERRIDE
	{
		Router->RouteMessage(MakeShareable(new FMessageContext(Context, Forwarder->GetSenderAddress(), Recipients, ForwardingScope, FDateTime::UtcNow(), FTaskGraphInterface::Get().GetCurrentThreadIfKnown())));
	}

	virtual IMessageTracerRef GetTracer( ) OVERRIDE
	{
		return Router->GetTracer();
	}

	virtual void Intercept( const IInterceptMessagesRef& Interceptor, const FName& MessageType ) OVERRIDE
	{
		if (MessageType != NAME_None)
		{
			if (!RecipientAuthorizer.IsValid() || RecipientAuthorizer->AuthorizeInterceptor(Interceptor, MessageType))
			{
				Router->AddInterceptor(Interceptor, MessageType);
			}			
		}
	}

	virtual FOnMessageBusShutdown& OnShutdown( ) OVERRIDE
	{
		return ShutdownDelegate;
	}

	virtual void Publish( void* Message, UScriptStruct* TypeInfo, EMessageScope::Type Scope, const FTimespan& Delay, const FDateTime& Expiration, const ISendMessagesRef& Publisher ) OVERRIDE
	{
		Router->RouteMessage(MakeShareable(new FMessageContext(Message, TypeInfo, nullptr, Publisher->GetSenderAddress(), TArray<FMessageAddress>(), Scope, FDateTime::UtcNow() + Delay, Expiration, FTaskGraphInterface::Get().GetCurrentThreadIfKnown())));
	}

	virtual void Register( const FMessageAddress& Address, const IReceiveMessagesRef& Recipient ) OVERRIDE
	{
		Router->AddRecipient(Address, Recipient);
	}

	virtual void Send( void* Message, UScriptStruct* TypeInfo, const IMessageAttachmentPtr& Attachment, const TArray<FMessageAddress>& Recipients, const FTimespan& Delay, const FDateTime& Expiration, const ISendMessagesRef& Sender ) OVERRIDE
	{
		Router->RouteMessage(MakeShareable(new FMessageContext(Message, TypeInfo, Attachment, Sender->GetSenderAddress(), Recipients, EMessageScope::Network, FDateTime::UtcNow() + Delay, Expiration, FTaskGraphInterface::Get().GetCurrentThreadIfKnown())));
	}

	virtual void Shutdown( ) OVERRIDE;

	virtual IMessageSubscriptionPtr Subscribe( const IReceiveMessagesRef& Subscriber, const FName& MessageType, const FMessageScopeRange& ScopeRange ) OVERRIDE
	{
		if (MessageType != NAME_None)
		{
			if (!RecipientAuthorizer.IsValid() || RecipientAuthorizer->AuthorizeSubscription(Subscriber, MessageType))
			{
				IMessageSubscriptionRef Subscription = MakeShareable(new FMessageSubscription(Subscriber, MessageType, ScopeRange));
				Router->AddSubscription(Subscription);

				return Subscription;
			}
		}

		return nullptr;
	}

	virtual void Unintercept( const IInterceptMessagesRef& Interceptor, const FName& MessageType ) OVERRIDE
	{
		if (MessageType != NAME_None)
		{
			Router->RemoveInterceptor(Interceptor, MessageType);
		}
	}

	virtual void Unregister( const FMessageAddress& Address ) OVERRIDE
	{
		if (!RecipientAuthorizer.IsValid() || RecipientAuthorizer->AuthorizeUnregistration(Address))
		{
			Router->RemoveRecipient(Address);
		}
	}

	virtual void Unsubscribe( const IReceiveMessagesRef& Subscriber, const FName& MessageType ) OVERRIDE
	{
		if (MessageType != NAME_None)
		{
			if (!RecipientAuthorizer.IsValid() || RecipientAuthorizer->AuthorizeUnsubscription(Subscriber, MessageType))
			{
				Router->RemoveSubscription(Subscriber, MessageType);
			}
		}
	}

	// End IMessageBus interface

private:

	// Holds the message router.
	FMessageRouter* Router;

	// Holds the message router thread.
	FRunnableThread* RouterThread;

	// Holds the recipient authorizer.
	IAuthorizeMessageRecipientsPtr RecipientAuthorizer;

	// Holds bus shutdown delegate.
	FOnMessageBusShutdown ShutdownDelegate;
};
