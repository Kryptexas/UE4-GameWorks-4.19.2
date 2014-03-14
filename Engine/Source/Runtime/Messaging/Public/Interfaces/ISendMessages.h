// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISendMessages.h: Declares the ISendMessages interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ISendMessages.
 */
typedef TSharedPtr<class ISendMessages, ESPMode::ThreadSafe> ISendMessagesPtr;

/**
 * Type definition for shared references to instances of ISendMessages.
 */
typedef TSharedRef<class ISendMessages, ESPMode::ThreadSafe> ISendMessagesRef;


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
	 * @param Context - The context of the message that generated the error.
	 * @param Error - The error string.
	 */
	virtual void NotifyMessageError( const IMessageContextRef& Context, const FString& Error ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISendMessages( ) { }
};
