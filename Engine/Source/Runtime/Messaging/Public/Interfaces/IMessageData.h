// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Enumerates message data states.
 */
enum class EMessageDataState
{
	/** The message data is complete. */
	Complete,

	/** The message data is incomplete. */
	Incomplete,

	/** The message data is invalid. */
	Invalid
};


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
	virtual FArchive* CreateReader() = 0;

	/**
	 * Gets the state of the message data.
	 *
	 * @return Message data state.
	 * @see OnStateChanged
	 */
	virtual EMessageDataState GetState() const = 0;

public:

	/**
	 * Returns a delegate that is executed when the message data's state changed.
	 *
	 * @return The delegate.
	 * @see GetState
	 */
	virtual FSimpleDelegate& OnStateChanged() = 0;

public:

	/** Virtual destructor. */
	virtual ~IMessageData() { }
};


/** Type definition for shared pointers to instances of IMessageData. */
typedef TSharedPtr<IMessageData, ESPMode::ThreadSafe> IMessageDataPtr;

/** Type definition for shared references to instances of IMessageData. */
typedef TSharedRef<IMessageData, ESPMode::ThreadSafe> IMessageDataRef;
