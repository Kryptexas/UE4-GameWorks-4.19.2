// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageContext.h: Declares the IMessageContext interface.
=============================================================================*/

#pragma once


namespace EMessageScope
{
	/**
	 * Enumerates scopes for published messages.
	 *
	 * The scope determines to which endpoints a published message will be delivered.
	 * By default, messages will be published to everyone on the local network (Subnet),
	 * but it is often useful to restrict the group of recipients to more local scopes,
	 * or to widen it to a larger audience outside the local network.
	 *
	 * For example, if a message is to be handled only by subscribers in the same application,
	 * the scope should be set to Process. If messages need to be published between
	 * different networks (i.e. between LAN and WLAN), it should be set to Network instead.
	 *
	 * Scopes only apply to published messages. Messages that are being sent to specific
	 * recipients will always be delivered, regardless of the endpoint locations.
	 */
	enum Type
	{
		/**
		 * Deliver to subscribers in the same thread.
		 */
		Thread,

		/**
		 * Deliver to subscribers in the same process.
		 */
		Process,

		/**
		 * Deliver to subscribers on the network.
		 */
		Network,

		/**
		 * Deliver to all subscribers.
		 *
		 * Note: This must be the last value in this enumeration.
		 */
		All
	};
}


/**
 * Type definition for message endpoint identifiers.
 */
typedef FGuid FMessageAddress;

/**
 * Type definition for shared pointers to instances of IMessageContext.
 */
typedef TSharedPtr<class IMessageContext, ESPMode::ThreadSafe> IMessageContextPtr;

/**
 * Type definition for shared references to instances of IMessageContext.
 */
typedef TSharedRef<class IMessageContext, ESPMode::ThreadSafe> IMessageContextRef;

/**
 * Type definition for message scope ranges.
 */
typedef TRange<EMessageScope::Type> FMessageScopeRange;

/**
 * Type definition for message scope range bounds.
 */
typedef TRangeBound<EMessageScope::Type> FMessageScopeRangeBound;


/**
 * Interface for message contexts.
 */
class IMessageContext
{
public:

	/**
	 * Gets the message attachment, if present.
	 *
	 * @return A pointer to the message attachment, or NULL if no attachment is present.
	 */
	virtual IMessageAttachmentPtr GetAttachment( ) const = 0;

	/**
	 * Gets the date and time at which the message expires.
	 *
	 * @return Expiration time.
	 */
	virtual const FDateTime& GetExpiration( ) const = 0;

	/**
	 * Gets the message address of the endpoint that forwarded this message.
	 *
	 * @return The forwarder's address.
	 *
	 * @see IsForwarded
	 */
	virtual const FMessageAddress& GetForwarder( ) const = 0;

	/**
	 * Gets the optional message headers.
	 *
	 * @return Message header collection.
	 */
	virtual const TMap<FName, FString>& GetHeaders( ) const = 0;

	/**
	 * Gets the message object.
	 *
	 * @return The message.
	 */
	virtual const void* GetMessage( ) const = 0;

	/**
	 * Gets the name of the message type.
	 *
	 * @return Message type name.
	 */
	virtual FName GetMessageType( ) const = 0;

	/**
	 * Gets the message's type information.
	 *
	 * @return Message type information.
	 */
	virtual const TWeakObjectPtr<UScriptStruct>& GetMessageTypeInfo( ) const = 0;

	/**
	 * Returns the original message context in case the message was forwarded.
	 *
	 * @return The original message.
	 */
	virtual IMessageContextPtr GetOriginalContext( ) const = 0;

	/**
	* Gets the list of message recipients.
	*
	* @return Message recipients.
	 */
	virtual const TArray<FMessageAddress>& GetRecipients( ) const = 0;

	/**
	 * Gets the message's scope.
	 *
	 * @return Scope.
	 */
	virtual EMessageScope::Type GetScope( ) const = 0;

	/**
	 * Gets the sender's address.
	 *
	 * @return Sender address.
	 */
	virtual const FMessageAddress& GetSender( ) const = 0;

	/**
	 * Gets the name of the thread from which the message was sent.
	 *
	 * @return Sender threat name.
	 */
	virtual ENamedThreads::Type GetSenderThread( ) const = 0;

	/**
	 * Gets the time at which the message was forwarded.
	 *
	 * @return Time forwarded.
	 */
	virtual const FDateTime& GetTimeForwarded( ) const = 0;

	/**
	 * Gets the time at which the message was sent.
	 *
	 * @return Time sent.
	 */
	virtual const FDateTime& GetTimeSent( ) const = 0;

	/**
	 * Checks whether this is a forwarded message.
	 *
	 * @return true if the message was forwarded, false otherwise.
	 *
	 * @see GetForwarder
	 */
	virtual bool IsForwarded( ) const = 0;

	/**
	 * Checks whether this context is valid.
	 *
	 * @return true if the context is valid, false otherwise.
	 */
	virtual bool IsValid( ) const = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageContext( ) { }
};
