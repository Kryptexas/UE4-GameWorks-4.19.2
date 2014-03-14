// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageBridge.h: Declares the IMessageBridge interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageBridge.
 */
typedef TSharedPtr<class IMessageBridge, ESPMode::ThreadSafe> IMessageBridgePtr;

/**
 * Type definition for shared references to instances of IMessageBridge.
 */
typedef TSharedRef<class IMessageBridge, ESPMode::ThreadSafe> IMessageBridgeRef;


/**
 * Interface for message bridges.
 */
class IMessageBridge
{
public:

	/**
	 * Disables this bridge.
	 *
	 * A disabled bridge will not receive any subscribed messages until it is enabled again.
	 * Bridges should be created in a disabled state by default and explicitly enabled.
	 *
	 * @see Enable
	 * @see IsEnabled
	 */
	virtual void Disable( ) = 0;

	/**
	 * Enables this bridge.
	 *
	 * An activated bridge will receive subscribed messages.
	 * Bridges should be created in a disabled state by default and explicitly enabled.
	 *
	 * @see Disable
	 * @see IsEnabled
	 */
	virtual void Enable( ) = 0;

	/**
	 * Checks whether the bridge is currently enabled.
	 *
	 * @return true if the bridge is enabled, false otherwise.
	 *
	 * @see Disable
	 * @see Enable
	 */
	virtual bool IsEnabled( ) const = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageBridge( ) { }
};
