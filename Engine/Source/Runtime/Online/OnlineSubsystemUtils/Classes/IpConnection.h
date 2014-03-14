// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//
// Ip based implementation of a network connection used by the net driver class
//

#pragma once
#include "IpConnection.generated.h"

UCLASS(transient, config=Engine)
class ONLINESUBSYSTEMUTILS_API UIpConnection : public UNetConnection
{
    GENERATED_UCLASS_BODY()
	// Variables.
	TSharedPtr<FInternetAddr>	RemoteAddr;
	FSocket*			Socket;
	FResolveInfo*		ResolveInfo;

	// Begin NetConnection Interface
	virtual void InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket = 0, int32 InPacketOverhead = 0) OVERRIDE;
	virtual void InitRemoteConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, const class FInternetAddr& InRemoteAddr, EConnectionState InState, int32 InMaxPacket = 0, int32 InPacketOverhead = 0) OVERRIDE;
	virtual void InitLocalConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket = 0, int32 InPacketOverhead = 0) OVERRIDE;
	virtual void LowLevelSend(void* Data,int32 Count) OVERRIDE;
	FString LowLevelGetRemoteAddress(bool bAppendPort=false) OVERRIDE;
	FString LowLevelDescribe() OVERRIDE;
	virtual int32 GetAddrAsInt(void) OVERRIDE
	{
		uint32 OutAddr = 0;
		// Get the host byte order ip addr
		RemoteAddr->GetIp(OutAddr);
		return (int32)OutAddr;
	}
	virtual int32 GetAddrPort(void) OVERRIDE
	{
		int32 OutPort = 0;
		// Get the host byte order ip port
		RemoteAddr->GetPort(OutPort);
		return OutPort;
	}
	// End NetConnection Interface
};
