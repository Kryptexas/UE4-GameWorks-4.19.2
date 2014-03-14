// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessageTunnel.h: Declares the FUdpMessageTunnel class.
=============================================================================*/

#pragma once


/**
 * Implements a bi-directional tunnel to send UDP messages over a TCP connection.
 */
class FUdpMessageTunnel
	: FRunnable
	, public IMessageTunnel
{
	// Structure for transport node information
	struct FNodeInfo
	{
		// Holds the connection that owns the node (only used for remote nodes).
		FUdpMessageTunnelConnectionPtr Connection;

		// Holds the node's IP endpoint (only used for local nodes).
		FIPv4Endpoint Endpoint;

		// Holds the time at which the last datagram was received.
		FDateTime LastDatagramReceivedTime;
	};

public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InLocalEndpoint - The local IP endpoint to receive messages on.
	 * @param InMulticastEndpoint - The multicast group endpoint to transport messages to.
	 */
	FUdpMessageTunnel( const FIPv4Endpoint& InUnicastEndpoint, const FIPv4Endpoint& InMulticastEndpoint )
		: Listener(NULL)
		, MulticastEndpoint(InMulticastEndpoint)
		, Stopping(false)
		, TotalInboundBytes(0)
		, TotalOutboundBytes(0)
	{
		// initialize sockets
		MulticastSocket = FUdpSocketBuilder(TEXT("UdpMessageMulticastSocket"))
			.AsNonBlocking()
			.AsReusable()
			.BoundToAddress(InUnicastEndpoint.GetAddress())
			.BoundToPort(InMulticastEndpoint.GetPort())
			.JoinedToGroup(InMulticastEndpoint.GetAddress())
			.WithMulticastLoopback()
			.WithMulticastTtl(1);

		UnicastSocket = FUdpSocketBuilder(TEXT("UdpMessageUnicastSocket"))
			.AsNonBlocking()
			.BoundToEndpoint(InUnicastEndpoint);

		int32 NewSize = 0;
		MulticastSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);
		UnicastSocket->SetReceiveBufferSize(2 * 1024 * 1024, NewSize);

		Thread = FRunnableThread::Create(this, TEXT("FUdpMessageTunnel"), false, false, 128 * 1024, TPri_AboveNormal);
	}

	/**
	 * Destructor.
	 */
	~FUdpMessageTunnel( )
	{
		if (Thread != NULL)
		{
			Thread->Kill(true);
			delete Thread;
		}

		// destroy sockets
		if (MulticastSocket != NULL)
		{
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(MulticastSocket);
			MulticastSocket = NULL;
		}
	
		if (UnicastSocket != NULL)
		{
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UnicastSocket);
			UnicastSocket = NULL;
		}
	}

public:

	// Begin FRunnable interface

	virtual bool Init( ) OVERRIDE
	{
		return true;
	}

	virtual uint32 Run( ) OVERRIDE
	{
		while (!Stopping)
		{
			CurrentTime = FDateTime::UtcNow();

			UpdateConnections();

			UdpToTcp(MulticastSocket);
			UdpToTcp(UnicastSocket);

			TcpToUdp();

			RemoveExpiredNodes(LocalNodes);
			RemoveExpiredNodes(RemoteNodes);
		}

		return 0;
	}

	virtual void Stop( ) OVERRIDE
	{
		Stopping = true;
	}

	virtual void Exit( ) OVERRIDE { }

	// End FRunnable interface

public:

	// Begin IMessageTunnel interface

	virtual bool Connect( const FIPv4Endpoint& RemoteEndpoint ) OVERRIDE
	{
		FSocket* Socket = FTcpSocketBuilder(TEXT("FUdpMessageTunnel.RemoteConnection"));

		if (Socket != NULL)
		{
			if (Socket->Connect(*RemoteEndpoint.ToInternetAddr()))
			{
				PendingConnections.Enqueue(MakeShareable(new FUdpMessageTunnelConnection(Socket, RemoteEndpoint)));

				return true;
			}
			else
			{
				ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
			}
		}

		return false;
	}

	virtual int32 GetConnections( TArray<IMessageTunnelConnectionPtr>& OutConnections ) OVERRIDE
	{
		FScopeLock Lock(&CriticalSection);

		for (TArray<FUdpMessageTunnelConnectionPtr>::TConstIterator It(Connections); It; ++It)
		{
			OutConnections.Add(*It);
		}

		return OutConnections.Num();
	}

	virtual uint64 GetTotalInboundBytes( ) const OVERRIDE
	{
		return TotalInboundBytes;
	}

	virtual uint64 GetTotalOutboundBytes( ) const OVERRIDE
	{
		return TotalOutboundBytes;
	}

	virtual bool IsServerRunning( ) const OVERRIDE
	{
		return (Listener != NULL);
	}

	virtual FSimpleDelegate& OnConnectionsChanged( ) OVERRIDE
	{
		return ConnectionsChangedDelegate;
	}

	virtual void StartServer( const FIPv4Endpoint& LocalEndpoint ) OVERRIDE
	{
		StopServer();

		Listener = new FTcpListener(LocalEndpoint);
		Listener->OnConnectionAccepted().BindRaw(this, &FUdpMessageTunnel::HandleListenerConnectionAccepted);
	}

	virtual void StopServer( ) OVERRIDE
	{
		delete Listener;
		Listener = NULL;
	}

	// End IMessageTunnel interface

protected:

	/**
	 * Removes expired nodes from the specified collection of node infos.
	 *
	 * @param Nodes - The collection of nodes to clean up.
	 */
	void RemoveExpiredNodes( TMap<FGuid, FNodeInfo>& Nodes )
	{
		for (TMap<FGuid, FNodeInfo>::TIterator It(Nodes); It; ++It)
		{
			if (CurrentTime - It.Value().LastDatagramReceivedTime > FTimespan::FromMinutes(2.0))
			{
				It.RemoveCurrent();
			}
		}
	}

	/**
	 * Receives all pending payloads from the tunnels and forwards them to the local message bus.
	 */
	void TcpToUdp( )
	{
		TArray<FUdpMessageTunnelConnectionPtr>::TIterator It(Connections);
		FArrayReaderPtr Payload;

		while (It && UnicastSocket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::Zero()))
		{
			if ((*It)->Receive(Payload))
			{
				FUdpMessageSegment::FHeader Header;
				*Payload << Header;

				// check protocol version
				if (Header.ProtocolVersion != UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION)
				{
					return;
				}

				Payload->Seek(0);

				// update remote node & statistics
				FNodeInfo& RemoteNode = RemoteNodes.FindOrAdd(Header.SenderNodeId);
				RemoteNode.Connection = *It;
				RemoteNode.LastDatagramReceivedTime = CurrentTime;

				TotalInboundBytes += Payload->Num();

				// forward payload to local nodes
				FIPv4Endpoint RecipientEndpoint;

				if (Header.RecipientNodeId.IsValid())
				{
					FNodeInfo* LocalNode = LocalNodes.Find(Header.RecipientNodeId);

					if (LocalNode == NULL)
					{
						continue;
					}

					RecipientEndpoint = LocalNode->Endpoint;
				}
				else
				{
					RecipientEndpoint = MulticastEndpoint;
				}

				int32 BytesSent = 0;
				UnicastSocket->SendTo(Payload->GetTypedData(), Payload->Num(), BytesSent, *RecipientEndpoint.ToInternetAddr());
			}
			else
			{
				++It;
			}
		}
	}

	/**
	 * Receives all buffered datagrams from the specified socket and forwards them to the tunnels.
	 *
	 * @param Socket - The socket to receive from.
	 */
	void UdpToTcp( FSocket* Socket )
	{
		TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

		uint32 DatagramSize = 0;

		while (Socket->HasPendingData(DatagramSize))
		{
			if (DatagramSize > 0)
			{
				FArrayReaderPtr Datagram = MakeShareable(new FArrayReader(true));
				Datagram->Init(FMath::Min(DatagramSize, 65507u));

				int32 BytesRead = 0;

				if (Socket->RecvFrom(Datagram->GetData(), Datagram->Num(), BytesRead, *Sender))
				{
					FUdpMessageSegment::FHeader Header;
					*Datagram << Header;

					// check protocol version
					if (Header.ProtocolVersion != UDP_MESSAGING_TRANSPORT_PROTOCOL_VERSION)
					{
						return;
					}

					// ignore loopback datagrams
					if (RemoteNodes.Contains(Header.SenderNodeId))
					{
						return;
					}

					Datagram->Seek(0);

					// forward datagram to remote nodes
					if (Header.RecipientNodeId.IsValid())
					{
						FNodeInfo* RemoteNode = RemoteNodes.Find(Header.RecipientNodeId);

						if ((RemoteNode != NULL) && (RemoteNode->Connection.IsValid()))
						{
							RemoteNode->Connection->Send(Datagram);
						}
					}
					else
					{
						for (int32 ConnectionIndex = 0; ConnectionIndex < Connections.Num(); ++ConnectionIndex)
						{
							Connections[ConnectionIndex]->Send(Datagram);
						}
					}

					// update local node & statistics
					FNodeInfo& LocalNode = LocalNodes.FindOrAdd(Header.SenderNodeId);
					LocalNode.Endpoint = FIPv4Endpoint(Sender);
					LocalNode.LastDatagramReceivedTime = CurrentTime;

					TotalOutboundBytes += Datagram->Num();
				}
			}
		}
	}

	/**
	 * Updates all active and pending connections.
	 */
	void UpdateConnections( )
	{
		bool ConnectionsChanged = false;

		// remove closed connections
		for (int32 ConnectionIndex = Connections.Num() - 1; ConnectionIndex >= 0; --ConnectionIndex)
		{
			if (!Connections[ConnectionIndex]->IsOpen())
			{
				Connections.RemoveAtSwap(ConnectionIndex);

				ConnectionsChanged = true;
			}
		}

		// add pending connections
		if (!PendingConnections.IsEmpty())
		{
			FUdpMessageTunnelConnectionPtr PendingConnection;

			while (PendingConnections.Dequeue(PendingConnection))
			{
				Connections.Add(PendingConnection);
			}

			ConnectionsChanged = true;
		}

		// notify application
		if (ConnectionsChanged)
		{
			ConnectionsChangedDelegate.ExecuteIfBound();
		}
	}

private:

	// Callback for accepted connections to the local tunnel server.
	bool HandleListenerConnectionAccepted( FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint )
	{
		PendingConnections.Enqueue(MakeShareable(new FUdpMessageTunnelConnection(ClientSocket, ClientEndpoint)));

		return true;
	}

private:

	// Holds the list of open tunnel connections.
	TArray<FUdpMessageTunnelConnectionPtr> Connections;

	// Holds a critical section for serializing access to the connections list.
	FCriticalSection CriticalSection;

	// Holds the current time.
	FDateTime CurrentTime;

	// Holds the local listener for incoming tunnel connections.
	FTcpListener* Listener;

	// Holds a collection of information for local transport nodes.
	TMap<FGuid, FNodeInfo> LocalNodes;

	// Holds the multicast endpoint.
	FIPv4Endpoint MulticastEndpoint;

	// Holds the multicast socket.
	FSocket* MulticastSocket;

	// Holds a queue of pending connections.
	TQueue<FUdpMessageTunnelConnectionPtr, EQueueMode::Mpsc> PendingConnections;

	// Holds a collection of information for remote transport nodes.
	TMap<FGuid, FNodeInfo> RemoteNodes;

	// Holds a flag indicating that the thread is stopping.
	bool Stopping;

	// Holds the thread object.
	FRunnableThread* Thread;

	// Holds the total number of bytes that were received from tunnels.
	uint64 TotalInboundBytes;

	// Holds the total number of bytes that were sent out through tunnels.
	uint64 TotalOutboundBytes;

	// Holds the unicast socket.
	FSocket* UnicastSocket;

private:

	// Holds a delegate that is executed when the list of incoming connections changed.
	FSimpleDelegate ConnectionsChangedDelegate;
};
