// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpMessageTunnelConnection.h: Declares the FUdpMessageTunnelConnection class.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of FUdpMessageTunnelConnection.
 */
typedef TSharedPtr<class FUdpMessageTunnelConnection> FUdpMessageTunnelConnectionPtr;

/**
 * Type definition for shared references to instances of FUdpMessageTunnelConnection.
 */
typedef TSharedRef<class FUdpMessageTunnelConnection> FUdpMessageTunnelConnectionRef;

/**
 * Type definition for thread-safe shared references to byte arrays.
 */
typedef TSharedRef<TArray<uint8>, ESPMode::ThreadSafe> FSaveByteArrayRef;


struct FUdpMessageTunnelPacket
{

};


/**
 * Delegate type for received packets.
 *
 * The first parameter is the received packet data.
 * The second parameter is the sender's node identifier.
 */
DECLARE_DELEGATE_TwoParams(FOnUdpMessageTunnelConnectionPacketReceived, FSaveByteArrayRef, const FGuid&)


/**
 * Implements a UDP message tunnel connection.
 */
class FUdpMessageTunnelConnection
	: public FRunnable
	, public TSharedFromThis<FUdpMessageTunnelConnection>
	, public IMessageTunnelConnection
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InSocket - The socket to use for this connection.
	 * @param InRemoteAddress - The IP endpoint of the remote client.
	 */
	FUdpMessageTunnelConnection( FSocket* InSocket, const FIPv4Endpoint& InRemoteEndpoint )
		: OpenedTime(FDateTime::UtcNow())
		, RemoteEndpoint(InRemoteEndpoint)
		, Socket(InSocket)
		, TotalBytesReceived(0)
		, TotalBytesSent(0)
		, bRun(false)
	{
		int32 NewSize = 0;
		Socket->SetReceiveBufferSize(2*1024*1024, NewSize);
		Socket->SetSendBufferSize(2*1024*1024, NewSize);
		Thread = FRunnableThread::Create(this, TEXT("FUdpMessageTunnelConnection"), false, false, 128 * 1024, TPri_Normal);
	}

	/**
	 * Destructor.
	 */
	~FUdpMessageTunnelConnection( )
	{
		if (Thread != NULL)
		{
			Thread->Kill(true);
			delete Thread;
		}

		delete Socket;
		Socket = NULL;
	}

public:

	/**
	 * Receives a payload from the connection's inbox.
	 *
	 * @param OutPayload - Will hold the received payload, if any.
	 *
	 * @return true if a payload was returned, false otherwise.
	 */
	bool Receive( FArrayReaderPtr& OutPayload )
	{
		return Inbox.Dequeue(OutPayload);
	}

	/**
	 * Sends a payload through this connection.
	 *
	 * @param Payload - The payload to send.
	 */
	bool Send( const FArrayReaderPtr& Payload )
	{
		if (IsOpen())
		{
			return Outbox.Enqueue(Payload);
		}

		return false;
	}

public:

	// Begin FRunnable interface

	virtual bool Init( ) OVERRIDE
	{
		return (Socket != NULL);
	}

	virtual uint32 Run( ) OVERRIDE
	{
		bRun = true;

		while (bRun)
		{
			if (Socket->Wait(ESocketWaitConditions::WaitForReadOrWrite, FTimespan::Zero()))
			{
				bRun = ReceivePayloads();
				bRun = SendPayloads();
			}

			FPlatformProcess::Sleep(0.0f);
		}

		ClosedTime = FDateTime::UtcNow();

		return 0;
	}

	virtual void Stop( ) OVERRIDE
	{
		Socket->Close();
		bRun = false;
	}

	virtual void Exit( ) OVERRIDE { }

	// End FRunnable interface

public:

	// Begin IMessageTunnelConnection interface

	virtual void Close( ) OVERRIDE
	{
		Stop();
	}

	virtual uint64 GetTotalBytesReceived( ) const OVERRIDE
	{
		return TotalBytesReceived;
	}

	virtual uint64 GetTotalBytesSent( ) const OVERRIDE
	{
		return TotalBytesSent;
	}

	virtual FText GetName( ) const
	{
		return RemoteEndpoint.ToText();
	}

	virtual FTimespan GetUptime( ) const OVERRIDE
	{
		if (IsOpen())
		{
			return (FDateTime::UtcNow() - OpenedTime);
		}
		
		return (ClosedTime - OpenedTime);
	}

	virtual bool IsOpen( ) const OVERRIDE
	{
		return (Socket->GetConnectionState() == SCS_Connected);
	}

	// End IMessageTunnelConnection interface

protected:

	/**
	 * Receives all pending payloads from the socket.
	 *
	 * @return true on success, false otherwise.
	 */
	bool ReceivePayloads( )
	{
		uint32 PendingDataSize = 0;

		// if we received a payload size...
		while (Socket->HasPendingData(PendingDataSize) && (PendingDataSize >= sizeof(uint16)))
		{
			int32 BytesRead = 0;
			
			FArrayReader PayloadSizeData = FArrayReader(true);
			PayloadSizeData.Init(sizeof(uint16));
			
			// ... read it from the stream without removing it...
			if (!Socket->Recv(PayloadSizeData.GetTypedData(), sizeof(uint16), BytesRead, ESocketReceiveFlags::Peek))
			{
				return false;
			}
			check(BytesRead == sizeof(uint16));

			uint16 PayloadSize;
			PayloadSizeData << PayloadSize;

			// ... and if we received the complete payload...
			if (Socket->HasPendingData(PendingDataSize) && (PendingDataSize >= sizeof(uint16) + PayloadSize))
			{
				// ... then remove the payload size from the stream...
				if (!Socket->Recv((uint8*)&PayloadSize, sizeof(uint16), BytesRead))
				{
					return false;
				}
				check(BytesRead == sizeof(uint16));

				TotalBytesReceived += BytesRead;

				// ... receive the payload
				FArrayReaderPtr Payload = MakeShareable(new FArrayReader(true));
				Payload->Init(PayloadSize);

				if (!Socket->Recv(Payload->GetData(), Payload->Num(), BytesRead))
				{
					return false;
				}
				check (BytesRead == PayloadSize);

				TotalBytesReceived += BytesRead;

				// .. and move it to the inbox
				Inbox.Enqueue(Payload);

				return true;
			}
		}

		return true;
	}

	/**
	 * Sends all pending payloads to the socket.
	 *
	 * @return true on success, false otherwise.
	 */
	bool SendPayloads( )
	{
		if (!Outbox.IsEmpty())
		{
			if (Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::Zero()))
			{
				FArrayReaderPtr Payload;
				Outbox.Dequeue(Payload);
				int32 BytesSent = 0;

				// send the payload size
				FArrayWriter PayloadSizeData = FArrayWriter(true);
				uint16 PayloadSize = Payload->Num();
				PayloadSizeData << PayloadSize;

				if (!Socket->Send(PayloadSizeData.GetTypedData(), sizeof(uint16), BytesSent))
				{
					return false;
				}

				TotalBytesSent += BytesSent;

				// send the payload
				if (!Socket->Send(Payload->GetTypedData(), Payload->Num(), BytesSent))
				{
					return false;
				}

				TotalBytesSent += BytesSent;
			}
		}

		return true;
	}

private:

	// Holds the time at which the connection was closed.
	FDateTime ClosedTime;

	// Holds the collection of received payloads.
	TQueue<FArrayReaderPtr> Inbox;

	// Holds the time at which the connection was opened.
	FDateTime OpenedTime;

	// Holds the collection of outbound payloads.
	TQueue<FArrayReaderPtr> Outbox;

	// Holds the IP endpoint of the remote client.
	FIPv4Endpoint RemoteEndpoint;

	// Holds the connection socket.
	FSocket* Socket;

	// Holds the thread object.
	FRunnableThread* Thread;

	// Holds the total number of bytes received from the connection.
	uint64 TotalBytesReceived;

	// Holds the total number of bytes sent to the connection.
	uint64 TotalBytesSent;

	// thread should continue running
	bool bRun;
};
