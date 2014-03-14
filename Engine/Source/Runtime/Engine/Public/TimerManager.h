// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TimerManager.h: Global gameplay timer facility
=============================================================================*/

#pragma once


// @hack to make wrapped dynamic delegate work in a non-UClass
inline void FTimerDynamicDelegate_DelegateWrapper(const FScriptDelegate& FTimerDynamicDelegate)
{
	FTimerDynamicDelegate.ProcessDelegate<UObject>(NULL);
}

DECLARE_DELEGATE(FTimerDelegate);
DECLARE_DYNAMIC_DELEGATE(FTimerDynamicDelegate);


/** Simple interface to wrap a timer delegate that can be either native or dynamic. */
struct FTimerUnifiedDelegate
{
	/** Holds the delegate to call. */
	FTimerDelegate FuncDelegate;
	/** Holds the dynamic delegate to call. */
	FTimerDynamicDelegate FuncDynDelegate;

	FTimerUnifiedDelegate() {};
	FTimerUnifiedDelegate(FTimerDelegate const& D) : FuncDelegate(D) {};
	FTimerUnifiedDelegate(FTimerDynamicDelegate const& D) : FuncDynDelegate(D) {};
	
	inline void Execute()
	{
		if (FuncDelegate.IsBound())
		{
#if STATS
			TStatId StatId = TStatId();
			UObject* Object = FuncDelegate.GetUObject();
			if (Object)
			{
				StatId = Object->GetStatID();
			}
			FScopeCycleCounter Context(StatId);
#endif
			FuncDelegate.Execute();
		}
		else if (FuncDynDelegate.IsBound())
		{
			FuncDynDelegate.ProcessDelegate<UObject>(NULL);
		}
	}

	inline bool IsBound() const
	{
		return ( FuncDelegate.IsBound() || FuncDynDelegate.IsBound() );
	}

	inline bool IsBoundToObject(void const* Object) const
	{
		if (FuncDelegate.IsBound())
		{
			return FuncDelegate.IsBoundToObject(Object);
		}
		else if (FuncDynDelegate.IsBound())
		{
			return FuncDynDelegate.IsBoundToObject(Object);
		}

		return false;
	}

	inline void Unbind()
	{
		FuncDelegate.Unbind();
		FuncDynDelegate.Unbind();
	}

	inline bool operator==(const FTimerUnifiedDelegate& Other) const
	{
		return ( FuncDelegate.IsBound() && (FuncDelegate == Other.FuncDelegate) ) || ( FuncDynDelegate.IsBound() && (FuncDynDelegate == Other.FuncDynDelegate) );
	}
};

UENUM()
namespace ETimerStatus
{
	enum Type
	{
		Pending,
		Active,
		Paused
	};
}

struct FTimerData
{
	/** If true, this timer will loop indefinitely.  Otherwise, it will be destroyed when it expires. */
	bool bLoop;

	/** Timer Status */
	ETimerStatus::Type Status;
	
	/** Time between set and fire, or repeat frequency if looping. */
	float Rate;

	/** 
	 * Time (on the FTimerManager's clock) that this timer should expire and fire its delegate. 
	 * Note when a timer is paused, we re-base ExpireTime to be relative to 0 instead of the running clock, 
	 * meaning ExpireTime contains the remaining time until fire.
	 */
	double ExpireTime;

	/** Holds the delegate to call. */
	FTimerUnifiedDelegate TimerDelegate;

	FTimerData()
		: bLoop(false), Status(ETimerStatus::Active)
		, Rate(0), ExpireTime(0)
	{}

	/** Operator less, used to sort the heap based on time until execution. **/
	bool operator<(const FTimerData& Other) const
	{
		return ExpireTime < Other.ExpireTime;
	}
};


/** 
 * Class to globally manage timers.
 */
class ENGINE_API FTimerManager : public FNoncopyable
{
public:

	// ----------------------------------
	// FTickableGameObject interface

	void Tick(float DeltaTime);
	TStatId GetStatId() const;


	// ----------------------------------
	// Timer API

	FTimerManager()
		: InternalTime(0.0)
	{}


/**
	 * Sets a timer to call the given native function at a set interval.  If a timer is already set
	 * for this delegate, it will update the current timer to the new parameters and reset its 
	 * elapsed time to 0.
	 *
	 * @param inObj			Object to call the timer function on.
	 * @param inTimerMethod Method to call when timer fires.
	 * @param inRate		The amount of time between set and firing.  If <= 0.f, clears existing timers.
	 * @param inbLoop		true to keep firing at Rate intervals, false to fire only once.
	 */
	template< class UserClass >	
	FORCEINLINE void SetTimer( UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod, float inRate, bool inbLoop = false)
	{
		SetTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod), inRate, inbLoop );
	}
	template< class UserClass >	
	FORCEINLINE void SetTimer( UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod, float inRate, bool inbLoop = false)
	{
		SetTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod), inRate, inbLoop );
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE void SetTimer(FTimerDelegate const& InDelegate, float InRate, bool InbLoop)
	{
		InternalSetTimer( FTimerUnifiedDelegate(InDelegate), InRate, InbLoop );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE void SetTimer(FTimerDynamicDelegate const& InDynDelegate, float InRate, bool InbLoop)
	{
		InternalSetTimer( FTimerUnifiedDelegate(InDynDelegate), InRate, InbLoop );
	}


	/**
	 * Clears a previously set timer, identical to calling SetTimer() with a <= 0.f rate.
	 *
	 * @param inObj			Object to call the timer function on.
	 * @param inTimerMethod Method to call when timer fires
	 */
	template< class UserClass >	
	FORCEINLINE void ClearTimer( UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		ClearTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE void ClearTimer( UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		ClearTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE void ClearTimer(FTimerDelegate const& InDelegate)
	{
		InternalClearTimer( FTimerUnifiedDelegate(InDelegate) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE void ClearTimer(FTimerDynamicDelegate const& InDynDelegate)
	{
		InternalClearTimer( FTimerUnifiedDelegate(InDynDelegate) );
	}

	/** Clears all timers that are bound to functions on the given object. */
	FORCEINLINE void ClearAllTimersForObject(void const* Object)
	{
		if (Object)
		{
			InternalClearAllTimers( Object );
		}
	}


	/**
	 * Pauses a previously set timer.
	 *
	 * @param inObj				Object to call the timer function on.
	 * @param inTimerMethod		Method to call when timer fires.
	 */
	template< class UserClass >	
	FORCEINLINE void PauseTimer(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		PauseTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE void PauseTimer(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		PauseTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE void PauseTimer(FTimerDelegate const& InDelegate)
	{
		InternalPauseTimer( FTimerUnifiedDelegate(InDelegate) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE void PauseTimer(FTimerDynamicDelegate const& InDynDelegate)
	{
		InternalPauseTimer( FTimerUnifiedDelegate(InDynDelegate) );
	}

	/**
	 * Unpauses a previously set timer
	 *
	 * @param inObj				Object to call the timer function on.
	 * @param inTimerMethod		Method to call when timer fires.
	 */
	template< class UserClass >	
	FORCEINLINE void UnPauseTimer(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		UnPauseTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE void UnPauseTimer(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		UnPauseTimer( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE void UnPauseTimer(FTimerDelegate const& InDelegate)
	{
		InternalUnPauseTimer( FTimerUnifiedDelegate(InDelegate) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE void UnPauseTimer(FTimerDynamicDelegate const& InDynDelegate)
	{
		InternalUnPauseTimer( FTimerUnifiedDelegate(InDynDelegate) );
	}

	/**
	 * Gets the current rate (time between activations) for the specified timer.
	 *
	 * @param inObj			Object to call the timer function on.
	 * @param inTimerMethod Method to call when timer fires.
	 * @return				The current rate or -1.f if timer does not exist.
	 */
	template< class UserClass >	
	FORCEINLINE float GetTimerRate(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		return GetTimerRate( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE float GetTimerRate(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		return GetTimerRate( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE float GetTimerRate(FTimerDelegate const& InDelegate)
	{
		return InternalGetTimerRate( FTimerUnifiedDelegate(InDelegate) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE float GetTimerRate(FTimerDynamicDelegate const& InDynDelegate)
	{
		return InternalGetTimerRate( FTimerUnifiedDelegate(InDynDelegate) );
	}
	/**
	 * Returns true if the specified timer exists and is not paused
	 *
	 * @param inObj			Object to call the timer function on.
	 * @param inTimerMethod Method to call when timer fires.
	 * @return				true if the timer is active, false otherwise.
	 */
	template< class UserClass >	
	FORCEINLINE bool IsTimerActive(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		return IsTimerActive( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE bool IsTimerActive(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		return IsTimerActive( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE bool IsTimerActive(FTimerDelegate const& InDelegate)
	{
		FTimerData const* const TimerData = FindTimer( FTimerUnifiedDelegate(InDelegate) );
		return ( (TimerData != NULL) && (TimerData->Status != ETimerStatus::Paused) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE bool IsTimerActive(FTimerDynamicDelegate const& InDynDelegate)
	{
		FTimerData const* const TimerData = FindTimer( FTimerUnifiedDelegate(InDynDelegate) );
		return ( (TimerData != NULL) && (TimerData->Status != ETimerStatus::Paused) );
	}

	/**
	 * Gets the current elapsed time for the specified timer.
	 *
	 * @param inObj			Object to call the timer function on
	 * @param inTimerMethod Method to call when timer fires
	 * @return				The current time elapsed or -1.f if timer does not exist.
	 */
	template< class UserClass >	
	FORCEINLINE float GetTimerElapsed(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		return GetTimerElapsed( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE float GetTimerElapsed(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		return GetTimerElapsed( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	/** Version that takes any generic delegate. */
	FORCEINLINE float GetTimerElapsed(FTimerDelegate const& InDelegate)
	{
		return InternalGetTimerElapsed( FTimerUnifiedDelegate(InDelegate) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE float GetTimerElapsed(FTimerDynamicDelegate const& InDynDelegate)
	{
		return InternalGetTimerElapsed( FTimerUnifiedDelegate(InDynDelegate) );
	}

	/**
	 * Gets the time remaining before the specified timer is called
	 *
	 * @param inObj			Object to call the timer function on.
	 * @param inTimerMethod Method to call when timer fires.
	 * @return				The current time remaining, or -1.f if timer does not exist
	 */
	template< class UserClass >	
	FORCEINLINE float GetTimerRemaining(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr inTimerMethod)
	{
		return GetTimerRemaining( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	template< class UserClass >	
	FORCEINLINE float GetTimerRemaining(UserClass* inObj, typename FTimerDelegate::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr inTimerMethod)
	{
		return GetTimerRemaining( FTimerDelegate::CreateUObject(inObj, inTimerMethod) );
	}
	/** Version that takes any generic delegate. */
	FORCEINLINE float GetTimerRemaining(FTimerDelegate const& InDelegate)
	{
		return InternalGetTimerRemaining( FTimerUnifiedDelegate(InDelegate) );
	}
	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE float GetTimerRemaining(FTimerDynamicDelegate const& InDynDelegate)
	{
		return InternalGetTimerRemaining( FTimerUnifiedDelegate(InDynDelegate) );
	}

	/** Timer manager has been ticked this frame? */
	bool FORCEINLINE HasBeenTickedThisFrame() const
	{
		return (LastTickedFrame == GFrameCounter);
	}

private:

	void InternalSetTimer( FTimerUnifiedDelegate const& InDelegate, float inRate, bool inbLoop );
	void InternalClearTimer( FTimerUnifiedDelegate const& InDelegate );
	void InternalClearAllTimers(void const* Object);

	/** Will find a timer either in the active or paused list. */
	FTimerData const* FindTimer( FTimerUnifiedDelegate const& InDelegate, int32* OutTimerIndex=NULL ) const;

	void InternalPauseTimer( FTimerUnifiedDelegate const& InDelegate );
	void InternalUnPauseTimer( FTimerUnifiedDelegate const& InDelegate );

	float InternalGetTimerRate( FTimerUnifiedDelegate const& InDelegate ) const;
	float InternalGetTimerElapsed( FTimerUnifiedDelegate const& InDelegate ) const;
	float InternalGetTimerRemaining( FTimerUnifiedDelegate const& InDelegate ) const;

	/** Will find the given timer in the given TArray and return its index. */ 
	int32 FindTimerInList(const TArray<FTimerData> &SearchArray, FTimerUnifiedDelegate const& InDelegate ) const;

	/** Heap of actively running timers. */
	TArray<FTimerData> ActiveTimerHeap;
	/** Unordered list of paused timers. */
	TArray<FTimerData> PausedTimerList;
	/** List of timers added this frame, to be added after timer has been ticked */
	TArray<FTimerData> PendingTimerList;

	/** An internally consistent clock, independent of World.  Advances during ticking. */
	double InternalTime;

	/** Timer delegate currently being executed.  Used to handle "timer delegates that manipulating timers" cases. */
	FTimerData CurrentlyExecutingTimer;

	/** Set this to GFrameCounter when Timer is ticked. To figure out if Timer has been already ticked or not this frame. */
	uint64 LastTickedFrame;
};

