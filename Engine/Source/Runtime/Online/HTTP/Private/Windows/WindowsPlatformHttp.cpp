// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "WindowsPlatformHttp.h"
#include "HttpWinInet.h"
#include "Curl/CurlHttp.h"
#include "Curl/CurlHttpManager.h"

bool bUseCurl = false;

void FWindowsPlatformHttp::Init()
{
	FString HttpMode;
	if (FParse::Value(FCommandLine::Get(), TEXT("HTTP="), HttpMode) &&
		(HttpMode.Equals(TEXT("Curl"), ESearchCase::IgnoreCase) || HttpMode.Equals(TEXT("LibCurl"), ESearchCase::IgnoreCase)))
	{
		bUseCurl = true;
	}

#if WITH_LIBCURL
	FCurlHttpManager::InitCurl();
#endif
}

void FWindowsPlatformHttp::Shutdown()
{
#if WITH_LIBCURL
	FCurlHttpManager::ShutdownCurl();
#endif
	FWinInetConnection::Get().ShutdownConnection();
}

FHttpManager * FWindowsPlatformHttp::CreatePlatformHttpManager()
{
#if WITH_LIBCURL
	if (bUseCurl)
	{
		return new FCurlHttpManager();
	}
#endif
	// allow default to be used
	return NULL;
}

IHttpRequest* FWindowsPlatformHttp::ConstructRequest()
{
#if WITH_LIBCURL
	if (bUseCurl)
	{
		return new FCurlHttpRequest(FCurlHttpManager::GMultiHandle);
	}
	else
#endif
	{
		return new FHttpRequestWinInet();
	}
}