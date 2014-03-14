// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	INetworkFileServerConnection.h: Declares the INetworkFileServerConnection interface.
=============================================================================*/

#pragma once


/**
 * Interface for network file server connections.
 */
class INetworkFileServerConnection
{
public:

	/**
	 * Closes the connection.
	 */
	virtual void Close( ) = 0;

	/**
	 * Checks whether the connection is open.
	 *
	 * @return true if the connection is open, false otherwise.
	 */
	virtual bool IsOpen( ) const = 0;

	/** @return description of the client, ie. internet address and port. */
	virtual FString GetDescription() const = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~INetworkFileServerConnection( ) { }
};