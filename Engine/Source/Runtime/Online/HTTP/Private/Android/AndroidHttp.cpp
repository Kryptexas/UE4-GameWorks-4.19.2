// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "AndroidHttp.h"


void FAndroidPlatformHttp::Init()
{
}


void FAndroidPlatformHttp::Shutdown()
{
}


IHttpRequest* FAndroidPlatformHttp::ConstructRequest()
{
	return FGenericPlatformHttp::ConstructRequest();
}