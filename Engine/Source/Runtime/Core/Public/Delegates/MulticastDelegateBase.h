// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MulticastDelegateBase.h: Declares the FMulticastDelegateBase class.
=============================================================================*/

#pragma once


/**
 * Abstract base class for multicast delegates.
 */
template<typename ObjectPtrType = FWeakObjectPtr>
class FMulticastDelegateBase
{
public:

	/**
	 * Removes all functions from this delegate's invocation list
	 */
	void Clear( )
	{
		InvocationList.Empty();
		ExpirableObjectCount = 0;
	}

	/**
	 * Checks to see if any functions are bound to this multi-cast delegate.
	 *
	 * This method is protected to avoid assumptions about only one delegate existing.
	 *
	 * @return true if any functions are bound, false otherwise.
	 */
	inline bool IsBound( ) const
	{
		return (InvocationList.Num() > 0);
	}

	/** 
	 * Checks to see if any functions are bound to the given user object.
	 *
	 * @return	True if any functions are bound to InUserObject, false otherwise.
	 */
	inline bool IsBoundToObject( void const* InUserObject ) const
	{
		for (IDelegateInstancePtr DelegateInstance : InvocationList)
		{
			if (DelegateInstance->HasSameObject(InUserObject))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Removes all functions from this multi-cast delegate's invocation list that are bound to the specified UserObject.
	 * Note that the order of the delegates may not be preserved!
	 *
	 * @param InUserObject The object to remove all delegates for.
	 */
	void RemoveAll( const void* InUserObject )
	{
		RemoveByObject(InUserObject);
		CompactInvocationList();
	}

protected:

	/**
	 * Hidden default constructor.
	 */
	inline FMulticastDelegateBase( )
		: ExpirableObjectCount(0)
	{ }

protected:

	/**
	 * Adds the given delegate instance to the invocation list.
	 *
	 * @param DelegateInstance The delegate instance to add.
	 */
	void AddInternal( const IDelegateInstanceRef& DelegateInstance )
	{
		InvocationList.Add(DelegateInstance);

		// Keep track of whether we have objects bound that may expire out from underneath us.
		const EDelegateInstanceType::Type DelegateType = DelegateInstance->GetType();

		if ((DelegateType == EDelegateInstanceType::SharedPointerMethod) ||
			(DelegateType == EDelegateInstanceType::ThreadSafeSharedPointerMethod) ||
			(DelegateType == EDelegateInstanceType::UObjectMethod))
		{
			++ExpirableObjectCount;
		}
	}

	/**
	 * Cleans up any delegates in our invocation list that have expired (performance is O(N))
	 */
	void CompactInvocationList( ) const
	{
		// We only need to compact if any objects were added whose lifetime is known to us
		if (ExpirableObjectCount == 0)
		{
			return;
		}

		// remove expired delegates
		for (int32 InvocationListIndex = InvocationList.Num() - 1; InvocationListIndex >= 0; --InvocationListIndex)
		{
			IDelegateInstancePtr DelegateInstance = InvocationList[InvocationListIndex];

			if (DelegateInstance->IsCompactable())
			{
				InvocationList.RemoveAtSwap(InvocationListIndex);

				checkSlow(ExpirableObjectCount > 0);
				--ExpirableObjectCount;
			}
		}
	}

	/**
	 * Gets a read-only reference to the invocation list.
	 *
	 * @return The invocation list.
	 */
	const TArray<IDelegateInstancePtr>& GetInvocationList( ) const
	{
		return InvocationList;
	}

	/**
	 * Removes all functions from this multi-cast delegate's invocation list that are bound to the specified UserObject.
	 *
	 * Note that the order of the delegates may not be preserved!
	 *
	 * @param InUserObject The object to remove the delegates for.
	 */
	void RemoveByObject( const void* InUserObject )
	{
		for (int32 InvocationListIndex = InvocationList.Num() - 1; InvocationListIndex >= 0; --InvocationListIndex)
		{
			IDelegateInstancePtr DelegateInstance = InvocationList[InvocationListIndex];

			if (DelegateInstance->HasSameObject(InUserObject))
			{
				RemoveByIndex(InvocationListIndex);
			}
		}
	}

	/**
	 * Removes a delegate instance given its index in the invocation list.
	 *
	 * @param InvocationListIndex The index of the delegate instance to remove.
	 */
	void RemoveByIndex( int32 InvocationListIndex )
	{
		IDelegateInstancePtr DelegateInstance = InvocationList[InvocationListIndex];

		// keep track of whether we have objects bound that may expire out from underneath us
		const EDelegateInstanceType::Type DelegateType = DelegateInstance->GetType();

		if ((DelegateType == EDelegateInstanceType::SharedPointerMethod) || 
			(DelegateType == EDelegateInstanceType::ThreadSafeSharedPointerMethod) ||
			(DelegateType == EDelegateInstanceType::UFunction) ||
			(DelegateType == EDelegateInstanceType::UObjectMethod))
		{
			--ExpirableObjectCount;
		}

		InvocationList.RemoveAtSwap(InvocationListIndex);
	}

	/**
	 * Removes the given delegate instance from the invocation list.
	 *
	 * @param DelegateInstance The delegate instance to remove.
	 */
	void RemoveInternal( const IDelegateInstancePtr& DelegateInstance )
	{
		int32 DelegateIndex;
		
		if (InvocationList.Find(DelegateInstance, DelegateIndex))
		{
			RemoveByIndex(DelegateIndex);
		}
	}

private:

	// Mutable so that we can housekeep list even with const broadcasts
	mutable int32 ExpirableObjectCount;

	// Holds the collection of delegate instances to invoke.
	// Mutable so that we can do housekeeping during const broadcasts.
	mutable TArray<IDelegateInstancePtr> InvocationList;
};
