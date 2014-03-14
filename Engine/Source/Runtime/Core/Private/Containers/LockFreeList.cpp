// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "CorePrivate.h"
#include "LockFreeList.h"

DEFINE_LOG_CATEGORY(LogLockFreeList);



FLockFreeVoidPointerListBase::FLinkAllocator& FLockFreeVoidPointerListBase::FLinkAllocator::Get()
{
	static FLinkAllocator TheAllocator;
	return TheAllocator;
}

FLockFreeVoidPointerListBase::FLinkAllocator::FLinkAllocator()
	: FreeLinks(NULL)
	, SpecialClosedLink(new FLink())
{
	ClosedLink()->LockCount.Increment();
}

FLockFreeVoidPointerListBase::FLinkAllocator::~FLinkAllocator()
{
	/** - Deliberately leak to allow LockFree to be used during shutdown
	* check(SpecialClosedLink); // double free?
	* check(!NumUsedLinks.GetValue());
	* while (FLink* ToDelete = FLink::Unlink(&FreeLinks))
	* {
	* 	delete ToDelete;
	* 	NumFreeLinks.Decrement();
	* }
	* check(!NumFreeLinks.GetValue());
	* delete SpecialClosedLink;
	* SpecialClosedLink = 0;
	*/
}
