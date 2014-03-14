// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IReceiveMessages.h: Declares the IReceiveMessages interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IReceiveMessages.
 */
typedef TSharedPtr<class IReceiveMessages, ESPMode::ThreadSafe> IReceiveMessagesPtr;

/**
 * Type definition for shared references to instances of IReceiveMessages.
 */
typedef TSharedRef<class IReceiveMessages, ESPMode::ThreadSafe> IReceiveMessagesRef;

/**
 * Type definition for shared pointers to instances of IReceiveMessages.
 */
typedef TWeakPtr<class IReceiveMessages, ESPMode::ThreadSafe> IReceiveMessagesWeakPtr;


/**
 * Interface for message recipients.
 *
 * This is a low-level interface for message recipients that are subscribed to a message bus.
 * For more convenient messaging functionality it is recommended to use message endpoints instead.
 *
 * @see IMessageEndpoint
 */
class IReceiveMessages
{
public:

	/**
	 * Gets the recipient's unique identifier (for debugging purposes).
	 *
	 * @return The recipient's identifier.
	 *
	 * @see GetRecipientName
	 */
	virtual const FGuid& GetRecipientId( ) const = 0;

	/**
	 * Gets the recipient's name (for debugging purposes).
	 *
	 * A single recipient can be bound to multiple message addresses, so we pass the address
	 * of interest as a parameter to this method.
	 *
	 * @param RecipientAddress - The address that the recipient is bound to.
	 *
	 * @return The name.
	 *
	 * @see GetRecipientId
	 */
	virtual FName GetRecipientName( const FMessageAddress& RecipientAddress ) const = 0;

	/**
	 * Gets the name of the thread on which to receive messages.
	 *
	 * If the recipient's ReceiveMessage() is thread-safe, return ThreadAny for best performance.
	 *
	 * @return Name of the receiving thread.
	 */
	virtual ENamedThreads::Type GetRecipientThread( ) const = 0;

	/**
	 * Checks whether this recipient represents a local endpoint.
	 *
	 * Local recipients are located in the current thread or process. Recipients located in
	 * other processes on the same machine or on remote machines are considered remote.
	 *
	 * @return true if this recipient is local, false otherwise.
	 */
	virtual bool IsLocal( ) const = 0;

	/**
	 * Handles the given message.
	 *
	 * @param Context - Will hold the context of the received message.
	 */
	virtual void ReceiveMessage( const IMessageContextRef& Context ) = 0;

public:

	/**
	 * Checks whether this recipient represents a remote endpoint.
	 *
	 * Local recipients are located in the current thread or process. Recipients located in
	 * other processes on the same machine or on remote machines are considered remote.
	 *
	 * @return true if this recipient is remote, false otherwise.
	 */
	bool IsRemote( ) const
	{
		return !IsLocal();
	}

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IReceiveMessages( ) { }
};
