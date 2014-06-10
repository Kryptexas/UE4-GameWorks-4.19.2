// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISendMessages.h: Declares the ISendMessages interface.
=============================================================================*/

#pragma once


/** Type definition for shared pointers to instances of ISendMessages. */
typedef TSharedPtr<class ISendMessages, ESPMode::ThreadSafe> ISendMessagesPtr;

/** Type definition for shared references to instances of ISendMessages. */
typedef TSharedRef<class ISendMessages, ESPMode::ThreadSafe> ISendMessagesRef;


/**
 * Interface for message senders.
 *
 * Classes that implement this interface are able to send messages on a message bus. Each message sender must be uniquely
 * identifiable with a message address that is returned by the @see ISendMessages.GetSenderAddress method. It is recommended
 * that implementors of this interface generate a GUID for each instance, which must not change throughout the lifetime of
 * the instance.
 *
 * The sending of messages is accomplished with the @see IMessageBus.Forward, IMessageBus.Publish and IMessageBus.Send methods.
 * In case an error occurs during the sending of a message, the @see ISendMessages.NotifyMessageError method will be called.
 *
 * This interface provides a rather low-level mechanism for receiving messages. Instead of implementing it, Most users will
 * want to use an instance of @see FMessageEndpoint, which provides a much more convenient way of sending and receiving messages.
 *
 * @see FMessageEndpoint, IMessageBus, IReceiveMessages
 */
class ISendMessages
{
public:

	/**
	 * Gets the sender's address.
	 *
	 * @return The message address.
	 */
	virtual FMessageAddress GetSenderAddress( ) = 0;

	/**
	 * Notifies the sender of errors.
	 *
	 * @param Context The context of the message that generated the error.
	 * @param Error The error string.
	 */
	virtual void NotifyMessageError( const IMessageContextRef& Context, const FString& Error ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISendMessages( ) { }
};
