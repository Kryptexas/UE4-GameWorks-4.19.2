// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Ticker.h"
#include "HttpPackage.h"

/**
 * Manages Http request that are currently being processed
 */
class FHttpManager : public FTickerObjectBase
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
	* @return shared ptr to the request or invalid shared ptr
	*/
	virtual bool IsValidRequest(const IHttpRequest* RequestPtr) const;

	/**
	 * FTicker callback
	 *
	 * @param DeltaSeconds - time in seconds since the last tick
	 *
	 * @return false if no longer needs ticking
	 */
	bool Tick(float DeltaSeconds) override;

	/**
	 * List all of the Http requests currently being processed
	 *
	 * @param Ar - output device to log with
	 */
	virtual void DumpRequests(FOutputDevice& Ar) const;

	/**
	 * Write http analytics events and clear them out
	 *
	 * @param Analytics - provider to use for writing events
	 */
	virtual void FlushAnalytics(class IAnalyticsProvider& Analytics);

protected:
	
	/** Keep track of an http request while it is being processed */
	class FActiveHttpRequest
	{
	public:
		FActiveHttpRequest(double InStartTime, const TSharedRef<IHttpRequest>& InHttpRequest)
			: StartTime(InStartTime)
			, HttpRequest(InHttpRequest)
		{}

		double StartTime;
		TSharedRef<IHttpRequest> HttpRequest;
	};

	/** List of Http requests that are actively being processed */
	TMap<IHttpRequest*, FActiveHttpRequest> Requests;

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

	/** Analytics data to keep track of for completed http requests */
	class FAnalyticsHttpRequest
	{
	public:
		FAnalyticsHttpRequest()
			: ElapsedTimeTotal(0)
			, ElapsedTimeMin(0)
			, ElapsedTimeMax(0)
			, DownloadRateTotal(0)
			, Count(0)
		{}

		void RecordRequest(double ElapsedTime, const TSharedRef<IHttpRequest>& HttpRequest);

		double ElapsedTimeTotal;
		double ElapsedTimeMin;
		double ElapsedTimeMax;
		double DownloadRateTotal;
		int32 Count;
		TMap<int32, int32> ResponseCodes;
	};
	TMap<FString, FAnalyticsHttpRequest> RequestAnalytics;

	void AddAnalytics(double ElapsedTime, const TSharedRef<IHttpRequest>& HttpRequest);

PACKAGE_SCOPE:

	/** Used to lock access to add/remove/find requests */
	static FCriticalSection RequestLock;
	/** Delay in seconds to defer deletion of requests */
	float DeferredDestroyDelay;
};
