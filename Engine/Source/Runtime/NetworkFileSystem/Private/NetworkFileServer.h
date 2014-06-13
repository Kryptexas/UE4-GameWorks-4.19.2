// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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
	 * @param InPort The port number to bind to (0 = any available port).
	 * @param InFileRequestDelegate 
	 */
	FNetworkFileServer( int32 InPort, const FFileRequestDelegate* InFileRequestDelegate, 
		const FRecompileShadersDelegate* InRecompileShadersDelegate, const TArray<ITargetPlatform*>& InActiveTargetPlatforms );

	/**
	 * Destructor.
	 */
	~FNetworkFileServer( );

public:

	// FRunnable Interface

	virtual bool Init( ) OVERRIDE
	{
		return true;
	}

	virtual uint32 Run( ) OVERRIDE;

	virtual void Stop( ) OVERRIDE
	{
		StopRequested.Set(true);
	}

	virtual void Exit( ) OVERRIDE;

public:

	// INetworkFileServer interface

	virtual bool IsItReadyToAcceptConnections(void) const; 
	virtual bool GetAddressList(TArray<TSharedPtr<FInternetAddr> >& OutAddresses) const;
	virtual int32 NumConnections() const;
	virtual void Shutdown();

private:

	// Holds the port to use.
	int32 FileServerPort;

	// Holds the server (listening) socket.
	FSocket* Socket;

	// Holds the server thread object.
	FRunnableThread* Thread;

	// Holds the list of all client connections.
	TArray< class FNetworkFileServerClientConnectionThreaded*> Connections;

	// Holds a flag indicating whether the thread should stop executing
	FThreadSafeCounter StopRequested;

	// Is the Listner thread up and running. 
	FThreadSafeCounter Running;

public:

	// Holds a delegate to be invoked on every sync request.
	FFileRequestDelegate FileRequestDelegate;

	// Holds a delegate to be invoked when a client requests a shader recompile.
	FRecompileShadersDelegate RecompileShadersDelegate;

	// cached copy of the active target platforms (if any)
	const TArray<ITargetPlatform*> ActiveTargetPlatforms;

	// Holds the address that the server is bound to.
	TSharedPtr<FInternetAddr> ListenAddr;
};
