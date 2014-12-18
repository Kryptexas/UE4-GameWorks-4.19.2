// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "HttpManager.h"
#include "IAnalyticsProvider.h"

// FHttpManager

FCriticalSection FHttpManager::RequestLock;

FHttpManager::FHttpManager()
	:	FTickerObjectBase(0.0f)
	,	DeferredDestroyDelay(10.0f)
{
	
}

FHttpManager::~FHttpManager()
{
	
}

bool FHttpManager::Tick(float DeltaSeconds)
{
	FScopeLock ScopeLock(&RequestLock);

	// Tick each active request
	for (auto It = Requests.CreateIterator(); It; ++It)
	{
		const FActiveHttpRequest& ActiveRequest = It.Value();
		ActiveRequest.HttpRequest->Tick(DeltaSeconds);
	}
	// Tick any pending destroy objects
	for (int Idx=0; Idx < PendingDestroyRequests.Num(); Idx++)
	{
		FRequestPendingDestroy& Request = PendingDestroyRequests[Idx];
		Request.TimeLeft -= DeltaSeconds;
		if (Request.TimeLeft <= 0)
		{	
			PendingDestroyRequests.RemoveAt(Idx--);
		}		
	}
	// keep ticking
	return true;
}

void FHttpManager::AddRequest(const TSharedRef<IHttpRequest>& Request)
{
	FScopeLock ScopeLock(&RequestLock);

	Requests.Add(&Request.Get(), FActiveHttpRequest(FPlatformTime::Seconds(), Request));
}

void FHttpManager::RemoveRequest(const TSharedRef<IHttpRequest>& Request)
{
	FScopeLock ScopeLock(&RequestLock);

	// Keep track of requests that have been removed to be destroyed later
	PendingDestroyRequests.AddUnique(FRequestPendingDestroy(DeferredDestroyDelay,Request));

	// add analytics info before removing the event
	const FActiveHttpRequest* Found = Requests.Find(&Request.Get());
	if (Found != NULL)
	{
		AddAnalytics(FPlatformTime::Seconds() - Found->StartTime, Request);
	}
	Requests.Remove(&Request.Get());
}

bool FHttpManager::IsValidRequest(const IHttpRequest* RequestPtr) const
{
	FScopeLock ScopeLock(&RequestLock);

	return Requests.Contains(RequestPtr);
}

void FHttpManager::DumpRequests(FOutputDevice& Ar) const
{
	FScopeLock ScopeLock(&RequestLock);
	
	Ar.Logf(TEXT("------- (%d) Http Requests"), Requests.Num());
	for (auto It = Requests.CreateConstIterator(); It; ++It)
	{
		const FActiveHttpRequest& ActiveRequest = It.Value();
		Ar.Logf(TEXT("	verb=[%s] url=[%s] status=%s"),
			*ActiveRequest.HttpRequest->GetVerb(), *ActiveRequest.HttpRequest->GetURL(), EHttpRequestStatus::ToString(ActiveRequest.HttpRequest->GetStatus()));
	}
}

void FHttpManager::FAnalyticsHttpRequest::RecordRequest(double ElapsedTime, const TSharedRef<IHttpRequest>& HttpRequest)
{
	// keep a histogram of response codes
	int32 ResponseCode = HttpRequest->GetResponse()->GetResponseCode();
	int32* ResponseCodeEntry = ResponseCodes.Find(ResponseCode);
	if (ResponseCodeEntry == NULL)
	{
		ResponseCodeEntry = &ResponseCodes.Add(ResponseCode, 0);
	}
	(*ResponseCodeEntry)++;
	// sum elapsed time for average calc
	ElapsedTimeTotal += ElapsedTime;
	ElapsedTimeMax = ElapsedTimeMax > 0 ? FMath::Max<double>(ElapsedTimeMax, ElapsedTime) : ElapsedTime;
	ElapsedTimeMin = ElapsedTimeMin > 0 ? FMath::Min<double>(ElapsedTimeMin, ElapsedTime) : ElapsedTime;
	// sum download rate for average calc
	const double DlSize = HttpRequest->GetResponse()->GetContent().Num() / 1024.f;
	DownloadRateTotal += DlSize / ElapsedTime;
	// keep track of total # of requests
	Count++;
}

static FString GetUrlDomain(const FString& Url)
{
	FString Protocol;
	FString Domain = Url;
	// split the http protocol portion from domain
	Url.Split(TEXT("://"), &Protocol, &Domain);
	// strip off everything but the domain portion
	int32 Idx = Domain.Find(TEXT("/"));
	int32 IdxOpt = Domain.Find(TEXT("?"));
	Idx = IdxOpt >= 0 && IdxOpt < Idx ? IdxOpt : Idx;
	if (Idx > 0)
	{
		Domain = Domain.Left(Idx);
	}
	return Domain;
}

void FHttpManager::AddAnalytics(double ElapsedTime, const TSharedRef<IHttpRequest>& HttpRequest)
{
	// only record events that actually received a response
	if (HttpRequest->GetResponse().IsValid())
	{
		// add/update analytics entry for this domain
		FString Domain = GetUrlDomain(HttpRequest->GetURL());
		FAnalyticsHttpRequest& AnalyticsEntry = RequestAnalytics.FindOrAdd(Domain);
		AnalyticsEntry.RecordRequest(ElapsedTime, HttpRequest);
	}
}

void FHttpManager::FlushAnalytics(class IAnalyticsProvider& Analytics)
{
	for (auto It = RequestAnalytics.CreateConstIterator(); It; ++It)
	{
		const FString& Domain = It.Key();
		const FAnalyticsHttpRequest& AnalyticsEntry = It.Value();

		TArray<FAnalyticsEventAttribute> Attributes;
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Domain"), Domain));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("AvgTime"), AnalyticsEntry.ElapsedTimeTotal / AnalyticsEntry.Count));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("MinTime"), AnalyticsEntry.ElapsedTimeMin));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("MaxTime"), AnalyticsEntry.ElapsedTimeMax));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("AvgRate"), AnalyticsEntry.DownloadRateTotal / AnalyticsEntry.Count));
		Attributes.Add(FAnalyticsEventAttribute(TEXT("Hits"), AnalyticsEntry.Count));
		for (auto CodeIt = AnalyticsEntry.ResponseCodes.CreateConstIterator(); CodeIt; ++CodeIt)
		{
			Attributes.Add(FAnalyticsEventAttribute(FString::Printf(TEXT("Code-%d"), CodeIt.Key()), CodeIt.Value()));
		}
		Analytics.RecordEvent(TEXT("Http.Services"), Attributes);
	}
	RequestAnalytics.Empty();
}
