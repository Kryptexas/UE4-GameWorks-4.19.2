// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkMessageBusHeartbeatManager.h"
#include "LiveLinkMessageBusSource.h"
#include "ScopeLock.h"
#include "Async.h"

#define LL_HEARTBEAT_SLEEP_TIME 1.0f

FHeartbeatManager* FHeartbeatManager::Instance = nullptr;

FHeartbeatManager::FHeartbeatManager() :
	bRunning(true)
{
	Thread = FRunnableThread::Create(this, TEXT("MessageBusHeartbeatManager"));
}

FHeartbeatManager::	~FHeartbeatManager()
{
	Stop();
	Thread->WaitForCompletion();
};

FHeartbeatManager* FHeartbeatManager::Get()
{
	if (Instance == nullptr)
	{
		Instance = new FHeartbeatManager();
	}
	return Instance;
}

uint32 FHeartbeatManager::Run()
{
	while (bRunning)
	{
		{
			FScopeLock Lock(&SourcesCriticalSection);

			TArray<FLiveLinkMessageBusSource*> SourcesToRemove;

			// Loop through all sources and send heartbeat
			for (const auto& MessageBusSource : MessageBusSources)
			{
				if (MessageBusSource != nullptr)
				{
					bool bStillValid = MessageBusSource->IsSourceStillValid();
					if (bStillValid)
					{
						bStillValid = MessageBusSource->SendHeartbeat();
					}

					if (!bStillValid)
					{
						// Source is invalid, queue it for removal
						SourcesToRemove.Emplace(MessageBusSource);
					}
				}
			}

			// Remove any dead sources
			for (const auto& SourceToRemove : SourcesToRemove)
			{
				MessageBusSources.Remove(SourceToRemove);
			}
		}
		FPlatformProcess::Sleep(LL_HEARTBEAT_SLEEP_TIME);
	}
	return 0;
};

void FHeartbeatManager::Stop()
{
	bRunning = false;
}

void FHeartbeatManager::RegisterSource(FLiveLinkMessageBusSource* const InSource)
{
	FScopeLock Lock(&SourcesCriticalSection);

	MessageBusSources.Add(InSource);
};

void FHeartbeatManager::RemoveSource(FLiveLinkMessageBusSource* const InSource)
{
	FScopeLock Lock(&SourcesCriticalSection);

	MessageBusSources.Remove(InSource);
};

bool FHeartbeatManager::IsRunning() const
{
	return bRunning;
};
