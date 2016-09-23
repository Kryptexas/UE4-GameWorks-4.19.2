// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "HttpThread.h"

// FHttpThread

FHttpThread::FHttpThread()
	:	Thread(nullptr)
{
	HttpThreadProcessingClampInSeconds = FHttpModule::Get().GetHttpThreadProcessingClampInSeconds();
	HttpThreadFrameTimeInSeconds = FHttpModule::Get().GetHttpThreadFrameTimeInSeconds();
	HttpThreadMinimumSleepTimeInSeconds = FHttpModule::Get().GetHttpThreadMinimumSleepTimeInSeconds();

	UE_LOG(LogHttp, Log, TEXT("HTTP thread processing clamped at %.1f ms. HTTP thread frame time clamped to %.1f ms. Minimum sleep time is %.1f ms"), HttpThreadProcessingClampInSeconds * 1000.0, HttpThreadFrameTimeInSeconds * 1000.0, HttpThreadMinimumSleepTimeInSeconds * 1000.0);
}

FHttpThread::~FHttpThread()
{
	StopThread();
}

void FHttpThread::StartThread()
{
	Thread = FRunnableThread::Create(this, TEXT("HttpManagerThread"), 128 * 1024, TPri_Normal);
}

void FHttpThread::StopThread()
{
	if (Thread != nullptr)
	{
		Thread->Kill(true);
		delete Thread;
		Thread = nullptr;
	}
}

void FHttpThread::AddRequest(IHttpThreadedRequest* Request)
{
	FScopeLock ScopeLock(&RequestArraysLock);
	PendingThreadedRequests.Add(Request);
}

void FHttpThread::CancelRequest(IHttpThreadedRequest* Request)
{
	FScopeLock ScopeLock(&RequestArraysLock);
	CancelledThreadedRequests.Add(Request);
}

void FHttpThread::GetCompletedRequests(TArray<IHttpThreadedRequest*>& OutCompletedRequests)
{
	FScopeLock ScopeLock(&RequestArraysLock);
	OutCompletedRequests = CompletedThreadedRequests;
	CompletedThreadedRequests.Reset();
}

bool FHttpThread::Init()
{
	return true;
}

void FHttpThread::HttpThreadTick(float DeltaSeconds)
{
	// empty
}

bool FHttpThread::StartThreadedRequest(IHttpThreadedRequest* Request)
{
	return Request->StartThreadedRequest();
}

void FHttpThread::CompleteThreadedRequest(IHttpThreadedRequest* Request)
{
	// empty
}

uint32 FHttpThread::Run()
{
	double LastTime = FPlatformTime::Seconds();
	TArray<IHttpThreadedRequest*> RequestsToCancel;
	TArray<IHttpThreadedRequest*> RequestsToStart;
	TArray<IHttpThreadedRequest*> RequestsToComplete;
	while (!ExitRequest.GetValue())
	{
		double TickBegin = FPlatformTime::Seconds();
		double TickEnd = 0.0;
		bool bKeepProcessing = true;
		while (bKeepProcessing)
		{
			double InterationBegin = FPlatformTime::Seconds();

			// cache all cancelled and pending requests
			{
				FScopeLock ScopeLock(&RequestArraysLock);

				RequestsToCancel = CancelledThreadedRequests;
				CancelledThreadedRequests.Reset();

				RequestsToStart = PendingThreadedRequests;
				PendingThreadedRequests.Reset();
			}

			// Cancel any pending cancel requests
			for (IHttpThreadedRequest* Request : RequestsToCancel)
			{
				if (RunningThreadedRequests.Remove(Request) > 0)
				{
					RequestsToComplete.Add(Request);
				}
			}

			// Start any pending requests
			for (IHttpThreadedRequest* Request : RequestsToStart)
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
			const double ElapsedTime = AppTime - LastTime;
			LastTime = AppTime;

			// Tick any running requests
			for (int32 Index = 0; Index < RunningThreadedRequests.Num(); ++Index)
			{
				IHttpThreadedRequest* Request = RunningThreadedRequests[Index];
				Request->TickThreadedRequest(ElapsedTime);
			}

			HttpThreadTick(ElapsedTime);

			// Move any completed requests
			for (int32 Index = 0; Index < RunningThreadedRequests.Num(); ++Index)
			{
				IHttpThreadedRequest* Request = RunningThreadedRequests[Index];
				if (Request->IsThreadedRequestComplete())
				{
					RequestsToComplete.Add(Request);
					RunningThreadedRequests.RemoveAtSwap(Index);
					--Index;
				}
			}

			if (RequestsToComplete.Num() > 0)
			{
				for (IHttpThreadedRequest* Request : RequestsToComplete)
				{
					CompleteThreadedRequest(Request);
				}

				{
					FScopeLock ScopeLock(&RequestArraysLock);
					CompletedThreadedRequests.Append(RequestsToComplete);
				}
				RequestsToComplete.Reset();
			}

			double TimeNow = FPlatformTime::Seconds();
			double TimeWithoutSleeping = TimeNow - TickBegin;
			bool bEndit = false;
			if (RunningThreadedRequests.Num() == 0)
			{
				bEndit = true;
			}

			if (HttpThreadProcessingClampInSeconds > 0.0 && TimeWithoutSleeping >= HttpThreadProcessingClampInSeconds)
			{
				bEndit = true;
			}

			if (bEndit)
			{
				bKeepProcessing = false;
				TickEnd = TimeNow;
			}
		}

		double TickTime = TickEnd - TickBegin;
		double TimeToSleep = HttpThreadMinimumSleepTimeInSeconds;
		if (HttpThreadFrameTimeInSeconds > 0.0 && HttpThreadFrameTimeInSeconds >= TickTime)
		{
			TimeToSleep = FMath::Max(HttpThreadFrameTimeInSeconds - TickTime, TimeToSleep);
		}

		if (TimeToSleep > 0.0)
		{
			FPlatformProcess::SleepNoStats(TimeToSleep);
		}
	}
	return 0;
}

void FHttpThread::Stop()
{
	ExitRequest.Set(true);
}
	
void FHttpThread::Exit()
{
	// empty
}
