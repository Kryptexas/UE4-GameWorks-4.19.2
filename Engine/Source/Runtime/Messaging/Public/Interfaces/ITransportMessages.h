// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ITransportMessages.h: Declares the ITransportMessages interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of ITransportMessages.
 */
typedef TSharedPtr<class ITransportMessages, ESPMode::ThreadSafe> ITransportMessagesPtr;

/**
 * Type definition for shared references to instances of ITransportMessages.
 */
typedef TSharedRef<class ITransportMessages, ESPMode::ThreadSafe> ITransportMessagesRef;


/**
 * Delegate type for receiving transport messages.
 *
 * The first parameter is the serialized message content.
 * The second parameter is the message attachment, if any.
 * The third parameter is the identifier of the transport node that sent the message.
 */
DECLARE_DELEGATE_ThreeParams(FOnMessageTransportMessageReceived, FArchive&, const IMessageAttachmentPtr&, const FGuid&);

/**
 * Delegate type for handling the discovery of transport nodes.
 *
 * The first parameter is the identifier of the transport node that was discovered.
 */
DECLARE_DELEGATE_OneParam(FOnMessageTransportNodeDiscovered, const FGuid&);

/**
 * Delegate type for handling disconnected or timed out transport nodes.
 *
 * Remote nodes can be lost if they disconnect from the bus or if a keep-alive
 * message was not received within the required timeout (i.e. in case of a crash).
 *
 * The first parameter is the identifier of the transport node that was lost.
 */
DECLARE_DELEGATE_OneParam(FOnMessageTransportNodeLost, const FGuid&);


/**
 * Interface for message transport technologies.
 *
 * Licensees can implement this interface to add support for custom message transport
 * technologies that are not implemented in this module, i.e. when integrating custom
 * network protocols or APIs.
 */
class ITransportMessages
{
public:

	/**
	 * Starts up the message transport.
	 *
	 * @return Whether the transport was started successfully.
	 */
	virtual bool StartTransport( ) = 0;

	/**
	 * Shuts down the message transport.
	 */
	virtual void StopTransport( ) = 0;

	/**
	 * Transports the given message data to the specified network nodes.
	 *
	 * @param Data - The serialized message data to transport.
	 * @param Attachment - An optional message attachment (i.e. file or memory buffer).
	 * @param Recipients - The transport nodes to send the message to.
	 *
	 * @return true if the message is being transported, false otherwise.
	 */
	virtual bool TransportMessage( const IMessageDataRef& Data, const IMessageAttachmentPtr& Attachment, const TArray<FGuid>& Recipients ) = 0;

public:

	/**
	 * Returns a delegate that is executed when message data has been received.
	 *
	 * @param Delegate - The delegate to set.
	 */
	virtual FOnMessageTransportMessageReceived& OnMessageReceived( ) = 0;

	/**
	 * Returns a delegate that is executed when a transport node has been discovered.
	 *
	 * @param Delegate - The delegate to set.
	 */
	virtual FOnMessageTransportNodeDiscovered& OnNodeDiscovered( ) = 0;

	/**
	 * Returns a delegate that is executed when a transport node has closed or timed out.
	 *
	 * @param Delegate - The delegate to set.
	 */
	virtual FOnMessageTransportNodeLost& OnNodeLost( ) = 0;

protected:

	ITransportMessages( ) { }
};
