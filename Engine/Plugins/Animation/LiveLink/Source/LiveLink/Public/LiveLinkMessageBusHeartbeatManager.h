// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runnable.h"
#include "RunnableThread.h"
#include "ThreadSafeBool.h"
#include "CriticalSection.h"

class FLiveLinkMessageBusSource;

// A class to asynchronously send heartbeats to a set of message bus sources
class FHeartbeatManager : FRunnable
{
public:
	// Singleton access to the Heartbeat Manager
	static FHeartbeatManager* Get();

	~FHeartbeatManager();

	// Begin FRunnable interface

	virtual uint32 Run() override;

	virtual void Stop() override;

	// End FRunnable interface

	void RegisterSource(FLiveLinkMessageBusSource* const InSource);
	void RemoveSource(FLiveLinkMessageBusSource* const InSource);

	bool IsRunning() const;

private:
	FHeartbeatManager();

	// Singleton instance
	static FHeartbeatManager* Instance;

	// Thread safe bool for stopping the thread
	FThreadSafeBool bRunning;

	// Thread the heartbeats are sent on
	FRunnableThread* Thread;

	// Critical section for accessing the Source Set
	FCriticalSection SourcesCriticalSection;

	// Set of sources we are currently sending heartbeats for
	TSet<FLiveLinkMessageBusSource*> MessageBusSources;
};