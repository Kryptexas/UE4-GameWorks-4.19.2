// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IInterceptMessages.h: Declares the IInterceptMessages interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IInterceptMessages.
 */
typedef TSharedPtr<class IInterceptMessages, ESPMode::ThreadSafe> IInterceptMessagesPtr;

/**
 * Type definition for shared references to instances of IInterceptMessages.
 */
typedef TSharedRef<class IInterceptMessages, ESPMode::ThreadSafe> IInterceptMessagesRef;


/**
 * Interface for message interceptors.
 */
class IInterceptMessages
{
public:

	/**
	 * Intercepts a message before it is being passed to the message router.
	 *
	 * @param Context - The context of the message to intercept.
	 *
	 * @return true if the message was intercepted and should not be routed, false otherwise.
	 */
	virtual bool InterceptMessage( const IMessageContextRef& Context ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IInterceptMessages( ) { }
};
