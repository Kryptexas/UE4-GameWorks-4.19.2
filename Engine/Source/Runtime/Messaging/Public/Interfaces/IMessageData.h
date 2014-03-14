// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageAttachment.h: Declares the IMessageAttachment interface.
=============================================================================*/

#pragma once


namespace EMessageDataState
{
	/**
	 * Enumerates message data states.
	 */
	enum Type
	{
		/**
		 * The message data is complete.
		 */
		Complete,

		/**
		 * The message data is incomplete.
		 */
		Incomplete,

		/**
		 * The message data is invalid.
		 */
		Invalid
	};
}


/**
 * Type definition for shared pointers to instances of IMessageData.
 */
typedef TSharedPtr<class IMessageData, ESPMode::ThreadSafe> IMessageDataPtr;

/**
 * Type definition for shared references to instances of IMessageData.
 */
typedef TSharedRef<class IMessageData, ESPMode::ThreadSafe> IMessageDataRef;


/**
 * Interface for serialized message data.
 */
class IMessageData
{
public:

	/**
	 * Creates an archive reader to the data.
	 *
	 * The caller is responsible for deleting the returned object.
	 *
	 * @return An archive reader.
	 */
	virtual FArchive* CreateReader( ) = 0;

	/**
	 * Gets the state of the message data.
	 *
	 * @return Message data state.
	 */
	virtual EMessageDataState::Type GetState( ) const = 0;

public:

	/**
	 * Returns a delegate that is executed when the message data's state changed.
	 *
	 * @return The delegate.
	 */
	virtual FSimpleDelegate& OnStateChanged( ) = 0;
};
