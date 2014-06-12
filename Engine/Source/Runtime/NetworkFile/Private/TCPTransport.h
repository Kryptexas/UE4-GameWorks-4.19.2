// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ITransport.h"

class FTCPTransport : public ITransport
{

public:

	FTCPTransport();
	~FTCPTransport();

	// ITransport Interface. 
	virtual bool Initialize(const TCHAR* HostIp);
	virtual bool SendPayloadAndReceiveResponse(TArray<uint8>& In, TArray<uint8>& Out); 

private: 

	class FSocket*		FileSocket;
	class FMultichannelTcpSocket* MCSocket;

};