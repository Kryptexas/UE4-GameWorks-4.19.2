// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "HTML5PlatformHttp.h"


void FHTML5PlatformHttp::Init()
{
}


void FHTML5PlatformHttp::Shutdown()
{
}


IHttpRequest* FHTML5PlatformHttp::ConstructRequest()
{
	return FGenericPlatformHttp::ConstructRequest();
}