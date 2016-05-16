// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Ticker.h"
#include "HttpPackage.h"

class IHttpRequest;
class IHttpThreadedRequest;

/**
 * Manages Http request that are currently being processed
 */
class FHttpManager
	: public FTickerObjectBase
	, FRunnable
{
public:

	// FHttpManager

	/**
	 * Constructor
	 */
	FHttpManager();

	/**
	 * Destructor
	 */
	virtual ~FHttpManager();

	/**
	 * Adds an Http request instance to the manager for tracking/ticking
	 * Manager should always have a list of requests currently being processed
	 *
	 * @param Request - the request object to add
	 */
	virtual void AddRequest(const TSharedRef<IHttpRequest>& Request);

	/**
	 * Removes an Http request instance from the manager
	 * Presumably it is done being processed
	 *
	 * @param Request - the request object to remove
	 */
	virtual void RemoveRequest(const TSharedRef<IHttpRequest>& Request);

	/**
	* Find an Http request in the lists of current valid requests
	*
	* @param RequestPtr - ptr to the http request object to find
	*
	* @return true if the request is being tracked, false if not
	*/
	virtual bool IsValidRequest(const IHttpRequest* RequestPtr) const;

	/**
	 * Block until all pending requests are finished processing
	 *
	 * @param bShutdown true if final flush during shutdown
	 */
	virtual void Flush(bool bShutdown);

	/**
	 * FTicker callback
	 *
	 * @param DeltaSeconds - time in seconds since the last tick
	 *
	 * @return false if no longer needs ticking
	 */
	virtual bool Tick(float DeltaSeconds) override;

	/** 
	 * Add a http request to be executed on the http thread
	 *
	 * @param Request - the request object to add
	 */
	virtual void AddThreadedRequest(const TSharedRef<IHttpThreadedRequest>& Request);

	/**
	 * Mark a threaded http request as cancelled to be removed from the http thread
	 *
	 * @param Request - the request object to cancel
	 */
	virtual void CancelThreadedRequest(const TSharedRef<IHttpThreadedRequest>& Request);

	/**
	 * List all of the Http requests currently being processed
	 *
	 * @param Ar - output device to log with
	 */
	virtual void DumpRequests(FOutputDevice& Ar) const;

protected:
	// FRunnable
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	/** signal request to stop and exit thread */
	FThreadSafeCounter ExitRequest;

	/**
	 * Tick on http thread
	 */
	virtual void HttpThreadTick(float DeltaSeconds);
	
	/** 
	 * Start processing a request on the http thread
	 */
	virtual bool StartThreadedRequest(IHttpThreadedRequest* Request);

	/** 
	 * Complete a request on the http thread
	 */
	virtual void CompleteThreadedRequest(IHttpThreadedRequest* Request);

protected:
	
	/** List of Http requests that are actively being processed */
	TArray<TSharedRef<IHttpRequest>> Requests;

	/** Keep track of a request that should be deleted later */
	class FRequestPendingDestroy
	{
	public:
		FRequestPendingDestroy(float InTimeLeft, const TSharedPtr<IHttpRequest>& InHttpRequest)
			: TimeLeft(InTimeLeft)
			, HttpRequest(InHttpRequest)
		{}

		FORCEINLINE bool operator==(const FRequestPendingDestroy& Other) const
		{
			return Other.HttpRequest == HttpRequest;
		}

		float TimeLeft;
		TSharedPtr<IHttpRequest> HttpRequest;
	};

	/** Dead requests that need to be destroyed */
	TArray<FRequestPendingDestroy> PendingDestroyRequests;

	/** Threaded requests that have been added on the game thread and are waiting to be processed on the http thread */
	TArray<IHttpThreadedRequest*> PendingThreadedRequests;

	/** Threaded requests that have been cancelled on the game thread and are waiting to be cancelled on the http thread */
	TArray<IHttpThreadedRequest*> CancelledThreadedRequests;

	/** Currently running threaded requests */
	TArray<IHttpThreadedRequest*> RunningThreadedRequests;

	/** Threaded requests completed on the http thread, awaiting processing on the game thread */
	TArray<IHttpThreadedRequest*> CompletedThreadedRequests;

	/** Target frame duration of a http thread tick, in seconds. */
	double	HttpThreadTickBudget;

	FRunnableThread* Thread;

PACKAGE_SCOPE:

	/** Used to lock access to add/remove/find requests */
	static FCriticalSection RequestLock;
	/** Delay in seconds to defer deletion of requests */
	float DeferredDestroyDelay;
};
