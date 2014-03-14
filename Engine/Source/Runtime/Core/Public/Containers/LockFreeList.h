// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LockFreeList.h: A lock free single linked list
=============================================================================*/

#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogLockFreeList, Log, All);

// what level of checking to perform...normally checkLockFreePointerList but could be ensure or check
#define checkLockFreePointerList checkSlow

#define CHECK_NON_CONCURRENT_ASSUMPTIONS (0)

/** Fake Thread safe counter, used to avoid cluttering code with ifdefs when counters are only used for debugging */
class FNoopCounter
{
public:
	FNoopCounter()
	{
	}
	FNoopCounter( const FNoopCounter& Other )
	{
	}
	FNoopCounter( int32 Value )
	{
	}
	int32 Increment()
	{
		return 0;
	}
	int32 Add( int32 Amount )
	{
		return 0;
	}
	int32 Decrement()
	{
		return 0;
	}
	int32 Subtract( int32 Amount )
	{
		return 0;
	}
	int32 Set( int32 Value )
	{
		return 0;
	}
	int32 Reset()
	{
		return 0;
	}
	int32 GetValue() const
	{
		return 0;
	}
};

#define MONITOR_LINK_ALLOCATION (0)

/** 
* Base class for a lock free list of pointers 
* This class has several different features. Not all methods are supported in all situations.
* The intention is that derived classes use private inheritance to build a data structure only supports a small meaningful set of operations.
*
* A key feature is that lists can be "closed". A closed list cannot be pushed to or popped from. Closure is atomic with push and pop.
* All operations are threadsafe, except that the constructor is not reentrant.
**/
class FLockFreeVoidPointerListBase
{
public:
	/** Constructor - sets the list to empty and initializes the allocator if it hasn't already been intialized **/
	FLockFreeVoidPointerListBase()
		: Head(NULL)
	{
		FLinkAllocator::Get(); // construct allocator singleton if it isn't already
	}
	/** Destructor - checks that the list is either empty or closed. Lists should not be destroyed in any other state. **/
	~FLockFreeVoidPointerListBase()
	{
		// leak these at shutdown
		//checkLockFreePointerList(Head == NULL || Head == FLinkAllocator::Get().ClosedLink()); // we do not allow destruction except in the empty or closed state
	}

	/**	
	 *	Push an item onto the head of the list
	 *	@param NewItem, the new item to push on the list, cannot be NULL
	 *	CAUTION: This method should not be used unless the list is known to not be closed.
	**/
	void Push(void *NewItem)
	{
		FLink* NewLink = FLinkAllocator::Get().AllocateLink(NewItem);
		NewLink->Link(&Head);
		checkLockFreePointerList(NewLink->Next != FLinkAllocator::Get().ClosedLink()); // it is not permissible to call push on a Closed queue
	}

	/**	
	 *	Push an item onto the head of the list, unless the list is closed
	 *	@param NewItem, the new item to push on the list, cannot be NULL
	 *	@return true if the item was pushed on the list, false if the list was closed.
	**/
	bool PushIfNotClosed(void *NewItem)
	{
		FLink* NewLink = FLinkAllocator::Get().AllocateLink(NewItem);
		bool bSuccess = NewLink->LinkIfHeadNotEqual(&Head, FLinkAllocator::Get().ClosedLink());
		if (!bSuccess)
		{
			NewLink->Dispose();
		}
		return bSuccess;
	}

	/**	
	 *	Push an item onto the head of the list, opening it first if necessary
	 *	@param NewItem, the new item to push on the list, cannot be NULL
	 *	@return true if the list needed to be opened first, false if the list was not closed before our push
	**/
	bool ReopenIfClosedAndPush(void *NewItem)
	{
		FLink* NewLink = FLinkAllocator::Get().AllocateLink(NewItem);
		while (1)
		{
			if (NewLink->LinkIfHeadNotEqual(&Head, FLinkAllocator::Get().ClosedLink()))
			{
				return false;
			}
			if (NewLink->ReplaceHeadIfHeadEqual(&Head, FLinkAllocator::Get().ClosedLink()))
			{
				return true;
			}
		}
	}

	/**	
	 *	Pop an item from the list or return NULL if the list is empty
	 *	@return The popped item, if any
	 *	CAUTION: This method should not be used unless the list is known to not be closed.
	**/
	void* Pop()
	{
		FLink* Link = FLink::Unlink(&Head);
		if (!Link)
		{
			return NULL;
		}
		checkLockFreePointerList(Link != FLinkAllocator::Get().ClosedLink()); // it is not permissible to call pop on a Closed queue
		void* Return = Link->Item;
		Link->Dispose();
		return Return;
	}

	/**	
	 *	Pop an item from the list or return NULL if the list is empty or closed
	 *	@return The new item, if any
	**/
	void* PopIfNotClosed()
	{
		FLink* Link = FLink::Unlink(&Head, FLinkAllocator::Get().ClosedLink());
		if (!Link)
		{
			return NULL;
		}
		checkLockFreePointerList(Link != FLinkAllocator::Get().ClosedLink()); // internal error, this should not be possible
		void* Return = Link->Item;
		Link->Dispose();
		return Return;
	}

	/**	
	 *	Close the list if it is empty
	 *	@return true if this call actively closed the list
	**/
	bool CloseIfEmpty()
	{
		if (FLinkAllocator::Get().ClosedLink()->ReplaceHeadIfHeadEqual(&Head,NULL))
		{
			return true;
		}
		return false;
	}

	/**	
	 *	Pop all items from the list 
	 *	@param Output The array to hold the returned items. Must be empty.
	 *	CAUTION: This method should not be used unless the list is known to not be closed.
	**/
	template <class ARRAYTYPE, typename ElementType>
	void PopAll(ARRAYTYPE& Output)
	{
		checkLockFreePointerList(!Output.Num());
		FLink* Link = FLink::ReplaceList(&Head);
		while (Link)
		{
			checkLockFreePointerList(Link != FLinkAllocator::Get().ClosedLink()); // it is not permissible to call PopAll on a Closed queue
			checkLockFreePointerList(Link->Item); // we don't allow null items
			Output.Add((ElementType)(Link->Item));
			FLink* NextLink = Link->Next;
			Link->Dispose();
			Link = NextLink;
		}
	}

	/**	
	 *	Pop all items from the list and atomically close it.
	 *	@param Output The array to hold the returned items. Must be empty.
	**/
	template <class ARRAYTYPE, typename ElementType>
	void PopAllAndClose(ARRAYTYPE& Output)
	{
		checkLockFreePointerList(!Output.Num());
		FLink* Link = FLink::ReplaceList(&Head, FLinkAllocator::Get().ClosedLink());
		while (Link)
		{
			checkLockFreePointerList(Link != FLinkAllocator::Get().ClosedLink()); // we should not pop off the closed link
			checkLockFreePointerList(Link->Item); // we don't allow null items
			Output.Add((ElementType)(Link->Item));
			FLink* NextLink = Link->Next;
			Link->Dispose();
			Link = NextLink;
		}
	}

	/**	
	 *	Check if the list is closed
	 *	@return true if the list is closed.
	 *	CAUTION: This methods safety depends on external assumptions. For example, if another thread could open or close the list at any time, the return value is no better than a best guess.
	 *	As typically used, the list is only closed once it is lifetime, so a return of true here means it is closed forever.
	**/
	bool IsClosed() const  // caution, if this returns false, that does not mean the list is open!
	{
		FPlatformMisc::MemoryBarrier();
		return Head == FLinkAllocator::Get().ClosedLink();
	}

	/**	
	 *	Check if the list is empty
	 *	@return true if the list is empty.
	 *	CAUTION: This methods safety depends on external assumptions. For example, if another thread could add to the list at any time, the return value is no better than a best guess.
	 *	As typically used, the list is not being access concurrently when this is called.
	**/
	bool IsEmpty() const  
	{
		FPlatformMisc::MemoryBarrier();
		return Head == NULL;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/**
	 * Checks if item pointers look 'ok'. Used for debugging.
	 */
	bool CheckPointers() const
	{
		const int32 AlignmentCheck = MIN_ALIGNMENT - 1;
		FPlatformMisc::MemoryBarrier();
		for (FLink* Link = Head; Link; Link = Link->Next)
		{
			void* ItemPtr = Link->Item;
			if (!ItemPtr || (UPTRINT)ItemPtr < 0x100)
			{
				return false;
			}	
			if ((UPTRINT)ItemPtr & AlignmentCheck)
			{
				return false;
			}
		}
		return true;
	}
#endif

	/**	Frees all of the links from the recycling queues.**/
	static void FlushMemoryForShutdown();

private:
	/**	Internal single-linked list link structure. **/
	struct FLink
	{
		/** Next item in the list or NULL if we are at the end of the list **/
		FLink*				Next;
		/** 
		 * Pointer to the item this list holds. The is useful to the external users of the system. Must not be NULL for ordinary links. 
		 * Also part of the solution to the ABA problem. NULL Items are always on the recycling list and NON-null items are not.
		**/
		void*				Item;
		/** Part of the solution to the ABA problem. Links cannot be recycled until no threads hold pointers to them. When the lock count drops to zero, the link is recycled. **/
		FThreadSafeCounter	LockCount;
		/** Part of the solution to the ABA problem. Links that are free still need locking, but we need to make sure that links are only added to the free list when they are transitioning to the free list. **/
		FThreadSafeCounter	MarkedForDeath;

		/** Construcor, everything is intialized to zero. **/
		FLink()
			: Next(NULL)
			, Item(NULL)
		{
		}

		/**	
		 *	Link this node into the head of the given list.
		 *	@param HeadPointer; the head of the list
		 *	CAUTION: Not checked here, but linking into a closed list with the routine would accidently and incorrectly open the list
		**/
		void Link(FLink** HeadPointer)
		{
			CheckNotMarkedForDeath();
			while (1)
			{
				FLink*  LocalHeadPointer = LockLink(HeadPointer);
				CheckNotMarkedForDeath();
				SetNextPointer(LocalHeadPointer);
				FLink* ValueWas = (FLink*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)HeadPointer,this,LocalHeadPointer);
				if (ValueWas == LocalHeadPointer)
				{
					if (LocalHeadPointer)
					{
						LocalHeadPointer->Unlock();
					}
					break;
				}
				SetNextPointer(NULL);
				if (LocalHeadPointer)
				{
					LocalHeadPointer->Unlock();
				}
			}
		}

		/**	
		 *	Link this node into the head of the given list, unless the head is a special, given node
		 *	@param HeadPointer; the head of the list
		 *	@param SpecialClosedLink; special link that is never recycled that indicates a closed list.
		 *	@return true if this node was linked to the head.
		**/
		bool LinkIfHeadNotEqual(FLink** HeadPointer, FLink* SpecialClosedLink)
		{
			checkLockFreePointerList(!Next); // this should not be set yet
			CheckNotMarkedForDeath();
			while (1)
			{
				if (*HeadPointer == SpecialClosedLink)
				{
					CheckNotMarkedForDeath();
					return false;
				}
				FLink*  LocalHeadPointer = LockLink(HeadPointer, SpecialClosedLink);
				CheckNotMarkedForDeath();
				// here if the list turns out to be locked now, the compare and swap will fail anyway
				SetNextPointer(LocalHeadPointer);
				FLink* ValueWas = (FLink*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)HeadPointer,this,LocalHeadPointer);
				if (ValueWas == LocalHeadPointer)
				{
					if (LocalHeadPointer)
					{
						LocalHeadPointer->Unlock();
					}
					break;
				}
				SetNextPointer(NULL);
				if (LocalHeadPointer)
				{
					LocalHeadPointer->Unlock();
				}
			}
			return true;
		}
		/**	
		 *	If the head is a particular value, then replace it with this node. This is the primitive that is used to close and open lists
		 *	@param HeadPointer; the head of the list
		 *	@param SpecialTestLink; special link that is never recycled that indicates a closed list. Or NULL in the case that we are using this to open the list
		 *	@return true if this node was linked to the head.
		 *	CAUTION: The test link must NULL or a special link that is never recycled. The ABA problem is assumed to not occur here.
		**/
		bool ReplaceHeadIfHeadEqual(FLink** HeadPointer, FLink* SpecialTestLink)
		{
			CheckNotMarkedForDeath();
			while (1)
			{
				if (*HeadPointer != SpecialTestLink)
				{
					return false;
				}
				CheckNotMarkedForDeath();
				if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)HeadPointer,this,SpecialTestLink) == SpecialTestLink)
				{
					break;
				}
			}
			return true;
		}
		/**	
		 *	Pop an item off the list, unless the list is empty or the head is a special, given node
		 *	@param HeadPointer; the head of the list
		 *	@param SpecialClosedLink; special link that is never recycled that indicates a closed list. Can be NULL for lists that are known to not be closed
		 *	@return The link that was popped off, or NULL if the list was empty or closed.
		 *	CAUTION: Not checked here, but if the list is closed and SpecialClosedLink is NULL, you will pop off the closed link, and that usually isn't what you want
		**/
		static FLink* Unlink(FLink** HeadPointer, FLink* SpecialClosedLink = NULL)
		{
			while (1)
			{
				FLink*  LocalHeadPointer = LockLink(HeadPointer, SpecialClosedLink);
				if (!LocalHeadPointer)
				{
					break;
				}
				FLink* NextLink = LocalHeadPointer->Next;
				FLink* ValueWas = (FLink*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)HeadPointer,NextLink,LocalHeadPointer);
				if (ValueWas == LocalHeadPointer)
				{
					checkLockFreePointerList(NextLink == LocalHeadPointer->Next);
					LocalHeadPointer->SetNextPointer(NULL); // this is redundant, but we are not given the caller ownership of the other items
					LocalHeadPointer->Unlock(true); // this cannot possibly result in a destruction of the link because we have taken sole possession of it
					LocalHeadPointer->CheckNotMarkedForDeath(); // we are the exclusive owners at this point
					return LocalHeadPointer;
				}
				LocalHeadPointer->Unlock();
			}
			return NULL;
		}
		/**	
		 *	Replace the list with another list.h Use to either acquire all of the items or acquire all of the items and close the list.
		 *	@param HeadPointer; the head of the list
		 *	@param NewHeadLink; Head of the new list to replace this list with
		 *	@return The original list before we replaced it.
		 *	CAUTION: Not checked here, but if the list is closed this is probably not what you want. 
		**/
		static FLink* ReplaceList(FLink** HeadPointer, FLink* NewHeadLink = NULL)
		{
			while (1)
			{
				FLink*  LocalHeadPointer = LockLink(HeadPointer);
				if (!LocalHeadPointer && !NewHeadLink)
				{
					// we are asking to replace NULL with NULL, this does not require an atomic (or any) operation
					break;
				}
				checkLockFreePointerList(LocalHeadPointer != NewHeadLink); // replacing NULL with NULL is ok, but otherwise we cannot be replacing something with itself or we lose determination of who did what.
				FLink* ValueWas = (FLink*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)HeadPointer,NewHeadLink,LocalHeadPointer);
				if (ValueWas == LocalHeadPointer)
				{
					if (LocalHeadPointer)
					{
						LocalHeadPointer->Unlock(true); // this cannot possibly result in a destruction of the link because we have taken sole possession of it
						LocalHeadPointer->CheckNotMarkedForDeath(); // we are the exclusive owners at this point
					}
					return LocalHeadPointer;
				}
				if (LocalHeadPointer)
				{
					LocalHeadPointer->Unlock();
				}
			}
			return NULL;  
		}

		/**	
		 *	Internal function to safely grab the head pointer and increase the ref count of a link to avoid the ABA problem
		 *	@param HeadPointer; the head of the list
		 *	@param SpecialClosedLink; special link that is never recycled that indicates a closed list. Can be NULL for lists that are known to not be closed, or if you want to operate on the closed link.
		 *	@return Pointer to the head link, with the ref count increased. Or NULL if the head is NULL or SpecialClosedLink
		 *	CAUTION: Not checked here, but if the list is closed this is probably not what you want. 
		**/
		static FLink* LockLink(FLink** HeadPointer, FLink* SpecialClosedLink = NULL)
		{
			while (1)
			{
				FLink*  LocalHeadPointer = *HeadPointer;
				if (!LocalHeadPointer || LocalHeadPointer == SpecialClosedLink)
				{
					return NULL;
				}
				LocalHeadPointer->LockCount.Increment();
				FPlatformMisc::MemoryBarrier();
				if (*HeadPointer == LocalHeadPointer)
				{
					return LocalHeadPointer;
				}
				LocalHeadPointer->Unlock();
			}
		}

		void SetNextPointer(FLink* NewNext)
		{
			FLink* LocalNextPointer = Next;
			checkLockFreePointerList(LocalNextPointer != NewNext || !NewNext); // should not be the same, except both can be NULL
			checkLockFreePointerList(!LocalNextPointer || !NewNext); // one (or both) of them should be NULL
#if CHECK_NON_CONCURRENT_ASSUMPTIONS
			FLink* ValueWas = (FLink*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Next, NewNext, LocalNextPointer); // not known to need to be interlocked
			checkLockFreePointerList(ValueWas == LocalNextPointer); // this should always work because there should never be contention on setting next
#else
			Next = NewNext;
#endif
			FPlatformMisc::MemoryBarrier();
			if (NewNext)
			{
				CheckNotMarkedForDeath(); // it shouldn't be possible for the node to die while we are setting a non-null next pointer
			}
		}

		// verify that we are not marked for destruction
		void CheckNotMarkedForDeath()
		{
			checkLockFreePointerList(MarkedForDeath.GetValue() == 0); // we don't allow null items
		}

		/**	
		 *	Mark the node for disposal and unlock it
		**/
		void Dispose()
		{
			CheckNotMarkedForDeath();

#if CHECK_NON_CONCURRENT_ASSUMPTIONS
			{
				void *CurrentItem = Item;
				checkLockFreePointerList(CurrentItem); // we don't allow null items
				void* ValueWas = FPlatformAtomics::InterlockedCompareExchangePointer(&Item,NULL,CurrentItem); // do not need an interlocked operation here
				checkLockFreePointerList(ValueWas == CurrentItem); // there should be no concurrency here
			}

			{
				FLink* LocalNextPointer = Next;
				FLink* ValueWas = (FLink*)FPlatformAtomics::InterlockedCompareExchangePointer((void**)&Next, NULL, LocalNextPointer); // not known to need to be interlocked
				checkLockFreePointerList(ValueWas == LocalNextPointer); // this should always work because there should never be contention on setting next
			}
#else
			Item = NULL;
			Next = NULL;
#endif
			FPlatformMisc::MemoryBarrier(); // memory barrier in here to make sure item and next are cleared before the link is recycled

			int32 Death = MarkedForDeath.Increment();
			checkLockFreePointerList(Death == 1); // there should be no concurrency here
			Unlock();
		}

		/**	
		 *	Internal function to unlock a link and recycle it if the ref count drops to zero and it hasn't already been recycled.
		 *	This solves the ABA problem by delaying recycling until all pointers to this link are gone.
		 *	@param bShouldNeverCauseAFree - if true, then this should never result in a free if it does, there is a run time error
		**/
		void Unlock(bool bShouldNeverCauseAFree = false);
	};

	/**	
	 *	Class to allocate and recycle links.
	**/
	class FLinkAllocator
	{
	public:

		/**	
		 *	Return a new link.
		 *	@param NewItem "item" pointer of the new node.
		 *	@return New node, ready for use
		**/
		FLink* AllocateLink(void *NewItem)
		{
			checkLockFreePointerList(NewItem); // we don't allow null items
#if MONITOR_LINK_ALLOCATION
			if (NumUsedLinks.Increment() % 8192 == 1)
			{
				UE_LOG(LogLockFreeList, Log, TEXT("Number of links %d"),NumUsedLinks.GetValue());
			}
#endif
			NumUsedLinks.Increment();

			FLink*  NewLink = NULL;
			NewLink = FLink::Unlink(&FreeLinks);
			if (NewLink)
			{
				NumFreeLinks.Decrement();
			}
			else
			{
#if MONITOR_LINK_ALLOCATION
				if (NumAllocatedLinks.Increment() % 10 == 1)
				{
					UE_LOG(LogLockFreeList, Log, TEXT("Number of allocated links %d"),NumAllocatedLinks.GetValue());
				}
#endif
				NumAllocatedLinks.Increment();

				NewLink = new FLink();
				NewLink->LockCount.Increment();
			}
			checkLockFreePointerList(!NewLink->Item);
			checkLockFreePointerList(!NewLink->Next);
			checkLockFreePointerList(NewLink->LockCount.GetValue() >= 1);
#if CHECK_NON_CONCURRENT_ASSUMPTIONS
			void* ValueWas = FPlatformAtomics::InterlockedCompareExchangePointer(&NewLink->Item, NewItem, NULL); // in theory this doesn't need to be interlocked
			checkLockFreePointerList(ValueWas == NULL);
#else
			NewLink->Item = NewItem;
#endif
			FPlatformMisc::MemoryBarrier();
			return NewLink;
		}
		/**	
		 *	Make a link available for recycling. 
		 *	@param Link link to recycle
		 *	CAUTION: Do not call this directly, it should only be called when the reference count drop to zero.
		**/
		void FreeLink(FLink* Link)
		{
			checkLockFreePointerList(Link != ClosedLink());  // very bad to recycle the special link
			NumUsedLinks.Decrement();
			Link->LockCount.Increment();
			FPlatformMisc::MemoryBarrier();
			Link->Link(&FreeLinks);
			NumFreeLinks.Increment();
		}
		/**	
		 *	Return a pointer to the special closed link
		 *	@return pointer to special closed link
		**/
		FLink* ClosedLink()
		{
			checkLockFreePointerList(NULL != SpecialClosedLink);
			return SpecialClosedLink;
		}
		/**	
		 *	Singleton access
		 *	@return the singleton for the link allocator
		**/
		static CORE_API FLinkAllocator& Get();

	private:
		/**	
		 *	Constructor zeros the free link list and creates the special closed link
		 *	@return the singleton for the link allocator
		**/
		FLinkAllocator();

		/**	
		 *	Destructor, should only be called when there are no outstanding links. 
		 *	Frees the elements of the free list and frees the special link
		 *	@return the singleton for the link allocator
		**/
		~FLinkAllocator();

		/** Head to a list of free links that can be used for new allocations. **/
		FLink*				FreeLinks;

		/** Pointer to the special closed link. It should never be recycled. **/
		FLink*				SpecialClosedLink;
#if MONITOR_LINK_ALLOCATION
		/** Total number of links outstanding and not in the free list **/
		FThreadSafeCounter	NumUsedLinks; 
		/** Total number of links in the free list **/
		FThreadSafeCounter	NumFreeLinks;
		/** Total number of links allocated **/
		FThreadSafeCounter	NumAllocatedLinks; 
#else
		/** If we aren't monitoring allocation, these do nothing **/
		FNoopCounter		NumUsedLinks; 
		FNoopCounter		NumFreeLinks;
		FNoopCounter		NumAllocatedLinks; 
#endif
	};

	/** Head of the list **/
	MS_ALIGN(8) FLink*	Head;

private:
	FLockFreeVoidPointerListBase(const FLockFreeVoidPointerListBase&);
	FLockFreeVoidPointerListBase& operator=(const FLockFreeVoidPointerListBase&);
};

/**	
	*	Internal function to unlock a link and recycle it if the ref count drops to zero and it hasn't already been recycled.
	*	This solves the ABA problem by delaying recycling until all pointers to this link are gone.
	*	@param bShouldNeverCauseAFree - if true, then this should never result in a free if it does, there is a run time error
**/
inline void FLockFreeVoidPointerListBase::FLink::Unlock(bool bShouldNeverCauseAFree)
{
	FPlatformMisc::MemoryBarrier();
	checkLockFreePointerList(LockCount.GetValue() > 0);
	if (LockCount.Decrement() == 0)
	{
		checkLockFreePointerList(MarkedForDeath.GetValue() < 2);
		if (MarkedForDeath.Reset())
		{
			checkLockFreePointerList(!bShouldNeverCauseAFree); 
			FLockFreeVoidPointerListBase::FLinkAllocator::Get().FreeLink(this);
		}
	}
}


/** 
 *	Wrapper around FLockFreeVoidPointerListBase
 *	Simple lock free list (stack) that does not permit closure.
 *	T is the type of the pointer the list will contain.
**/
template <class T>
class TLockFreePointerList : private FLockFreeVoidPointerListBase 
{
public:
	/**	
	 *	Push an item onto the head of the list
	 *	@param NewItem, the new item to push on the list, cannot be NULL
	**/
	void Push(T *NewItem)
	{
		FLockFreeVoidPointerListBase::Push(NewItem);
	}

	/**	
	 *	Pop an item from the list or return NULL if the list is empty
	 *	@return The popped item, if any
	**/
	T* Pop()
	{
		return (T*)FLockFreeVoidPointerListBase::Pop();
	}

	/**	
	 *	Pop all items from the list 
	 *	@param Output The array to hold the returned items. Must be empty.
	**/
	void PopAll(TArray<T *>& Output)
	{
		FLockFreeVoidPointerListBase::PopAll<TArray<T *>, T * >(Output);
	}	

	/**	
	 *	Check if the list is empty
	 *	@return true if the list is empty.
	 *	CAUTION: This methods safety depends on external assumptions. For example, if another thread could add to the list at any time, the return value is no better than a best guess.
	 *	As typically used, the list is not being access concurrently when this is called.
	**/
	bool IsEmpty() const  
	{
		return FLockFreeVoidPointerListBase::IsEmpty();
	}

};

template <class T>
class TClosableLockFreePointerList : private FLockFreeVoidPointerListBase 
{
public:
	/**	
	 *	Push an item onto the head of the list, unless the list is closed
	 *	@param NewItem, the new item to push on the list, cannot be NULL
	 *	@return true if the item was pushed on the list, false if the list was closed.
	**/
	bool PushIfNotClosed(T *NewItem)
	{
		return FLockFreeVoidPointerListBase::PushIfNotClosed(NewItem);
	}

	/**	
	 *	Pop all items from the list and atomically close it.
	 *	@param Output The array to hold the returned items. Must be empty.
	**/
	void PopAllAndClose(TArray<T *>& Output)
	{
		FLockFreeVoidPointerListBase::PopAllAndClose<TArray<T *>, T * >(Output);
	}	

	/**	
	 *	Check if the list is closed
	 *	@return true if the list is closed.
	**/
	bool IsClosed() const
	{
		return FLockFreeVoidPointerListBase::IsClosed();
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/**
	 * Checks if the pointers look 'ok'. Used for debugging.
	 */
	bool CheckPointers() const
	{
		return FLockFreeVoidPointerListBase::CheckPointers();
	}
#endif
};

template <class T>
class TReopenableLockFreePointerList : private FLockFreeVoidPointerListBase 
{
public:
	/**	
	 *	Push an item onto the head of the list, opening it first if necessary
	 *	@param NewItem, the new item to push on the list, cannot be NULL
	 *	@return true if the list needed to be opened first, false if the list was not closed before our push
	**/
	bool ReopenIfClosedAndPush(T *NewItem)
	{
		bool bWasReopenedByMe = FLockFreeVoidPointerListBase::ReopenIfClosedAndPush(NewItem);
		return bWasReopenedByMe;
	}

	/**	
	 *	Pop an item from the list or return NULL if the list is empty or closed
	 *	@return The new item, if any
	**/
	T* PopIfNotClosed()
	{
		return (T*)FLockFreeVoidPointerListBase::PopIfNotClosed();
	}

	/**	
	 *	Pop all items from the list 
	 *	@param Output The array to hold the returned items. Must be empty.
	 *	CAUTION: This method should not be used unless the list is known to not be closed.
	**/
	void PopAll(TArray<T *>& Output)
	{
		FLockFreeVoidPointerListBase::PopAll<TArray<T *>, T * >(Output);
	}	
	/**	
	 *	Close the list if it is empty
	 *	@return true if this call actively closed the list
	**/
	bool CloseIfEmpty()
	{
		return FLockFreeVoidPointerListBase::CloseIfEmpty();
	}
};


