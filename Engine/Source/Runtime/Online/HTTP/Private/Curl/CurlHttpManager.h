// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_LIBCURL
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif
	#include "curl/curl.h"
#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
#include "HttpManager.h"

class FCurlHttpManager : public FHttpManager
{
protected:

	/** multi handle that groups all the requests - not owned by this class */
	CURLM * MultiHandle;

	/** Cached value of running request to reduce number of read info calls*/
	int LastRunningRequests;

	/** Mapping of libcurl easy handles to HTTP requests */
	TMap<CURL*, TSharedRef<class IHttpRequest> > HandlesToRequests;

public:

	// Begin HttpManager interface
	virtual void AddRequest(TSharedRef<class IHttpRequest> Request) OVERRIDE;
	virtual void RemoveRequest(TSharedRef<class IHttpRequest> Request) OVERRIDE;
	virtual bool Tick(float DeltaSeconds) OVERRIDE;
	// End HttpManager interface

	FCurlHttpManager();

	static void InitCurl();
	static void ShutdownCurl();
	static CURLM * GMultiHandle;
};

#endif //WITH_LIBCURL
