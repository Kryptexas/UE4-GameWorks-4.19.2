// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IAuthorizeMessageRecipients.h: Declares the IAuthorizeMessageRecipients interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IAuthorizeMessageRecipients.
 */
typedef TSharedPtr<class IAuthorizeMessageRecipients> IAuthorizeMessageRecipientsPtr;

/**
 * Type definition for shared references to instances of IAuthorizeMessageRecipients.
 */
typedef TSharedRef<class IAuthorizeMessageRecipients> IAuthorizeMessageRecipientsRef;


/**
 * Interface for classes that authorize message subscriptions.
 */
class IAuthorizeMessageRecipients
{
public:

	/**
	 * Authorizes a request to intercept messages of the specified type.
	 *
	 * @param Interceptor - The message interceptor to authorize.
	 * @param MessageType - The type of messages to intercept.
	 *
	 * @return true if the request was authorized, false otherwise.
	 */
	virtual bool AuthorizeInterceptor( const IInterceptMessagesRef& Interceptor, const FName& MessageType ) = 0;

	/**
	 * Authorizes a request to register the specified recipient.
	 *
	 * @param Recipient - The recipient to register.
	 * @param Address - The recipient's address.
	 *
	 * @return true if the request was authorized, false otherwise.
	 */
	virtual bool AuthorizeRegistration( const IReceiveMessagesRef& Recipient, const FMessageAddress& Address ) = 0;

	/**
	 * Authorizes a request to add a subscription for the specified topic pattern.
	 *
	 * @param Subscriber - The subscriber.
	 * @param TopicPattern - The message topic pattern to subscribe to.
	 *
	 * @return true if the request is authorized, false otherwise.
	 */
	virtual bool AuthorizeSubscription( const IReceiveMessagesRef& Subscriber, const FName& TopicPattern ) = 0;

	/**
	 * Authorizes a request to unregister the specified recipient.
	 *
	 * @param Address - The address of the recipient to unregister.
	 *
	 * @return true if the request was authorized, false otherwise.
	 */
	virtual bool AuthorizeUnregistration( const FMessageAddress& Address ) = 0;

	/**
	 * Authorizes a request to remove a subscription for the specified topic pattern.
	 *
	 * @param Subscriber - The subscriber.
	 * @param TopicPattern - The message topic pattern to unsubscribe from.
	 *
	 * @return true if the request is authorized, false otherwise.
	 */
	virtual bool AuthorizeUnsubscription( const IReceiveMessagesRef& Subscriber, const FName& TopicPattern ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IAuthorizeMessageRecipients( ) { }
};
