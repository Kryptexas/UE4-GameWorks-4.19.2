// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalRHIPrivate.h: Private Metal RHI definitions.
=============================================================================*/

#pragma once

#include "Engine.h"

// UE4 has a Max of 8 RTs, but we max out at 4, no need to waste time processing 8
// @todo urban: Currently only using 1, so just do 1!
const uint32 MaxMetalRenderTargets = 1;

// How many possible vertex streams are allowed
const uint32 MaxMetalStreams = 16;

// Requirement for vertex buffer offset field
const uint32 BufferOffsetAlignment = 256;

//#define BUFFER_CACHE_MODE MTLResourceOptionCPUCacheModeDefault
#define BUFFER_CACHE_MODE MTLResourceOptionCPUCacheModeWriteCombined

#define GPU_TIMINGS 0 // (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)

#define SHOULD_TRACK_OBJECTS 0 // (UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT)

#define UNREAL_TO_METAL_BUFFER_INDEX(Index) (15 - Index)

// Dependencies
#include "MetalRHI.h"
#include "MetalGlobalUniformBuffer.h"
#include "RHI.h"
#include "MetalManager.h"


// Access the underlying surface object from any kind of texture
FMetalSurface& GetMetalSurfaceFromRHITexture(FRHITexture* Texture);

#define NOT_SUPPORTED(Func) UE_LOG(LogMetal, Fatal, TEXT("'%s' is not supported"), L##Func);

#if SHOULD_TRACK_OBJECTS
extern TMap<id, int32> ClassCounts;
#define TRACK_OBJECT(Obj) ClassCounts.FindOrAdd([Obj class])++;
#define UNTRACK_OBJECT(Obj) ClassCounts.FindOrAdd([Obj class])--;
#else
#define TRACK_OBJECT(Obj)
#define UNTRACK_OBJECT(Obj)
#endif






class FLocalEvent : public FEvent
{
	// This is a little complicated, in an attempt to match Win32 Event semantics...
    typedef enum
    {
        TRIGGERED_NONE,
        TRIGGERED_ONE,
        TRIGGERED_ALL,
    } TriggerType;
	
	bool bInitialized;
	bool bIsManualReset;
	volatile TriggerType Triggered;
	volatile int32 WaitingThreads;
    pthread_mutex_t Mutex;
    pthread_cond_t Condition;
	
	inline void LockEventMutex()
	{
		const int rc = pthread_mutex_lock(&Mutex);
		check(rc == 0);
	}
	
	inline void UnlockEventMutex()
	{
		const int rc = pthread_mutex_unlock(&Mutex);
		check(rc == 0);
	}
	
	static inline void SubtractTimevals(const struct timeval *FromThis, struct timeval *SubThis, struct timeval *Difference)
	{
		if (FromThis->tv_usec < SubThis->tv_usec)
		{
			int nsec = (SubThis->tv_usec - FromThis->tv_usec) / 1000000 + 1;
			SubThis->tv_usec -= 1000000 * nsec;
			SubThis->tv_sec += nsec;
		}
		
		if (FromThis->tv_usec - SubThis->tv_usec > 1000000)
		{
			int nsec = (FromThis->tv_usec - SubThis->tv_usec) / 1000000;
			SubThis->tv_usec += 1000000 * nsec;
			SubThis->tv_sec -= nsec;
		}
		
		Difference->tv_sec = FromThis->tv_sec - SubThis->tv_sec;
		Difference->tv_usec = FromThis->tv_usec - SubThis->tv_usec;
	}
	
public:
	
	FLocalEvent()
	{
		bInitialized = false;
		bIsManualReset = false;
		Triggered = TRIGGERED_NONE;
		WaitingThreads = 0;
	}
	
	virtual ~FLocalEvent()
	{
		// Safely destructing an Event is VERY delicate, so it can handle badly-designed
		//  calling code that might still be waiting on the event.
		if (bInitialized)
		{
			// try to flush out any waiting threads...
			LockEventMutex();
			bIsManualReset = true;
			UnlockEventMutex();
			Trigger();  // any waiting threads should start to wake up now.
			
			LockEventMutex();
			bInitialized = false;  // further incoming calls to this object will now crash in check().
			while (WaitingThreads)  // spin while waiting threads wake up.
			{
				UnlockEventMutex();  // cycle through waiting threads...
				LockEventMutex();
			}
			// No threads are currently waiting on Condition and we hold the Mutex. Kill it.
			pthread_cond_destroy(&Condition);
			// Unlock and kill the mutex, since nothing else can grab it now.
			UnlockEventMutex();
			pthread_mutex_destroy(&Mutex);
		}
	}
	
	virtual bool Create(bool _bIsManualReset = false) override
	{
		check(!bInitialized);
		bool RetVal = false;
		Triggered = TRIGGERED_NONE;
		bIsManualReset = _bIsManualReset;
		
		if (pthread_mutex_init(&Mutex, NULL) == 0)
		{
			if (pthread_cond_init(&Condition, NULL) == 0)
			{
				bInitialized = true;
				RetVal = true;
			}
			else
			{
				pthread_mutex_destroy(&Mutex);
			}
		}
		return RetVal;
	}
	
	virtual void Trigger() override
	{
		check(bInitialized);
		
		LockEventMutex();
		
		if (bIsManualReset)
		{
			// Release all waiting threads at once.
			Triggered = TRIGGERED_ALL;
			int rc = pthread_cond_broadcast(&Condition);
			check(rc == 0);
		}
		else
		{
			// Release one or more waiting threads (first one to get the mutex
			//  will reset Triggered, rest will go back to waiting again).
			Triggered = TRIGGERED_ONE;
			int rc = pthread_cond_signal(&Condition);  // may release multiple threads anyhow!
			check(rc == 0);
		}
		
		UnlockEventMutex();
	}
	
	virtual void Reset() override
	{
		check(bInitialized);
		LockEventMutex();
		Triggered = TRIGGERED_NONE;
		UnlockEventMutex();
	}
	
	virtual bool Wait(uint32 WaitTime)
	{
		check(bInitialized);
		
		struct timeval StartTime;
		
		// We need to know the start time if we're going to do a timed wait.
		if ( (WaitTime > 0) && (WaitTime != ((uint32)-1)) )  // not polling and not infinite wait.
		{
			gettimeofday(&StartTime, NULL);
		}
		
		LockEventMutex();
		
		bool bRetVal = false;
		
		// loop in case we fall through the Condition signal but someone else claims the event.
		do
		{
			// See what state the event is in...we may not have to wait at all...
			
			// One thread should be released. We saw it first, so we'll take it.
			if (Triggered == TRIGGERED_ONE)
			{
				Triggered = TRIGGERED_NONE;  // dibs!
				bRetVal = true;
			}
			
			// manual reset that is still signaled. Every thread goes.
			else if (Triggered == TRIGGERED_ALL)
			{
				bRetVal = true;
			}
			
			// No event signalled yet.
			else if (WaitTime != 0)  // not just polling, wait on the condition variable.
			{
				WaitingThreads++;
				if (WaitTime == ((uint32)-1)) // infinite wait?
				{
					int rc = pthread_cond_wait(&Condition, &Mutex);  // unlocks Mutex while blocking...
					check(rc == 0);
				}
				else  // timed wait.
				{
					struct timespec TimeOut;
					const uint32 ms = (StartTime.tv_usec / 1000) + WaitTime;
					TimeOut.tv_sec = StartTime.tv_sec + (ms / 1000);
					TimeOut.tv_nsec = (ms % 1000) * 1000000;  // remainder of milliseconds converted to nanoseconds.
					int rc = pthread_cond_timedwait(&Condition, &Mutex, &TimeOut);    // unlocks Mutex while blocking...
					check((rc == 0) || (rc == ETIMEDOUT));
					
					// Update WaitTime and StartTime in case we have to go again...
					struct timeval Now, Difference;
					gettimeofday(&Now, NULL);
					SubtractTimevals(&Now, &StartTime, &Difference);
					const int32 DifferenceMS = ((Difference.tv_sec * 1000) + (Difference.tv_usec / 1000));
					WaitTime = ((DifferenceMS >= WaitTime) ? 0 : (WaitTime - DifferenceMS));
					StartTime = Now;
				}
				WaitingThreads--;
				check(WaitingThreads >= 0);
			}
			
		} while ((!bRetVal) && (WaitTime != 0));
		
		UnlockEventMutex();
		return bRetVal;
	}
};
