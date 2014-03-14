// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StreamingNetworkFileServer.h: Declares the FStreamingNetworkFileServer class.
=============================================================================*/

#pragma once


/**
 * This class wraps the server thread and network connection
 */
class FNetworkFileServer
	: public FRunnable
	, public INetworkFileServer
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InPort - The port number to bind to (0 = any available port).
	 * @param InFileRequestDelegate -
	 */
	FNetworkFileServer( int32 InPort, const FFileRequestDelegate* InFileRequestDelegate, const FRecompileShadersDelegate* InRecompileShadersDelegate );

	/**
	 * Destructor.
	 */
	~FNetworkFileServer( );


public:

	// Begin FRunnable Interface

	virtual bool Init( ) OVERRIDE
	{
		return true;
	}

	virtual uint32 Run( ) OVERRIDE;

	virtual void Stop( ) OVERRIDE
	{
		bNeedsToStop = true;
	}

	virtual void Exit( ) OVERRIDE;

	// End FRunnable Interface


public:

	// Begin INetworkFileServer interface

	virtual bool GetAddressList( TArray<TSharedPtr<FInternetAddr> >& OutAddresses ) const OVERRIDE;

	virtual int32 NumConnections( ) const OVERRIDE
	{
		return Connections.Num();
	}

	virtual void Shutdown( ) OVERRIDE
	{
		Stop();
	}

	// End INetworkFileServer interface


private:

	// Holds the port to use.
	int32 FileServerPort;

	// Holds the server (listening) socket.
	FSocket* Socket;

	// Holds the server thread object.
	FRunnableThread* Thread;

	// Holds the list of all client connections.
	TArray<INetworkFileServerConnection*> Connections;

	// Holds a flag indicating whether the thread should stop executing.
	bool bNeedsToStop;


public:

	// Holds a delegate to be invoked on every sync request.
	FFileRequestDelegate FileRequestDelegate;

	// Holds a delegate to be invoked when a client requests a shader recompile.
	FRecompileShadersDelegate RecompileShadersDelegate;

	// Holds the address that the server is bound to.
	TSharedPtr<FInternetAddr> ListenAddr;
};