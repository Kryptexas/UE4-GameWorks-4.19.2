// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "HttpPrivatePCH.h"
#include "LinuxHttpManager.h"
#include "CurlHttp.h"

FLinuxHttpManager::FLinuxHttpManager(CURLM * InMultiHandle)
	:	FHttpManager()
	,	MultiHandle(InMultiHandle)
	,	LastRunningRequests(0)
{
	check(MultiHandle);
}

// note that we cannot call parent implementation because lock might be possible non-multiple
void FLinuxHttpManager::AddRequest(TSharedRef<class IHttpRequest> Request)
{
	FScopeLock ScopeLock(&RequestLock);

	Requests.AddUnique(Request);

	FCurlHttpRequest* CurlRequest = static_cast< FCurlHttpRequest* >( &Request.Get() );
	HandlesToRequests.Add(CurlRequest->GetEasyHandle(), Request);
}

// note that we cannot call parent implementation because lock might be possible non-multiple
void FLinuxHttpManager::RemoveRequest(TSharedRef<class IHttpRequest> Request)
{
	FScopeLock ScopeLock(&RequestLock);

	// Keep track of requests that have been removed to be destroyed later
	PendingDestroyRequests.AddUnique(FRequestPendingDestroy(DeferredDestroyDelay,Request));

	FCurlHttpRequest* CurlRequest = static_cast< FCurlHttpRequest* >( &Request.Get() );
	HandlesToRequests.Remove(CurlRequest->GetEasyHandle());

	Requests.RemoveSingle(Request);
}

bool FLinuxHttpManager::Tick(float DeltaSeconds)
{
	check(MultiHandle);
	if (Requests.Num() > 0)
	{
		FScopeLock ScopeLock(&RequestLock);

		int RunningRequests = -1;
		curl_multi_perform(MultiHandle, &RunningRequests);

		// read more info if number of requests changed or if there's zero running
		// (note that some requests might have never be "running" from libcurl's point of view)
		if (RunningRequests == 0 || RunningRequests != LastRunningRequests)
		{
			for(;;)
			{
				int MsgsStillInQueue = 0;	// may use that to impose some upper limit we may spend in that loop
				CURLMsg * Message = curl_multi_info_read(MultiHandle, &MsgsStillInQueue);

				if (Message == NULL)
				{
					break;
				}

				// find out which requests have completed
				if (Message->msg == CURLMSG_DONE)
				{
					CURL* CompletedHandle = Message->easy_handle;
					TSharedRef<IHttpRequest> * RequestRefPtr = HandlesToRequests.Find(CompletedHandle);
					if (RequestRefPtr)
					{
						FCurlHttpRequest* CurlRequest = static_cast< FCurlHttpRequest* >( &RequestRefPtr->Get() );
						CurlRequest->MarkAsCompleted(Message->data.result);

						UE_LOG(LogHttp, Verbose, TEXT("Request %p (easy handle:%p) has completed (code:%d) and has been marked as such"), CurlRequest, CompletedHandle, Message->data.result);
					}
					else
					{
						UE_LOG(LogHttp, Warning, TEXT("Could not find mapping for completed request (easy handle: %p)"), CompletedHandle);
					}
				}
			}
		}

		LastRunningRequests = RunningRequests;
	}

	// we should be outside scope lock here to be able to call parent!
	return FHttpManager::Tick(DeltaSeconds);
}
