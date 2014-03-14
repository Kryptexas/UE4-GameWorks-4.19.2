// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IMessageTunnelConnection.h: Declares the IMessageTunnelConnection interface.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of IMessageTunnelConnection.
 */
typedef TSharedPtr<class IMessageTunnelConnection> IMessageTunnelConnectionPtr;

/**
 * Type definition for shared references to instances of IMessageTunnelConnection.
 */
typedef TSharedRef<class IMessageTunnelConnection> IMessageTunnelConnectionRef;


/**
 * Interface for message tunnel connections.
 */
class IMessageTunnelConnection
{
public:

	/**
	 * Closes this connection.
	 */
	virtual void Close( ) = 0;

	/**
	 * Gets the total number of bytes received from this connection.
	 *
	 * @return Number of bytes.
	 */
	virtual uint64 GetTotalBytesReceived( ) const = 0;

	/**
	 * Gets the total number of bytes received from this connection.
	 *
	 * @return Number of bytes.
	 */
	virtual uint64 GetTotalBytesSent( ) const = 0;

	/**
	 * Gets the human readable name of the connection.
	 *
	 * @return Connection name.
	 */
	virtual FText GetName( ) const = 0;

	/**
	 * Gets the amount of time that the connection has been established.
	 *
	 * @return Time span.
	 */
	virtual FTimespan GetUptime( ) const = 0;

	/**
	 * Checks whether this connection is open.
	 *
	 * @return true if the connection is open, false otherwise.
	 */
	virtual bool IsOpen( ) const = 0;

public:

	/**
	 * Virtual destructor.
	 */
	virtual ~IMessageTunnelConnection( ) { }
};
