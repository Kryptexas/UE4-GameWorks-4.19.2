// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UdpSocketBuilder.h: Declares the FUdpSocketBuilder class.
=============================================================================*/

#pragma once


/**
 * Implements a fluent builder for UDP sockets.
 */
class FUdpSocketBuilder
{
public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InDescription - Debug description for the socket.
	 */
	FUdpSocketBuilder( const FString& InDescription )
		: AllowBroadcast(false)
		, Blocking(false)
		, Bound(false)
		, BoundEndpoint(FIPv4Address::Any, 0)
		, Description(InDescription)
		, MulticastLoopback(false)
		, MulticastTtl(1)
		, Reusable(false)
	{ }


public:

	/**
	 * Sets socket operations to be blocking.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder AsBlocking( )
	{
		Blocking = true;

		return *this;
	}

	/**
	 * Sets socket operations to be non-blocking.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder AsNonBlocking( )
	{
		Blocking = false;

		return *this;
	}

	/**
	 * Makes the bound address reusable by other sockets.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder AsReusable( )
	{
		Reusable = true;

		return *this;
	}

	/**
 	 * Sets the local address to bind the socket to.
	 *
	 * Unless specified in a subsequent call to BoundToPort(), a random
	 * port number will be assigned by the underlying provider.
	 *
	 * @param Address - The IP address to bind the socket to.
	 *
	 * @return This instance (for method chaining).
	 *
	 * @see BoundToEndpoint
	 * @see BoundToPort
	 */
	FUdpSocketBuilder BoundToAddress( const FIPv4Address& Address )
	{
		BoundEndpoint = FIPv4Endpoint(Address, BoundEndpoint.GetPort());
		Bound = true;

		return *this;
	}

	/**
 	 * Sets the local endpoint to bind the socket to.
	 *
	 * @param Endpoint - The IP endpoint to bind the socket to.
	 *
	 * @return This instance (for method chaining).
	 *
	 * @see BoundToAddress
	 * @see BoundToPort
	 */
	FUdpSocketBuilder BoundToEndpoint( const FIPv4Endpoint& Endpoint )
	{
		BoundEndpoint = Endpoint;
		Bound = true;

		return *this;
	}

	/**
	 * Sets the local port to bind the socket to.
	 *
	 * Unless specified in a subsequent call to BoundToAddress(), the local
	 * address will be determined automatically by the underlying provider.
	 *
	 * @param Port - The local port number to bind the socket to.
	 *
	 * @return This instance (for method chaining).
	 *
	 * @see BoundToAddress
	 */
	FUdpSocketBuilder BoundToPort( int32 Port )
	{
		BoundEndpoint = FIPv4Endpoint(BoundEndpoint.GetAddress(), Port);
		Bound = true;

		return *this;
	}

	/**
	 * Joins the socket to the specified multicast group.
	 *
	 * @param GroupAddress - The IP address of the multicast group to join.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder JoinedToGroup( const FIPv4Address& GroupAddress )
	{
		JoinedGroups.Add(GroupAddress);

		return *this;
	}

	/**
	 * Enables broadcasting.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder WithBroadcast( )
	{
		AllowBroadcast = true;

		return *this;
	}

	/**
	 * Enables multicast loopback.
	 *
	 * @param AllowLoopback - Whether to allow multicast loopback.
	 * @param TimeToLive - The time to live.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder WithMulticastLoopback( )
	{
		MulticastLoopback = true;

		return *this;
	}

	/**
	 * Sets the multicast time-to-live.
	 *
	 * @param TimeToLive - The time to live.
	 *
	 * @return This instance (for method chaining).
	 */
	FUdpSocketBuilder WithMulticastTtl( uint8 TimeToLive )
	{
		MulticastTtl = TimeToLive;

		return *this;
	}


public:

	/**
	 * Implicit conversion operator that builds the socket as configured.
	 *
	 * @return The built socket.
	 */
	operator FSocket*( ) const
	{
		return Build();
	}

	/**
	 * Builds the socket as configured.
	 *
	 * @return The built socket.
	 */
	FSocket* Build( ) const
	{
		FSocket* Socket = NULL;

		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

		if (SocketSubsystem != NULL)
		{
			Socket = SocketSubsystem->CreateSocket(NAME_DGram, *Description, true);

			if (Socket != NULL)
			{
				bool Error = !Socket->SetNonBlocking(!Blocking) ||
							 !Socket->SetReuseAddr(Reusable) ||
							 !Socket->SetBroadcast(AllowBroadcast) ||
							 !Socket->SetRecvErr();

				if (!Error)
				{
					Error = Bound && !Socket->Bind(*SocketSubsystem->CreateInternetAddr(BoundEndpoint.GetAddress().GetValue(), BoundEndpoint.GetPort()));
				}

				if (!Error)
				{
					Error = !Socket->SetMulticastLoopback(MulticastLoopback) ||
							!Socket->SetMulticastTtl(MulticastTtl);
				}

				if (!Error)
				{
					for (int32 Index = 0; Index < JoinedGroups.Num(); ++Index)
					{
						if (!Socket->JoinMulticastGroup(*SocketSubsystem->CreateInternetAddr(JoinedGroups[Index].GetValue(), 0)))
						{
							Error = true;

							break;
						}
					}
				}

				if (Error)
				{
					GLog->Logf(TEXT("FUdpSocketBuilder: Failed to create the socket %s as configured"), *Description);

					SocketSubsystem->DestroySocket(Socket);

					Socket = NULL;
				}
			}
		}

		return Socket;
	}


private:

	// Holds a flag indicating whether broadcasts will be enabled.
	bool AllowBroadcast;

	// Holds a flag indicating whether socket operations are blocking.
	bool Blocking;

	// Holds a flag indicating whether the socket should be bound.
	bool Bound;

	// Holds the IP address (and port) that the socket will be bound to.
	FIPv4Endpoint BoundEndpoint;

	// Holds the socket's debug description text.
	FString Description;

	// Holds the list of joined multicast groups.
	TArray<FIPv4Address> JoinedGroups;

	// Holds a flag indicating whether multicast loopback will be enabled.
	bool MulticastLoopback;

	// Holds the multicast time to live.
	uint8 MulticastTtl;

	// Holds a flag indicating whether the bound address can be reused by other sockets.
	bool Reusable;
};
