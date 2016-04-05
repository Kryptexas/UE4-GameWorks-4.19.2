// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

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
	/** 
	 * Struture containing data associated with a CURL easy handle
	 */
	struct CurlEasyRequestData
	{
		CurlEasyRequestData(const TSharedRef<class IHttpRequest>& InRequest)
			: Request(InRequest)
			, DateTime(FDateTime::Now())
			, bProcessingStarted(false)
			, bAddedToMulti(false)
		{
		}

		/** Pointer to the Http Request */
		TSharedRef<class IHttpRequest> Request;
		
		/** Date time this was added to ensure first in first out */
		FDateTime DateTime;

		/** Have we started processing the request by adding it to curl's multi handle */
		bool bProcessingStarted;

		/** Is the request currently added to curl's multi handle */
		bool bAddedToMulti;
	};


	/** multi handle that groups all the requests - not owned by this class */
	CURLM * MultiHandle;

	/** Maximum number of requests that can be added to MultiHandle simultaneously (0 = no limit) */
	int32 MaxSimultaneousRequests;
	
	/** Maximum number of requests that can be added to MultiHandle in one tick (0 = no limit) */
	int32 MaxRequestsAddedPerFrame;

	/** Number of requests that have been added to curl multi */
	int NumRequestsAddedToMulti;

	/** Mapping of libcurl easy handles to HTTP requests */
	TMap<CURL*, CurlEasyRequestData> HandlesToRequests;

	/** Find the next easy handle to be processed */
	bool FindNextEasyHandle(CURL** OutEasyHandle) const;

	/** Global multi handle */
	static CURLM * GMultiHandle;

public:

	//~ Begin HttpManager Interface
	virtual void AddRequest(const TSharedRef<class IHttpRequest>& Request) override;
	virtual void RemoveRequest(const TSharedRef<class IHttpRequest>& Request) override;
	virtual bool Tick(float DeltaSeconds) override;
	//~ End HttpManager Interface

	FCurlHttpManager();

	static void InitCurl();
	static void ShutdownCurl();
	static CURLSH* GShareHandle;

	static struct FCurlRequestOptions
	{
		FCurlRequestOptions()
			:	bVerifyPeer(true)
			,	bUseHttpProxy(false)
			,	bDontReuseConnections(false)
			,	CertBundlePath(nullptr)
		{}

		/** Prints out the options to the log */
		void Log();

		/** Whether or not should verify peer certificate (disable to allow self-signed certs) */
		bool bVerifyPeer;

		/** Whether or not should use HTTP proxy */
		bool bUseHttpProxy;

		/** Forbid reuse connections (for debugging purposes, since normally it's faster to reuse) */
		bool bDontReuseConnections;

		/** Address of the HTTP proxy */
		FString HttpProxyAddress;

		/** A path to certificate bundle */
		const char * CertBundlePath;
	}
	CurlRequestOptions;
};

#endif //WITH_LIBCURL
