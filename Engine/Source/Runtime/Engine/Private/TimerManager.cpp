// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TickTaskManager.cpp: Manager for ticking tasks
=============================================================================*/

#include "EnginePrivate.h"


// ---------------------------------
// Private members
// ---------------------------------

/** Will find and return a timer if it exists, regardless whether it is paused. */ 
FTimerData const* FTimerManager::FindTimer(FTimerUnifiedDelegate const& InDelegate, int32* OutTimerIndex) const
{
	int32 ActiveTimerIdx = FindTimerInList(ActiveTimerHeap, InDelegate);
	if (ActiveTimerIdx != INDEX_NONE)
	{
		// found it in the active heap
		if (OutTimerIndex)
		{
			*OutTimerIndex = ActiveTimerIdx;
		}
		return &ActiveTimerHeap[ActiveTimerIdx];
	}

	int32 PausedTimerIdx = FindTimerInList(PausedTimerList, InDelegate);
	if (PausedTimerIdx != INDEX_NONE)
	{
		// found it in the paused list
		if (OutTimerIndex)
		{
			*OutTimerIndex = PausedTimerIdx;
		}
		return &PausedTimerList[PausedTimerIdx];
	}

	int32 PendingTimerIdx = FindTimerInList(PendingTimerList, InDelegate);
	if (PendingTimerIdx != INDEX_NONE)
	{
		// found it in the paused list
		if (OutTimerIndex)
		{
			*OutTimerIndex = PendingTimerIdx;
		}
		return &PendingTimerList[PendingTimerIdx];
	}

	return NULL;
}


/** Will find the given timer in the given TArray and return its index. */ 
int32 FTimerManager::FindTimerInList(const TArray<FTimerData> &SearchArray, FTimerUnifiedDelegate const& InDelegate) const
{
	for (int32 Idx=0; Idx<SearchArray.Num(); ++Idx)
	{
		if (SearchArray[Idx].TimerDelegate == InDelegate)
		{
			return Idx;
		}
	}

	return INDEX_NONE;
}

void FTimerManager::InternalSetTimer(FTimerUnifiedDelegate const& InDelegate, float InRate, bool InbLoop)
{
	// not currently threadsafe
	check(IsInGameThread());

	// if the timer is already set, just clear it and we'll re-add it, since 
	// there's no data to maintain.
	InternalClearTimer(InDelegate);

	if (InRate > 0.f)
	{
		// set up the new timer
		FTimerData NewTimerData;
		NewTimerData.Rate = InRate;
		NewTimerData.bLoop = InbLoop;
		NewTimerData.TimerDelegate = InDelegate;
		
		if( HasBeenTickedThisFrame() )
		{
			NewTimerData.ExpireTime = InternalTime + InRate;
			NewTimerData.Status = ETimerStatus::Active;
			ActiveTimerHeap.HeapPush(NewTimerData);
		}
		else
		{
			// Store time remaining in ExpireTime while pending
			NewTimerData.ExpireTime = InRate;
			NewTimerData.Status = ETimerStatus::Pending;
			PendingTimerList.Add(NewTimerData);
		}
	}
}

void FTimerManager::InternalClearTimer(FTimerUnifiedDelegate const& InDelegate)
{
	// not currently threadsafe
	check(IsInGameThread());

	int32 TimerIdx;
	FTimerData const* const TimerData = FindTimer(InDelegate, &TimerIdx);
	if( TimerData )
	{
		switch( TimerData->Status )
		{
			case ETimerStatus::Pending : PendingTimerList.RemoveAtSwap(TimerIdx); break;
			case ETimerStatus::Active : ActiveTimerHeap.HeapRemoveAt(TimerIdx); break;
			case ETimerStatus::Paused : PausedTimerList.RemoveAtSwap(TimerIdx); break;
			default : check(false);
		}
	}
	else
	{
		// Edge case. We're currently handling this timer when it got cleared.  Unbind it to prevent it firing again
		// in case it was scheduled to fire multiple times.
		if (CurrentlyExecutingTimer.TimerDelegate == InDelegate)
		{
			CurrentlyExecutingTimer.TimerDelegate.Unbind();
		}
	}
}


void FTimerManager::InternalClearAllTimers(void const* Object)
{
	if (Object)
	{
		// search active timer heap for timers using this object and remove them
		int32 const OldActiveHeapSize = ActiveTimerHeap.Num();

		for (int32 Idx=0; Idx<ActiveTimerHeap.Num(); ++Idx)
		{
			if ( ActiveTimerHeap[Idx].TimerDelegate.IsBoundToObject(Object) )
			{
				// remove this item
				// this will break the heap property, but we will re-heapify afterward
				ActiveTimerHeap.RemoveAtSwap(Idx--);
			}
		}
		if (OldActiveHeapSize != ActiveTimerHeap.Num())
		{
			// need to heapify
			ActiveTimerHeap.Heapify();
		}

		// search paused timer list for timers using this object and remove them, too
		for (int32 Idx=0; Idx<PausedTimerList.Num(); ++Idx)
		{
			if (PausedTimerList[Idx].TimerDelegate.IsBoundToObject(Object))
			{
				PausedTimerList.RemoveAtSwap(Idx--);
			}
		}

		// search pending timer list for timers using this object and remove them, too
		for (int32 Idx=0; Idx<PendingTimerList.Num(); ++Idx)
		{
			if (PendingTimerList[Idx].TimerDelegate.IsBoundToObject(Object))
			{
				PendingTimerList.RemoveAtSwap(Idx--);
			}
		}
	}
}



float FTimerManager::InternalGetTimerRemaining(FTimerUnifiedDelegate const& InDelegate) const
{
	FTimerData const* const TimerData = FindTimer(InDelegate);
	if (TimerData)
	{
		if( TimerData->Status != ETimerStatus::Active )
		{
			// ExpireTime is time remaining for paused timers
			return TimerData->ExpireTime;
		}
		else
		{
			return TimerData->ExpireTime - InternalTime;
		}
	}

	return -1.f;
}

float FTimerManager::InternalGetTimerElapsed(FTimerUnifiedDelegate const& InDelegate) const
{
	FTimerData const* const TimerData = FindTimer(InDelegate);
	if (TimerData)
	{
		if( TimerData->Status != ETimerStatus::Active)
		{
			// ExpireTime is time remaining for paused timers
			return (TimerData->Rate - TimerData->ExpireTime);
		}
		else
		{
			return (TimerData->Rate - (TimerData->ExpireTime - InternalTime));
		}
	}

	return -1.f;
}

float FTimerManager::InternalGetTimerRate(FTimerUnifiedDelegate const& InDelegate) const
{
	FTimerData const* const TimerData = FindTimer(InDelegate);
	if (TimerData)
	{
		return TimerData->Rate;
	}
	return -1.f;
}

void FTimerManager::InternalPauseTimer(FTimerUnifiedDelegate const& InDelegate)
{
	// not currently threadsafe
	check(IsInGameThread());

	int32 TimerIdx;
	FTimerData const* TimerToPause = FindTimer(InDelegate, &TimerIdx);
	if( TimerToPause && (TimerToPause->Status != ETimerStatus::Paused) )
	{
		ETimerStatus::Type PreviousStatus = TimerToPause->Status;

		// Add to Paused list
		int32 NewIndex = PausedTimerList.Add(*TimerToPause);

		// Set new status
		FTimerData &NewTimer = PausedTimerList[NewIndex];
		NewTimer.Status = ETimerStatus::Paused;

		// Remove from previous TArray
		switch( PreviousStatus )
		{
			case ETimerStatus::Active : 
				// Store time remaining in ExpireTime while paused
				NewTimer.ExpireTime = NewTimer.ExpireTime - InternalTime;
				ActiveTimerHeap.HeapRemoveAt(TimerIdx); 
				break;
			
			case ETimerStatus::Pending : 
				PendingTimerList.RemoveAtSwap(TimerIdx); 
				break;

			default : check(false);
		}
	}
}

void FTimerManager::InternalUnPauseTimer(FTimerUnifiedDelegate const& InDelegate)
{
	// not currently threadsafe
	check(IsInGameThread());

	int32 PausedTimerIdx = FindTimerInList(PausedTimerList, InDelegate);
	if (PausedTimerIdx != INDEX_NONE)
	{
		FTimerData& TimerToUnPause = PausedTimerList[PausedTimerIdx];
		check(TimerToUnPause.Status == ETimerStatus::Paused);

		// Move it out of paused list and into proper TArray
		if( HasBeenTickedThisFrame() )
		{
			// Convert from time remaining back to a valid ExpireTime
			TimerToUnPause.ExpireTime += InternalTime;
			TimerToUnPause.Status = ETimerStatus::Active;
			ActiveTimerHeap.HeapPush(TimerToUnPause);
		}
		else
		{
			TimerToUnPause.Status = ETimerStatus::Pending;
			PendingTimerList.Add(TimerToUnPause);
		}

		// remove from paused list
		PausedTimerList.RemoveAtSwap(PausedTimerIdx);
	}
}

// ---------------------------------
// Public members
// ---------------------------------

void FTimerManager::Tick(float DeltaTime)
{
	// @todo, might need to handle long-running case
	// (e.g. every X seconds, renormalize to InternalTime = 0)

	InternalTime += DeltaTime;

	while (ActiveTimerHeap.Num() > 0)
	{
		FTimerData& Top = ActiveTimerHeap.HeapTop();
		if (InternalTime > Top.ExpireTime)
		{
			// Timer has expired! Fire the delegate, then handle potential looping.

			// Remove it from the heap and store it while we're executing
			ActiveTimerHeap.HeapPop(CurrentlyExecutingTimer); 

			// Determine how many times the timer may have elapsed (e.g. for large DeltaTime on a short looping timer)
			int32 const CallCount = CurrentlyExecutingTimer.bLoop ? 
				FMath::Trunc( (InternalTime - CurrentlyExecutingTimer.ExpireTime) / CurrentlyExecutingTimer.Rate ) + 1
				: 1;

			// Now call the function
			for (int32 CallIdx=0; CallIdx<CallCount; ++CallIdx)
			{
				CurrentlyExecutingTimer.TimerDelegate.Execute();

				// If timer was cleared in the delegate execution, don't execute further 
				if( !CurrentlyExecutingTimer.TimerDelegate.IsBound() )
				{
					break;
				}
			}

			if( CurrentlyExecutingTimer.bLoop && 
				CurrentlyExecutingTimer.TimerDelegate.IsBound() && 							// did not get cleared during execution
				(FindTimer(CurrentlyExecutingTimer.TimerDelegate) == NULL)		// did not get manually re-added during execution
			  )
			{
				// Put this timer back on the heap
				CurrentlyExecutingTimer.ExpireTime += CallCount * CurrentlyExecutingTimer.Rate;
				ActiveTimerHeap.HeapPush(CurrentlyExecutingTimer);
			}

			CurrentlyExecutingTimer.TimerDelegate.Unbind();
		}
		else
		{
			// no need to go further down the heap, we can be finished
			break;
		}
	}

	// Timer has been ticked.
	LastTickedFrame = GFrameCounter;

	// If we have any Pending Timers, add them to the Active Queue.
	if( PendingTimerList.Num() > 0 )
	{
		for(int32 Index=0; Index<PendingTimerList.Num(); Index++)
		{
			FTimerData& TimerToActivate = PendingTimerList[Index];
			// Convert from time remaining back to a valid ExpireTime
			TimerToActivate.ExpireTime += InternalTime;
			TimerToActivate.Status = ETimerStatus::Active;
			ActiveTimerHeap.HeapPush( TimerToActivate );
		}
		PendingTimerList.Empty();
	}
}

TStatId FTimerManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FTimerManager, STATGROUP_Tickables);
}
