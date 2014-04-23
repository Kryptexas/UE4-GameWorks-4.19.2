// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Platform.h"

#if PLATFORM_HAS_BSD_SOCKETS

#include "Sockets.h"
#include "IPAddress.h"

namespace EIPv6SocketInternalState
{
	enum Param
	{
		CanRead,
		CanWrite,
		HasError,
	};
	enum Return
	{
		Yes,
		No,
		EncounteredError,
	};
};

/**
 * Implements a BSD network socket.
 */
class FSocketBSDIPv6
	: public FSocket
{
public:

	/**
	 * Assigns a BSD socket to this object
	 *
	 * @param InSocket the socket to assign to this object
	 * @param InSocketType the type of socket that was created
	 * @param InSocketDescription the debug description of the socket
	 * @param InSubsystem the subsystem that created this socket
	 */
	FSocketBSDIPv6(SOCKET InSocket, ESocketType InSocketType, const FString& InSocketDescription, ISocketSubsystem* InSubsystem) 
		: FSocket(InSocketType, InSocketDescription)
		, Socket(InSocket)
		, SocketSubsystem(InSubsystem)
	{ }

	/**
	 * Destructor.
	 *
	 * Closes the socket if it is still open
	 */
	virtual ~FSocketBSDIPv6()
	{
		Close();
	}


public:

	virtual bool Close() OVERRIDE;

	virtual bool Bind(const FInternetAddr& Addr) OVERRIDE;

	virtual bool Connect(const FInternetAddr& Addr) OVERRIDE;

	virtual bool Listen(int32 MaxBacklog) OVERRIDE;

	virtual bool HasPendingConnection(bool& bHasPendingConnection) OVERRIDE;

	virtual bool HasPendingData(uint32& PendingDataSize) OVERRIDE;

	virtual class FSocket* Accept(const FString& SocketDescription) OVERRIDE;

	virtual class FSocket* Accept(FInternetAddr& OutAddr, const FString& SocketDescription) OVERRIDE;

	virtual bool SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination) OVERRIDE;

	virtual bool Send(const uint8* Data, int32 Count, int32& BytesSent) OVERRIDE;

	virtual bool RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None) OVERRIDE;

	virtual bool Recv(uint8* Data,int32 BufferSize,int32& BytesRead, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None) OVERRIDE;

	virtual bool Wait(ESocketWaitConditions::Type Condition, FTimespan WaitTime) OVERRIDE;

	virtual ESocketConnectionState GetConnectionState() OVERRIDE;

	virtual void GetAddress(FInternetAddr& OutAddr) OVERRIDE;

	virtual bool SetNonBlocking(bool bIsNonBlocking = true) OVERRIDE;

	virtual bool SetBroadcast(bool bAllowBroadcast = true) OVERRIDE;

	virtual bool JoinMulticastGroup(const FInternetAddr& GroupAddress) OVERRIDE;

	virtual bool LeaveMulticastGroup(const FInternetAddr& GroupAddress) OVERRIDE;

	virtual bool SetMulticastLoopback(bool bLoopback) OVERRIDE;

	virtual bool SetMulticastTtl(uint8 TimeToLive) OVERRIDE;

	virtual bool SetReuseAddr(bool bAllowReuse = true) OVERRIDE;

	virtual bool SetLinger(bool bShouldLinger = true, int32 Timeout = 0) OVERRIDE;

	virtual bool SetRecvErr(bool bUseErrorQueue = true) OVERRIDE;

	virtual bool SetSendBufferSize(int32 Size,int32& NewSize) OVERRIDE;

	virtual bool SetReceiveBufferSize(int32 Size,int32& NewSize) OVERRIDE;

	virtual int32 GetPortNo() OVERRIDE;

	bool SetIPv6Only(bool bIPv6Only);


protected:

	// This is generally select(), but makes it easier for platforms without select to replace it
	virtual EIPv6SocketInternalState::Return HasState(EIPv6SocketInternalState::Param State, FTimespan WaitTime=FTimespan(0));

	// Holds the BSD socket object.
	SOCKET Socket;

	// Pointer to the subsystem that created it
	ISocketSubsystem * SocketSubsystem;
};

#endif	//PLATFORM_HAS_BSD_SOCKETS
