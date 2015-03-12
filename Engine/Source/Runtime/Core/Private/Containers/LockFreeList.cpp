// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "LockFreeList.h"


DEFINE_LOG_CATEGORY(LogLockFreeList);

FThreadSafeCounter FLockFreeListStats::NumOperations;
FThreadSafeCounter FLockFreeListStats::Cycles;


#if USE_LOCKFREELIST_128

/*-----------------------------------------------------------------------------
	FLockFreeVoidPointerListBase128
-----------------------------------------------------------------------------*/

FLockFreeVoidPointerListBase128::FLinkAllocator& FLockFreeVoidPointerListBase128::FLinkAllocator::Get()
{
	static FLinkAllocator TheAllocator;
	return TheAllocator;
}

FLockFreeVoidPointerListBase128::FLinkAllocator::FLinkAllocator()
	: SpecialClosedLink( FLargePointer( new FLink(), FLockFreeListConstants::SpecialClosedLink ) ) 
{
	if( !FPlatformAtomics::CanUseCompareExchange128() )
	{
		// This is a fatal error.
		FPlatformMisc::MessageBoxExt( EAppMsgType::Ok, TEXT( "CPU does not support Compare and Exchange 128-bit operation. Unreal Engine 4 will exit now." ), TEXT( "Unsupported processor" ) );
		FPlatformMisc::RequestExit( true );
		UE_LOG( LogHAL, Fatal, TEXT( "CPU does not support Compare and Exchange 128-bit operation" ) );
	}
}
	

FLockFreeVoidPointerListBase128::FLinkAllocator::~FLinkAllocator()
{
 	// - Deliberately leak to allow LockFree to be used during shutdown
#if	0
 	check(SpecialClosedLink.Pointer); // double free?
 	check(!NumUsedLinks.GetValue());
 	while( FLink* ToDelete = FLink::Unlink(FreeLinks) )
 	{
 		delete ToDelete;
 		NumFreeLinks.Decrement();
 	}
 	check(!NumFreeLinks.GetValue());
 	delete SpecialClosedLink.Pointer;
 	SpecialClosedLink.Pointer = 0;
#endif // 0
}

#else

/*-----------------------------------------------------------------------------
	FLockFreeVoidPointerListBase
-----------------------------------------------------------------------------*/

FLockFreeVoidPointerListBase::FLinkAllocator FLockFreeVoidPointerListBase::FLinkAllocator::TheLinkAllocator;

FLockFreeVoidPointerListBase::FLinkAllocator::FLinkAllocator()
	: SpecialClosedLink(new FLink())
{
	static_assert(sizeof(FLink) >= sizeof(void*) && sizeof(FLink) % sizeof(void*) == 0, "Blocks in TLockFreeFixedSizeAllocator must be at least the size of a pointer.");
	check(IsInGameThread());
	TlsSlot = FPlatformTLS::AllocTlsSlot();
	check(FPlatformTLS::IsValidTlsSlot(TlsSlot));
}
FLockFreeVoidPointerListBase::FLinkAllocator::~FLinkAllocator()
{
	FPlatformTLS::FreeTlsSlot(TlsSlot);
	TlsSlot = 0;
}

void FLockFreeVoidPointerListBase::FLinkAllocator::PutFreeBundle(void **Bundle)
{
	FScopeLock Lock(&BundleArrayCriticalSection);
	BundleArray.Push(Bundle);
}
void** FLockFreeVoidPointerListBase::FLinkAllocator::GetFreeBundle()
{
	FScopeLock Lock(&BundleArrayCriticalSection);
	void** Result = nullptr;
	if (BundleArray.Num())
	{
		Result = BundleArray.Pop(false);
	}
	return Result;
}

#endif //USE_LOCKFREELIST_128

