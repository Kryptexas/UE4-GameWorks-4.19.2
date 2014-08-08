// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#if ENABLE_HTTP_FOR_NFS
#include "ITransport.h"

#if !PLATFORM_HTML5
#include "Http.h"
#endif 

class FHTTPTransport : public ITransport
{

public:

	FHTTPTransport();

	// ITransport Interface.
	virtual bool Initialize(const TCHAR* HostIp) override;
	virtual bool SendPayloadAndReceiveResponse(TArray<uint8>& In, TArray<uint8>& Out) override;
	virtual bool ReceiveResponse(TArray<uint8> &Out) override { return false; } // unsupported for now :(

private: 

#if !PLATFORM_HTML5
	FHttpRequestPtr HttpRequest; 
#endif 

	FGuid Guid; 
	TCHAR Url[1048];

};
#endif