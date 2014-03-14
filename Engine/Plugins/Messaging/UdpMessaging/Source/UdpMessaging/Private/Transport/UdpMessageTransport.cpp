// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessageTransport.cpp: Implements the FUdpMessageTransport class.
=============================================================================*/

#include "UdpMessagingPrivatePCH.h"


/* FUdpMessageTransport structors
 *****************************************************************************/

FUdpMessageTransport::FUdpMessageTransport( const FIPv4Endpoint& InLocalEndpoint, const FIPv4Endpoint& InMulticastEndpoint, uint8 InMulticastTtl )
	: LocalEndpoint(InLocalEndpoint)
	, MessageProcessor(nullptr)
	, MessageProcessorThread(nullptr)
	, MulticastEndpoint(InMulticastEndpoint)
	, MulticastReceiver(nullptr)
	, MulticastSocket(nullptr)
	, MulticastTtl(InMulticastTtl)
	, SocketSubsystem(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	, UnicastReceiver(nullptr)
	, UnicastSocket(nullptr)
{ }


FUdpMessageTransport::~FUdpMessageTransport( )
{
	StopTransport();
}


/* ITransportMessages interface
 *****************************************************************************/

bool FUdpMessageTransport::StartTransport( )
{
	// create network sockets
	MulticastSocket = FUdpSocketBuilder(TEXT("UdpMessageMulticastSocket"))
		.AsNonBlocking()
		.AsReusable()
		.BoundToAddress(LocalEndpoint.GetAddress())
		.BoundToPort(MulticastEndpoint.GetPort())
		.JoinedToGroup(MulticastEndpoint.GetAddress())
		.WithMulticastLoopback()
		.WithMulticastTtl(MulticastTtl);

	if (MulticastSocket == nullptr)
	{
		GLog->Logf(TEXT("UdpMessageTransport.StartTransport: Failed to create multicast socket on %s, joined to %s with TTL %i"), *LocalEndpoint.ToText().ToString(), *MulticastEndpoint.ToText().ToString(), MulticastTtl);

		return false;
	}

	UnicastSocket = FUdpSocketBuilder(TEXT("UdpMessageUnicastSocket"))
		.AsNonBlocking()
		.WithMulticastLoopback()
		.BoundToEndpoint(LocalEndpoint);

	if (UnicastSocket == nullptr)
	{
		SocketSubsystem->DestroySocket(MulticastSocket);
		MulticastSocket = nullptr;

		GLog->Logf(TEXT("UdpMessageTransport.StartTransport: Failed to create unicast socket on %s"), *LocalEndpoint.ToText().ToString());

		return false;
	}

	// adjust buffer sizes
	int32 NewSize = 0;
	MulticastSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);
	UnicastSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);

	FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);

	// initialize threads
	MessageProcessor = new FUdpMessageProcessor(UnicastSocket, FGuid::NewGuid(), MulticastEndpoint);
	MessageProcessor->OnMessageReceived().BindRaw(this, &FUdpMessageTransport::HandleProcessorMessageReceived);
	MessageProcessor->OnNodeDiscovered().BindRaw(this, &FUdpMessageTransport::HandleProcessorNodeDiscovered);
	MessageProcessor->OnNodeLost().BindRaw(this, &FUdpMessageTransport::HandleProcessorNodeLost);

	MulticastReceiver = new FUdpSocketReceiver(MulticastSocket, ThreadWaitTime, TEXT("UdpMessageMulticastReceiver"));
	MulticastReceiver->OnDataReceived().BindRaw(this, &FUdpMessageTransport::HandleSocketDataReceived);

	UnicastReceiver = new FUdpSocketReceiver(UnicastSocket, ThreadWaitTime, TEXT("UdpMessageUnicastReceiver"));
	UnicastReceiver->OnDataReceived().BindRaw(this, &FUdpMessageTransport::HandleSocketDataReceived);

	return true;
}


void FUdpMessageTransport::StopTransport( )
{
	// shut down threads
	delete MulticastReceiver;
	MulticastReceiver = nullptr;

	delete UnicastReceiver;
	UnicastReceiver = nullptr;

	delete MessageProcessor;
	MessageProcessor = nullptr;

	// destroy sockets
	if (MulticastSocket != nullptr)
	{
		SocketSubsystem->DestroySocket(MulticastSocket);
		MulticastSocket = nullptr;
	}
	
	if (UnicastSocket != nullptr)
	{
		SocketSubsystem->DestroySocket(UnicastSocket);
		UnicastSocket = nullptr;
	}
}


bool FUdpMessageTransport::TransportMessage( const IMessageDataRef& Data, const IMessageAttachmentPtr& Attachment, const TArray<FGuid>& Recipients )
{
	if (MessageProcessor == nullptr)
	{
		return false;
	}

	// published message
	if (Recipients.Num() == 0)
	{
		return MessageProcessor->EnqueueOutboundMessage(Data, FGuid());
	}

	// sent message
	for (int32 Index = 0; Index < Recipients.Num(); ++Index)
	{
		if (!MessageProcessor->EnqueueOutboundMessage(Data, Recipients[Index]))
		{
			return false;
		}
	}

	return true;
}


/* FUdpMessageTransport event handlers
 *****************************************************************************/

void FUdpMessageTransport::HandleSocketDataReceived( const FArrayReaderPtr& Data, const FIPv4Endpoint& Sender )
{
	if (MessageProcessor != nullptr)
	{
		MessageProcessor->EnqueueInboundSegment(Data, Sender);
	}
}
