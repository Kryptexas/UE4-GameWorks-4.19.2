// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	NetworkFileServer.cpp: Implements the FNetworkFileServer class.
=============================================================================*/

#include "NetworkFileSystemPrivatePCH.h"


/* FNetworkFileServer constructors
 *****************************************************************************/

FNetworkFileServer::FNetworkFileServer( int32 InPort, const FFileRequestDelegate* InFileRequestDelegate, const FRecompileShadersDelegate* InRecompileShadersDelegate )
	: bNeedsToStop(false)
{
	UE_LOG(LogFileServer, Warning, TEXT("Unreal Network File Server starting up..."));

	if (InFileRequestDelegate && InFileRequestDelegate->IsBound())
	{
		FileRequestDelegate = *InFileRequestDelegate;
	}

	if (InRecompileShadersDelegate && InRecompileShadersDelegate->IsBound())
	{
		RecompileShadersDelegate = *InRecompileShadersDelegate;
	}

	// make sure sockets are going
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get();

	if(!SocketSubsystem)
	{
		UE_LOG(LogFileServer, Error, TEXT("Could not get socket subsystem."));
	}
	else
	{
		// create a server TCP socket
		Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("FNetworkFileServer tcp-listen"));

		if(!Socket)
		{
			UE_LOG(LogFileServer, Error, TEXT("Could not create listen socket."));
		}
		else
		{
			// listen on any IP address
			ListenAddr = SocketSubsystem->GetLocalBindAddr(*GLog);

			ListenAddr->SetPort(InPort);
			Socket->SetReuseAddr();

			// bind to the address
			if (!Socket->Bind(*ListenAddr))
			{
				UE_LOG(LogFileServer, Warning, TEXT("Failed to bind listen socket %s in FNetworkFileServer"), *ListenAddr->ToString(true));
			}
			// listen for connections
			else if (!Socket->Listen(16))
			{
				UE_LOG(LogFileServer, Warning, TEXT("Failed to listen on socket %s in FNetworkFileServer"), *ListenAddr->ToString(true));
			}
			else
			{
				// set the port on the listen address to be the same as the port on the socket
				int32 port = Socket->GetPortNo();
				check((InPort == 0 && port != 0) || port == InPort);
				ListenAddr->SetPort(port);

				// now create a thread to accept connections
				Thread = FRunnableThread::Create(this,TEXT("FNetworkFileServer"), false, false, 8 * 1024, TPri_AboveNormal);

				UE_LOG(LogFileServer, Display, TEXT("Unreal Network File Server is ready for client connections on %s!"), *ListenAddr->ToString(true));
			}
		}
	}
}

FNetworkFileServer::~FNetworkFileServer()
{
	// Kill the running thread.
	if( Thread != NULL )
	{
		Thread->Kill(true);

		delete Thread;
		Thread = NULL;
	}

	// We are done with the socket.
	Socket->Close();
	ISocketSubsystem::Get()->DestroySocket(Socket);
	Socket = NULL;
}


/* FRunnable overrides
 *****************************************************************************/

uint32 FNetworkFileServer::Run( )
{
	// go until requested to be done
	while (!bNeedsToStop)
	{
		bool bReadReady = false;

		// clean up closed connections
		for (int32 ConnectionIndex = 0; ConnectionIndex < Connections.Num(); ++ConnectionIndex)
		{
			INetworkFileServerConnection* Connection = Connections[ConnectionIndex];

			if (!Connection->IsOpen())
			{
				UE_LOG(LogFileServer, Display, TEXT( "Client %s disconnected." ), *Connection->GetDescription() );
				Connections.RemoveAtSwap(ConnectionIndex);
				delete Connection;
			}
		}

		// check for incoming connections
		if (Socket->HasPendingConnection(bReadReady) && bReadReady)
		{
			FSocket* ClientSocket = Socket->Accept(TEXT("Remote Console Connection"));

			if (ClientSocket != NULL)
			{
				FNetworkFileServerClientConnection* Connection = new FNetworkFileServerClientConnection(ClientSocket, FileRequestDelegate, RecompileShadersDelegate);
				Connections.Add(Connection);
				UE_LOG(LogFileServer, Display, TEXT( "Client %s connected." ), *Connection->GetDescription() );
			}
		}

		FPlatformProcess::Sleep(0.25f);
	}

	return 0;
}


void FNetworkFileServer::Exit( )
{
	// close all connections
	for (int32 ConnectionIndex = 0; ConnectionIndex < Connections.Num(); ConnectionIndex++)
	{
		delete Connections[ConnectionIndex];
	}

	Connections.Empty();
}


/* INetworkFileServer overrides
 *****************************************************************************/

bool FNetworkFileServer::GetAddressList( TArray<TSharedPtr<FInternetAddr> >& OutAddresses ) const
{
	if (ListenAddr.IsValid())
	{
		FString ListenAddressString = ListenAddr->ToString(true);

		if (ListenAddressString.StartsWith(TEXT("0.0.0.0")))
		{
			if (ISocketSubsystem::Get()->GetLocalAdapterAddresses(OutAddresses))
			{
				for (int32 AddressIndex = 0; AddressIndex < OutAddresses.Num(); ++AddressIndex)
				{
					OutAddresses[AddressIndex]->SetPort(ListenAddr->GetPort());
				}
			}
		}
		else
		{
			OutAddresses.Add(ListenAddr);
		}
	}

	return (OutAddresses.Num() > 0);
}