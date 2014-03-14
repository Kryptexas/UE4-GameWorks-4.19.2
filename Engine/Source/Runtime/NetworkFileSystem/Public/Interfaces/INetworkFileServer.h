// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	INetworkFileServer.h: Declares the INetworkFileServer interface.
=============================================================================*/

#pragma once


/**
 * Interface for network file servers.
 */
class INetworkFileServer
{
public:

	/**
	 * Gets the list of local network addresses that the file server listens on.
	 *
	 * @param OutAddresses - Will hold the address list.
	 *
	 * @return true on success, false otherwise.
	 */
	virtual bool GetAddressList( TArray<TSharedPtr<FInternetAddr> >& OutAddresses ) const = 0;

	/**
	 * Gets the number of active connections.
	 *
	 * @return The number of connections.
	 */
	virtual int32 NumConnections( ) const = 0;

	/**
	 * Shuts down the file server.
	 */
	virtual void Shutdown( ) = 0;


public:

	/**
	 * Virtual destructor.
	 */
	virtual ~INetworkFileServer( ) { }
};