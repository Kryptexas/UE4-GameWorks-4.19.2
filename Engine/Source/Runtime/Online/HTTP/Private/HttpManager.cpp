// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "HttpManager.h"

// FHttpManager

FCriticalSection FHttpManager::RequestLock;

FHttpManager::FHttpManager()
	:	FTickerObjectBase(0.0f)
	,	HttpThreadTickBudget(0.033)
	,	DeferredDestroyDelay(10.0f)
{
	double TargetTickRate = FHttpModule::Get().GetHttpThreadTickRate();
	if (TargetTickRate > 0.0)
	{
		HttpThreadTickBudget = 1.0 / TargetTickRate;
	}

	UE_LOG(LogHttp, Log, TEXT("HTTP thread tick budget is %.1f ms (tick rate is %f)"), HttpThreadTickBudget, TargetTickRate);

	Thread = FRunnableThread::Create(this, TEXT("HttpManagerThread"), 1024 * 1024, TPri_Normal);
}

FHttpManager::~FHttpManager()
{
	if (Thread != nullptr)
	{
		Thread->Kill(true);
		delete Thread;
	}
}

void FHttpManager::Flush(bool bShutdown)
{
	bool bHasRequests = false;
	
	{
		FScopeLock ScopeLock(&RequestLock);
		if (bShutdown)
		{
			if (Requests.Num())
			{
				UE_LOG(LogHttp, Display, TEXT("Http module shutting down, but needs to wait on %d outstanding Http requests:"), Requests.Num());
			}
			// Clear delegates since they may point to deleted instances
			for (TArray<TSharedRef<IHttpRequest>>::TIterator It(Requests); It; ++It)
			{
				TSharedRef<IHttpRequest> Request = *It;
				Request->OnProcessRequestComplete().Unbind();
				Request->OnRequestProgress().Unbind();
				UE_LOG(LogHttp, Display, TEXT("	verb=[%s] url=[%s] status=%s"), *Request->GetVerb(), *Request->GetURL(), EHttpRequestStatus::ToString(Request->GetStatus()));
			}
		}
		bHasRequests = Requests.Num() > 0;
	}

	// block until all active requests have completed
	double LastTime = FPlatformTime::Seconds();
	while (bHasRequests)
	{	
		{
			FScopeLock ScopeLock(&RequestLock);
			const double AppTime = FPlatformTime::Seconds();
			Tick(AppTime - LastTime);
			LastTime = AppTime;
			bHasRequests = Requests.Num() > 0;
		}
		if (bHasRequests)
		{
			UE_LOG(LogHttp, Display, TEXT("Sleeping 0.5s to wait for %d outstanding Http requests."), Requests.Num());
			FPlatformProcess::Sleep(0.5f);
		}
	}
	if (bShutdown)
	{
		// Stop the http thread
		Stop();
	}
}

bool FHttpManager::Tick(float DeltaSeconds)
{
	FScopeLock ScopeLock(&RequestLock);

	// Tick each active request
	for (TArray<TSharedRef<IHttpRequest>>::TIterator It(Requests); It; ++It)
	{
		TSharedRef<IHttpRequest> Request = *It;
		Request->Tick(DeltaSeconds);
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
	// Finish and remove any completed requests
	for (IHttpThreadedRequest* CompletedRequest : CompletedThreadedRequests)
	{
		TSharedRef<IHttpRequest> CompletedRequestRef(CompletedRequest->AsShared());
		Requests.Remove(CompletedRequestRef);

		// Keep track of requests that have been removed to be destroyed later
		PendingDestroyRequests.AddUnique(FRequestPendingDestroy(DeferredDestroyDelay,CompletedRequestRef));

		CompletedRequest->FinishRequest();
	}
	CompletedThreadedRequests.Empty();
	// keep ticking
	return true;
}

void FHttpManager::AddRequest(const TSharedRef<IHttpRequest>& Request)
{
	FScopeLock ScopeLock(&RequestLock);

	Requests.Add(Request);
}

void FHttpManager::RemoveRequest(const TSharedRef<IHttpRequest>& Request)
{
	FScopeLock ScopeLock(&RequestLock);

	// Keep track of requests that have been removed to be destroyed later
	PendingDestroyRequests.AddUnique(FRequestPendingDestroy(DeferredDestroyDelay,Request));

	Requests.Remove(Request);
}

void FHttpManager::AddThreadedRequest(const TSharedRef<IHttpThreadedRequest>& Request)
{
	FScopeLock ScopeLock(&RequestLock);

	Requests.Add(Request);

	PendingThreadedRequests.Add(&Request.Get());
}

void FHttpManager::CancelThreadedRequest(const TSharedRef<IHttpThreadedRequest>& Request)
{
	FScopeLock ScopeLock(&RequestLock);
	if (Requests.Contains(Request))
	{
		CancelledThreadedRequests.Add(&Request.Get());
	}
}

bool FHttpManager::IsValidRequest(const IHttpRequest* RequestPtr) const
{
	FScopeLock ScopeLock(&RequestLock);

	bool bResult = false;
	for (auto& Request : Requests)
	{
		if (&Request.Get() == RequestPtr)
		{
			bResult = true;
			break;
		}
	}
	if (!bResult)
	{
		bResult = CompletedThreadedRequests.Contains(RequestPtr);
	}

	return bResult;
}

void FHttpManager::DumpRequests(FOutputDevice& Ar) const
{
	FScopeLock ScopeLock(&RequestLock);
	
	Ar.Logf(TEXT("------- (%d) Http Requests"), Requests.Num());
	for (auto& Request : Requests)
	{
		Ar.Logf(TEXT("	verb=[%s] url=[%s] status=%s"),
			*Request->GetVerb(), *Request->GetURL(), EHttpRequestStatus::ToString(Request->GetStatus()));
	}
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

bool FHttpManager::Init()
{
	return true;
}

void FHttpManager::HttpThreadTick(float DeltaSeconds)
{
	// empty
}

bool FHttpManager::StartThreadedRequest(IHttpThreadedRequest* Request)
{
	return Request->StartThreadedRequest();
}

void FHttpManager::CompleteThreadedRequest(IHttpThreadedRequest* Request)
{
	// empty
}

uint32 FHttpManager::Run()
{
	double LastTime = FPlatformTime::Seconds();
	TArray<IHttpThreadedRequest*> RequestsToCancel;
	TArray<IHttpThreadedRequest*> RequestsToStart;
	TArray<IHttpThreadedRequest*> RequestsToComplete;
	while (!ExitRequest.GetValue())
	{
		double TickBegin = FPlatformTime::Seconds();

		{
			FScopeLock ScopeLock(&RequestLock);

			RequestsToCancel = CancelledThreadedRequests;
			CancelledThreadedRequests.Empty();

			RequestsToStart = PendingThreadedRequests;
			PendingThreadedRequests.Empty();
		}

		// Cancel any pending cancel requests
		for (auto& Request : RequestsToCancel)
		{
			if (RunningThreadedRequests.Remove(Request) > 0)
			{
				RequestsToComplete.Add(Request);
			}
		}
		// Start any pending requests
		for (auto& Request : RequestsToStart)
		{
			if (StartThreadedRequest(Request))
			{
				RunningThreadedRequests.Add(Request);
			}
			else
			{
				RequestsToComplete.Add(Request);
			}
		}
		const double AppTime = FPlatformTime::Seconds();
		// Tick any running requests
		for (int Index = 0; Index < RunningThreadedRequests.Num(); ++Index)
		{
			auto& Request = RunningThreadedRequests[Index];
			Request->TickThreadedRequest(AppTime - LastTime);
		}
		HttpThreadTick(AppTime - LastTime);
		// Move any completed requests
		for (int Index = 0; Index < RunningThreadedRequests.Num(); ++Index)
		{
			auto& Request = RunningThreadedRequests[Index];
			if (Request->IsThreadedRequestComplete())
			{
				RequestsToComplete.Add(Request);
				RunningThreadedRequests.RemoveAtSwap(Index);
				--Index;
			}
		}
		LastTime = AppTime;

		if (RequestsToComplete.Num() > 0)
		{
			for (auto& Request : RequestsToComplete)
			{
				CompleteThreadedRequest(Request);
			}

			{
				FScopeLock ScopeLock(&RequestLock);
				CompletedThreadedRequests.Append(RequestsToComplete);
			}
			RequestsToComplete.Empty();
		}

		double TickDuration = (FPlatformTime::Seconds() - TickBegin);
		double WaitTime = FMath::Max(0.0, HttpThreadTickBudget - TickDuration);
		FPlatformProcess::SleepNoStats(WaitTime);
	}
	return 0;
}

void FHttpManager::Stop()
{
	ExitRequest.Set(true);
}
	
void FHttpManager::Exit()
{
	// empty
}
