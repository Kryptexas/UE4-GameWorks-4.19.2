// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessageTransport.h: Declares the FUdpMessageTransport class.
=============================================================================*/

#pragma once


/**
 * Implements a message transport technology using an UDP network connection.
 */
class FUdpMessageTransport
	: public ITransportMessages
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InLocalEndpoint - The local IP endpoint to receive messages on.
	 * @param InMulticastEndpoint - The multicast group endpoint to transport messages to.
	 * @param InMulticastTtl - The multicast time-to-live.
	 */
	FUdpMessageTransport( const FIPv4Endpoint& InLocalEndpoint, const FIPv4Endpoint& InMulticastEndpoint, uint8 InMulticastTtl );

	/**
	 * Destructor.
	 */
	virtual ~FUdpMessageTransport( );

public:

	// Begin ITransportMessages interface

	virtual FOnMessageTransportMessageReceived& OnMessageReceived( ) OVERRIDE
	{
		return MessageReceivedDelegate;
	}

	virtual FOnMessageTransportNodeDiscovered& OnNodeDiscovered( ) OVERRIDE
	{
		return NodeDiscoveredDelegate;
	}

	virtual FOnMessageTransportNodeLost& OnNodeLost( ) OVERRIDE
	{
		return NodeLostDelegate;
	}

	virtual bool StartTransport( ) OVERRIDE;

	virtual void StopTransport( ) OVERRIDE;

	virtual bool TransportMessage( const IMessageDataRef& Data, const IMessageAttachmentPtr& Attachment, const TArray<FGuid>& Recipients ) OVERRIDE;

	// End ITransportMessages interface

private:

	// Handles received transport messages.
	void HandleProcessorMessageReceived( FArchive& MessageData, const IMessageAttachmentPtr& Attachment, const FGuid& NodeId )
	{
		MessageReceivedDelegate.ExecuteIfBound(MessageData, Attachment, NodeId);
	}

	// Handles discovered transport endpoints.
	void HandleProcessorNodeDiscovered( const FGuid& DiscoveredNodeId )
	{
		NodeDiscoveredDelegate.ExecuteIfBound(DiscoveredNodeId);
	}

	// Handles lost transport endpoints.
	void HandleProcessorNodeLost( const FGuid& LostNodeId )
	{
		NodeLostDelegate.ExecuteIfBound(LostNodeId);
	}

	// Handles received socket data.
	void HandleSocketDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Sender);

private:

	// Holds the local endpoint to receive messages on.
	FIPv4Endpoint LocalEndpoint;

	// Holds the message processor.
	FUdpMessageProcessor* MessageProcessor;

	// Holds the message processor thread.
	FRunnableThread* MessageProcessorThread;

	// Holds the multicast endpoint.
	FIPv4Endpoint MulticastEndpoint;

	// Holds the multicast socket receiver.
	FUdpSocketReceiver* MulticastReceiver;

	// Holds the multicast socket.
	FSocket* MulticastSocket;

	// Holds the multicast time to live.
	uint8 MulticastTtl;

	// Holds a pointer to the socket sub-system.
	ISocketSubsystem* SocketSubsystem;

	// Holds the unicast socket receiver.
	FUdpSocketReceiver* UnicastReceiver;

	// Holds the unicast socket.
	FSocket* UnicastSocket;

private:

	// Holds a delegate to be invoked when a message was received on the transport channel.
	FOnMessageTransportMessageReceived MessageReceivedDelegate;

	// Holds a delegate to be invoked when a network node was discovered.
	FOnMessageTransportNodeDiscovered NodeDiscoveredDelegate;

	// Holds a delegate to be invoked when a network node was lost.
	FOnMessageTransportNodeLost NodeLostDelegate;
};
