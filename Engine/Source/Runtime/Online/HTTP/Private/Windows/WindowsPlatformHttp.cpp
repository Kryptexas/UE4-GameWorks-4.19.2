// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "WindowsPlatformHttp.h"
#include "HttpWinInet.h"


void FWindowsPlatformHttp::Init()
{
}


void FWindowsPlatformHttp::Shutdown()
{
	FWinInetConnection::Get().ShutdownConnection();
}


IHttpRequest* FWindowsPlatformHttp::ConstructRequest()
{
	return new FHttpRequestWinInet();
}