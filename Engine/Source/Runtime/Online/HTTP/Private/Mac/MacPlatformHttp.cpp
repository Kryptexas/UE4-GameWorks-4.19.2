// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "MacPlatformHttp.h"
#include "MacHTTP.h"


void FMacPlatformHttp::Init()
{
}


void FMacPlatformHttp::Shutdown()
{
}


IHttpRequest* FMacPlatformHttp::ConstructRequest()
{
	return new FMacHttpRequest();
}
