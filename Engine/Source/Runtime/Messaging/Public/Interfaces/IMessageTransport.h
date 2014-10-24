// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


// forward declarations
class IMessageAttachment;
class IMessageData;


/**
 * Delegate type for receiving transport messages.
 *
 * The first parameter is the context of the received message.
 * The second parameter is the identifier of the transport node that sent the message.
 */
DECLARE_DELEGATE_TwoParams(FOnMessageTransportMessageReceived, const IMessageContextRef&, const FGuid&);

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
 * technologies that are not supported out of the box, i.e. custom network protocols or APIs.
 */
class IMessageTransport
{
public:

	/**
	 * Gets the name of this transport (for debugging purposes).
	 *
	 * @return The debug name.
	 */
	virtual FName GetDebugName() const = 0;

	/**
	 * Starts up the message transport.
	 *
	 * @return Whether the transport was started successfully.
	 * @see StopTransport
	 */
	virtual bool StartTransport() = 0;

	/**
	 * Shuts down the message transport.
	 *
	 * @see StartTransport
	 */
	virtual void StopTransport() = 0;

	/**
	 * Transports the given message data to the specified network nodes.
	 *
	 * @param Context The context of the message to transport.
	 * @param Recipients The transport nodes to send the message to.
	 * @return true if the message is being transported, false otherwise.
	 */
	virtual bool TransportMessage( const IMessageContextRef& Context, const TArray<FGuid>& Recipients ) = 0;

public:

	/**
	 * Returns a delegate that is executed when message data has been received.
	 *
	 * @param Delegate The delegate to set.
	 */
	virtual FOnMessageTransportMessageReceived& OnMessageReceived() = 0;

	/**
	 * Returns a delegate that is executed when a transport node has been discovered.
	 *
	 * @param Delegate The delegate to set.
	 */
	virtual FOnMessageTransportNodeDiscovered& OnNodeDiscovered() = 0;

	/**
	 * Returns a delegate that is executed when a transport node has closed or timed out.
	 *
	 * @param Delegate The delegate to set.
	 */
	virtual FOnMessageTransportNodeLost& OnNodeLost() = 0;

protected:

	/** Virtual constructor. */
	~IMessageTransport() { }
};


/** Type definition for shared pointers to instances of ITransportMessages. */
typedef TSharedPtr<IMessageTransport, ESPMode::ThreadSafe> IMessageTransportPtr;

/** Type definition for shared references to instances of ITransportMessages. */
typedef TSharedRef<IMessageTransport, ESPMode::ThreadSafe> IMessageTransportRef;
