// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ISerializeMessages.h: Declares the ISerializeMessages interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ISerializeMessages.
 */
typedef TSharedPtr<class ISerializeMessages, ESPMode::ThreadSafe> ISerializeMessagesPtr;

/**
 * Type definition for shared references to instances of ISerializeMessages.
 */
typedef TSharedRef<class ISerializeMessages, ESPMode::ThreadSafe> ISerializeMessagesRef;


/**
 * Interface for message serializers.
 *
 * Licensees can implement this interface to add support for custom message serialization
 * formats that are not implemented in this module.
 */
class ISerializeMessages
{
public:

	/**
	 * Deserializes a message from an archive.
	 *
	 * @param Data - The archive to serialize from.
	 * @param OutContext - Will hold the context of the deserialized message.
	 *
	 * @return true if deserialization was successful, false otherwise.
	 */
	virtual bool DeserializeMessage( FArchive& Archive, IMutableMessageContextRef& OutContext ) = 0;

	/**
	 * Serializes a message into an archive.
	 *
	 * @param Context - The context of the message to serialize.
	 * @param Archive - The archive to serialize into.
	 *
	 * @return true if serialization was successful, false otherwise.
	 */
	virtual bool SerializeMessage( const IMessageContextRef& Context, FArchive& Archive ) = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~ISerializeMessages( ) { }
};
